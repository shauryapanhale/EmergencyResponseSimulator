#include "ers/services/SimulationEngine.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>
#include "ers/common/Strings.h"

namespace ers {

namespace {
const double EPS = 1e-9;

std::string fmt1(double v) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << v;
    return os.str();
}

std::string oneLine(std::string s) {
    while (!s.empty() && s.back() == '\n') s.pop_back();
    for (auto& c : s) if (c == '\n') c = ';';
    return s;
}
}

SimulationEngine::SimulationEngine(CityMap& city, std::vector<std::unique_ptr<Vehicle>> fleet,
                                   SimulationConfig config)
    : city_(city), routing_(city_), dispatcher_(routing_), fleet_(std::move(fleet)), cfg_(config) {
    if (cfg_.tickMinutes <= 0) throw ValidationException("Tick length must be positive");
    for (std::size_t i = 0; i < fleet_.size(); ++i) {
        if (!fleet_[i]) throw ValidationException("Null vehicle in fleet");
        if (!city_.hasNode(fleet_[i]->currentNode()))
            throw ValidationException("Vehicle is placed on an unknown node");
        if (!vehicleIndex_.emplace(fleet_[i]->id(), i).second)
            throw ValidationException("Duplicate vehicle id");
    }
    log_.append(0.0, "Simulation", 0, "Created",
                "Simulation initialised with " + std::to_string(fleet_.size()) + " vehicles");
}

void SimulationEngine::start() { running_ = true; log_.append(now_, "Simulation", 0, "Started"); }
void SimulationEngine::pause() { running_ = false; log_.append(now_, "Simulation", 0, "Paused"); }
void SimulationEngine::resume() { running_ = true; log_.append(now_, "Simulation", 0, "Resumed"); }

Emergency& SimulationEngine::findIncident(EmergencyId id) {
    auto it = incidents_.find(id);
    if (it == incidents_.end()) throw ValidationException("Unknown incident id");
    return *it->second;
}

const Emergency& SimulationEngine::incident(EmergencyId id) const {
    auto it = incidents_.find(id);
    if (it == incidents_.end()) throw ValidationException("Unknown incident id");
    return *it->second;
}

std::vector<EmergencyId> SimulationEngine::incidentIds() const {
    std::vector<EmergencyId> ids;
    for (const auto& kv : incidents_) ids.push_back(kv.first);
    return ids;
}

std::vector<VehicleType> SimulationEngine::unmetResources(EmergencyId id) const {
    auto it = unmet_.find(id);
    if (it == unmet_.end()) return {};
    return it->second;
}

Vehicle& SimulationEngine::vehicleRef(VehicleId id) {
    auto it = vehicleIndex_.find(id);
    if (it == vehicleIndex_.end()) throw ValidationException("Unknown vehicle id");
    return *fleet_[it->second];
}

const Vehicle& SimulationEngine::vehicle(VehicleId id) const {
    auto it = vehicleIndex_.find(id);
    if (it == vehicleIndex_.end()) throw ValidationException("Unknown vehicle id");
    return *fleet_[it->second];
}

std::vector<DispatchRecord> SimulationEngine::dispatchRecords() const {
    std::vector<DispatchRecord> out;
    for (const auto& r : responses_) out.push_back(r.record);
    return out;
}

std::string SimulationEngine::explanationFor(EmergencyId id) const {
    auto it = explanations_.find(id);
    return it == explanations_.end() ? std::string() : it->second;
}

EmergencyId SimulationEngine::reportIncident(EmergencyType type, NodeId node, Priority priority,
                                             const std::string& description) {
    if (!city_.hasNode(node)) throw ValidationException("Incident location does not exist");
    EmergencyId id = nextIncidentId_;
    auto e = createEmergency(type, id, node, priority, now_, description);
    ++nextIncidentId_;
    queue_.push(*e);
    stats_.incidentCreated(id, now_);
    log_.append(now_, "Incident", id, "Created",
                toString(type) + " (" + toString(priority) + ") at " + city_.node(node).name() +
                    ": " + description);
    incidents_[id] = std::move(e);
    processPending();
    return id;
}

void SimulationEngine::changeIncidentPriority(EmergencyId id, Priority p) {
    Emergency& e = findIncident(id);
    Priority old = e.priority();
    e.changePriority(p);
    if (queue_.contains(id)) queue_.push(e);  // re-prioritize (versioned entry)
    log_.append(now_, "Incident", id, "PriorityChanged", toString(old) + " -> " + toString(p));
}

