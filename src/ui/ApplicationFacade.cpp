#include "ers/ui/ApplicationFacade.h"
#include <exception>
#include "ers/common/Exceptions.h"
#include "ers/common/Strings.h"
#include "ers/repositories/SampleCity.h"
#include "ers/services/Scenario.h"

namespace ers {

namespace {

QString q(const std::string& s) { return QString::fromStdString(s); }

QString statusText(EmergencyStatus s) {
    switch (s) {
        case EmergencyStatus::Pending: return "Pending";
        case EmergencyStatus::Dispatched: return "Dispatched";
        case EmergencyStatus::OnScene: return "OnScene";
        case EmergencyStatus::Resolved: return "Resolved";
        case EmergencyStatus::Unserved: return "Unserved";
    }
    return "Pending";
}

QString statusText(VehicleStatus s) {
    switch (s) {
        case VehicleStatus::Available: return "Available";
        case VehicleStatus::EnRoute: return "EnRoute";
        case VehicleStatus::OnScene: return "OnScene";
        case VehicleStatus::Returning: return "Returning";
        case VehicleStatus::OutOfService: return "OutOfService";
    }
    return "Available";
}

} // namespace

ApplicationFacade::ApplicationFacade(QObject* parent) : QObject(parent) {
    timer_.setInterval(500);
    connect(&timer_, &QTimer::timeout, this, &ApplicationFacade::stepOnce);
    createSimulation();
}

ApplicationFacade::~ApplicationFacade() = default;

template <typename F>
bool ApplicationFacade::guarded(F&& action) {
    try {
        action();
        return true;
    } catch (const ErsException& e) {
        emit errorOccurred(q(e.what()));
    } catch (const std::exception& e) {
        emit errorOccurred(q(e.what()));
    }
    return false;
}

void ApplicationFacade::createSimulation() {
    timer_.stop();
    sim_.reset();                 // engine first: it references city_.map
    city_ = buildSampleCity();    // fresh city: no leftover traffic or closures
    sim_ = std::make_unique<SimulationEngine>(city_.map, buildSampleFleet(), cfg_);
    logIndex_ = 0;
    running_ = false;
}

void ApplicationFacade::notify() {
    const auto& entries = sim_->log().entries();
    for (; logIndex_ < entries.size(); ++logIndex_)
        emit logLine(q(EventLog::format(entries[logIndex_])));
    emit timeChanged(sim_->time());
    emit stateChanged();
}

int ApplicationFacade::reportIncident(EmergencyType type, int nodeId, Priority priority,
                                      const QString& description) {
    int id = -1;
    if (guarded([&] { id = sim_->reportIncident(type, nodeId, priority, description.toStdString()); }))
        notify();
    return id;
}

bool ApplicationFacade::resolveIncident(int id) {
    bool ok = guarded([&] { sim_->resolveIncident(id); });
    if (ok) notify();
    return ok;
}

bool ApplicationFacade::changePriority(int id, Priority priority) {
    bool ok = guarded([&] { sim_->changeIncidentPriority(id, priority); });
    if (ok) notify();
    return ok;
}

bool ApplicationFacade::setTraffic(int roadId, double factor) {
    bool ok = guarded([&] { sim_->setTraffic(roadId, factor); });
    if (ok) notify();
    return ok;
}

bool ApplicationFacade::setBlocked(int roadId, bool blocked) {
    bool ok = guarded([&] { sim_->setBlocked(roadId, blocked); });
    if (ok) notify();
    return ok;
}

bool ApplicationFacade::runScenario(const QString& name) {
    const bool wasRunning = running_;
    bool ok = guarded([&] {
        const Scenario& sc = findScenario(name.toStdString());
        createSimulation();
        ers::runScenario(*sim_, sc);
    });
    if (!ok) return false;
    if (wasRunning) emit runningChanged(false);
    emit simulationReset();
    notify();
    emit scenarioFinished(name);
    return true;
}

void ApplicationFacade::setTickIntervalMs(int ms) {
    if (ms <= 0) {
        emit errorOccurred("Tick interval must be positive");
        return;
    }
    timer_.setInterval(ms);
}

void ApplicationFacade::setAutoResolveMinutes(double minutes) {
    cfg_.autoResolveMinutes = minutes;
    reset();
}

double ApplicationFacade::time() const { return sim_->time(); }

void ApplicationFacade::start() {
    if (running_) return;
    sim_->start();
    running_ = true;
    timer_.start();
    emit runningChanged(true);
    notify();
}

void ApplicationFacade::resume() {
    if (running_) return;
    sim_->resume();
    running_ = true;
    timer_.start();
    emit runningChanged(true);
    notify();
}

void ApplicationFacade::pause() {
    if (!running_) return;
    sim_->pause();
    timer_.stop();
    running_ = false;
    emit runningChanged(false);
    notify();
}

void ApplicationFacade::stepOnce() {
    if (guarded([&] { sim_->step(); })) notify();
}

void ApplicationFacade::reset() {
    const bool wasRunning = running_;
    createSimulation();
    if (wasRunning) emit runningChanged(false);
    emit simulationReset();
    notify();
}

std::vector<IncidentView> ApplicationFacade::incidents() const {
    std::vector<IncidentView> out;
    const auto records = sim_->dispatchRecords();
    for (EmergencyId id : sim_->incidentIds()) {
        const Emergency& e = sim_->incident(id);
        IncidentView v;
        v.id = id;
        v.type = q(e.typeName());
        v.priority = q(toString(e.priority()));
        v.status = statusText(e.status());
        v.nodeId = e.nodeId();
        v.nodeName = q(sim_->city().node(e.nodeId()).name());
        v.createdAt = e.createdAt();
        v.description = q(e.description());
        for (const auto& r : records)
            if (r.emergencyId() == id) v.vehicleIds.push_back(r.vehicleId());
        for (VehicleType t : sim_->unmetResources(id))
            v.unmet += (v.unmet.isEmpty() ? "" : ", ") + q(toString(t));
        v.explanation = q(sim_->explanationFor(id));
        out.push_back(v);
    }
    return out;
}

std::vector<VehicleView> ApplicationFacade::vehicles() const {
    std::vector<VehicleView> out;
    for (const auto& up : sim_->fleet()) {
        const Vehicle& veh = *up;
        VehicleView v;
        v.id = veh.id();
        v.type = q(toString(veh.type()));
        v.status = statusText(veh.status());
        v.stationId = veh.stationId();
        v.nodeId = veh.currentNode();
        Point p = sim_->vehiclePosition(veh.id());
        v.x = p.x;
        v.y = p.y;
        auto assigned = veh.assignedIncident();
        v.assignedIncident = assigned ? *assigned : -1;
        out.push_back(v);
    }
    return out;
}

StatsView ApplicationFacade::stats() const {
    const StatsSnapshot s = sim_->stats();
    StatsView v;
    v.created = s.created;
    v.resolved = s.resolved;
    v.active = s.active;
    v.pending = s.pending;
    v.responseCount = s.responseCount;
    v.avgResponse = s.avgResponse;
    v.minResponse = s.minResponse;
    v.maxResponse = s.maxResponse;
    v.avgWaiting = s.avgWaiting;
    v.avgResolution = s.avgResolution;
    v.resolvedWithUnmet = s.resolvedWithUnmet;
    return v;
}

QStringList ApplicationFacade::logLines() const {
    QStringList lines;
    for (const auto& e : sim_->log().entries()) lines << q(EventLog::format(e));
    return lines;
}

QString ApplicationFacade::explanationFor(int incidentId) const {
    return q(sim_->explanationFor(incidentId));
}

QStringList ApplicationFacade::scenarioNames() const {
    QStringList names;
    for (const Scenario& s : sampleScenarios()) names << q(s.name);
    return names;
}

QString ApplicationFacade::disclaimer() {
    return "Educational simulation only - not for real emergency use.";
}

} // namespace ers