void SimulationEngine::setTraffic(RoadId road, double factor) {
    city_.setTraffic(road, factor);
    log_.append(now_, "Road", road, "TrafficChanged", "factor " + fmt1(factor));
}

void SimulationEngine::setBlocked(RoadId road, bool blocked) {
    city_.setBlocked(road, blocked);
    log_.append(now_, "Road", road, blocked ? "Blocked" : "Unblocked");
}

// Handles never-dispatched incidents and unmet resource retries together, highest priority first.
void SimulationEngine::processPending() {
    struct Work {
        EmergencyId id;
        bool supplement;
    };
    std::vector<Work> work;
    for (EmergencyId id : queue_.snapshot()) work.push_back(Work{id, false});
    for (const auto& kv : unmet_) work.push_back(Work{kv.first, true});

    std::stable_sort(work.begin(), work.end(), [this](const Work& a, const Work& b) {
        const Emergency& x = *incidents_.at(a.id);
        const Emergency& y = *incidents_.at(b.id);
        if (x.priority() != y.priority())
            return static_cast<int>(x.priority()) < static_cast<int>(y.priority());
        if (x.createdAt() != y.createdAt()) return x.createdAt() < y.createdAt();
        return x.id() < y.id();
    });

    for (const Work& w : work) {
        Emergency& e = *incidents_.at(w.id);

        if (!w.supplement) {
            DispatchResult r = dispatcher_.dispatch(e, fleet_, now_);
            explanations_[w.id] = r.explanation;
            if (!r.anyAllocated()) {
                if (loggedPending_.insert(w.id).second)
                    log_.append(now_, "Incident", w.id, "Pending", oneLine(r.explanation));
                continue;
            }
            queue_.remove(w.id);
            registerDispatch(e, r);
            continue;
        }

        if (e.status() == EmergencyStatus::Resolved || e.status() == EmergencyStatus::Unserved) {
            log_.append(now_, "Incident", w.id, "UnmetDropped",
                        "Incident closed with unmet resource types");
            unmet_.erase(w.id);
            continue;
        }

        const std::vector<VehicleType> types = unmet_.at(w.id);
        DispatchResult r = dispatcher_.supplement(e, fleet_, now_, types);
        if (!r.anyAllocated()) continue;
        explanations_[w.id] += r.explanation;
        registerDispatch(e, r);
    }
}

void SimulationEngine::registerDispatch(Emergency& e, const DispatchResult& result) {
    stats_.incidentDispatched(e.id(), now_);
    for (const DispatchRecord& rec : result.records) {
        log_.append(now_, "Dispatch", rec.id(), "Dispatched",
                    "Emergency " + std::to_string(e.id()) + " <- Vehicle " +
                        std::to_string(rec.vehicleId()) + ", ETA " + fmt1(rec.etaMinutes()) +
                        " min | " + rec.reason());
        responses_.emplace_back(rec);
        activeByVehicle_[rec.vehicleId()] = responses_.size() - 1;
    }
    if (result.unmet.empty()) {
        unmet_.erase(e.id());
    } else {
        unmet_[e.id()] = result.unmet;
        std::string missing;
        for (VehicleType t : result.unmet) missing += (missing.empty() ? "" : ", ") + toString(t);
        log_.append(now_, "Incident", e.id(), "PartialAllocation", "Unmet: " + missing);
    }
}

double SimulationEngine::moveAlong(Response& r, double dt, bool& reached) {
    Vehicle& v = vehicleRef(r.record.vehicleId());
    const double factor = v.timeFactor();
    double remaining = dt / factor;  // progress is kept in road minutes
    while (r.seg < r.roads.size()) {
        const Road& road = city_.road(r.roads[r.seg]);
        double need = road.currentMinutes() - r.progress;
        if (remaining + EPS >= need) {
            remaining = std::max(0.0, remaining - need);
            r.progress = 0.0;
            ++r.seg;
            v.setCurrentNode(r.nodes[r.seg]);
        } else {
            r.progress += remaining;
            remaining = 0.0;
            break;
        }
    }
    reached = r.seg >= r.roads.size();
    return (dt / factor - remaining) * factor;
}

void SimulationEngine::handleArrival(Response& r, double time) {
    Vehicle& v = vehicleRef(r.record.vehicleId());
    if (r.phase == Phase::ToScene) {
        v.markArrived();
        r.record.markArrived(time);
        r.phase = Phase::OnScene;
        r.arrivedAt = time;
        Emergency& e = findIncident(r.record.emergencyId());
        if (e.status() == EmergencyStatus::Dispatched) {
            e.markOnScene();
            log_.append(time, "Incident", e.id(), "OnScene");
        }
        stats_.vehicleArrived(e.id(), r.record.dispatchedAt(), time);
        log_.append(time, "Vehicle", v.id(), "Arrived",
                    "at incident " + std::to_string(e.id()) + ", response time " +
                        fmt1(r.record.responseTime()) + " min");
    } else {
        v.markBackAtBase();
        r.phase = Phase::Done;
        activeByVehicle_.erase(v.id());
        log_.append(time, "Vehicle", v.id(), "BackAtBase");
    }
}

void SimulationEngine::resolveIncident(EmergencyId id) {
    Emergency& e = findIncident(id);
    if (e.status() != EmergencyStatus::OnScene)
        throw InvalidTransitionException("Incident is not on scene");
    for (const Response& r : responses_)
        if (r.record.emergencyId() == id && r.phase == Phase::ToScene)
            throw InvalidTransitionException("Assigned vehicles are still en route");

    e.markResolved();
    stats_.incidentResolved(id, now_, unmet_.count(id) > 0);
    log_.append(now_, "Incident", id, "Resolved");

    for (Response& r : responses_) {
        if (r.record.emergencyId() != id || r.phase != Phase::OnScene) continue;
        Vehicle& v = vehicleRef(r.record.vehicleId());
        r.record.markCompleted(now_);
        v.markReturning();
        try {
            Route home = routing_.shortestPath(v.currentNode(), v.homeNode());
            r.nodes = home.nodes();
            r.roads = home.roadIds();
            r.seg = 0;
            r.progress = 0.0;
            r.phase = Phase::Returning;
            log_.append(now_, "Vehicle", v.id(), "Returning", "ETA home " + fmt1(home.totalMinutes() * v.timeFactor()) + " min");
        } catch (const NoRouteException&) {
            log_.append(now_, "Vehicle", v.id(), "NoRouteHome", "returned to base instantly");
            v.markBackAtBase();
            r.phase = Phase::Done;
            activeByVehicle_.erase(v.id());
        }
    }
}

void SimulationEngine::autoResolve() {
    if (cfg_.autoResolveMinutes < 0) return;
    std::vector<EmergencyId> ready;
    for (const auto& kv : incidents_) {
        if (kv.second->status() != EmergencyStatus::OnScene) continue;
        bool allThere = true;
        double last = 0.0;
        for (const Response& r : responses_) {
            if (r.record.emergencyId() != kv.first) continue;
            if (r.phase == Phase::ToScene) { allThere = false; break; }
            if (r.phase == Phase::OnScene) last = std::max(last, r.arrivedAt);
        }
        if (allThere && last + cfg_.autoResolveMinutes <= now_ + EPS) ready.push_back(kv.first);
    }
    for (EmergencyId id : ready) resolveIncident(id);
}

void SimulationEngine::step() {
    const double t0 = now_;
    for (std::size_t i = 0; i < responses_.size(); ++i) {
        Response& r = responses_[i];
        if (r.phase != Phase::ToScene && r.phase != Phase::Returning) continue;
        bool reached = false;
        double used = moveAlong(r, cfg_.tickMinutes, reached);
        if (reached) handleArrival(r, t0 + used);
    }
    now_ = t0 + cfg_.tickMinutes;
    autoResolve();
    processPending();
    for (const auto& v : fleet_) {
        bool busy = v->status() == VehicleStatus::EnRoute || v->status() == VehicleStatus::OnScene;
        stats_.trackVehicle(v->id(), busy, cfg_.tickMinutes);
    }
}

void SimulationEngine::runTicks(int n) {
    for (int i = 0; i < n; ++i) step();
}

Point SimulationEngine::vehiclePosition(VehicleId id) const {
    const Vehicle& v = vehicle(id);
    auto it = activeByVehicle_.find(id);
    if (it != activeByVehicle_.end()) {
        const Response& r = responses_[it->second];
        if ((r.phase == Phase::ToScene || r.phase == Phase::Returning) && r.seg < r.roads.size()) {
            const CityNode& a = city_.node(r.nodes[r.seg]);
            const CityNode& b = city_.node(r.nodes[r.seg + 1]);
            double dur = city_.road(r.roads[r.seg]).currentMinutes();
            double f = dur > 0 ? std::min(1.0, r.progress / dur) : 0.0;
            return Point{a.x() + (b.x() - a.x()) * f, a.y() + (b.y() - a.y()) * f};
        }
    }
    const CityNode& n = city_.node(v.currentNode());
    return Point{n.x(), n.y()};
}

} // namespace ers