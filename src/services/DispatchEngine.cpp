#include "ers/services/DispatchEngine.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>
#include "ers/common/Strings.h"

namespace ers {

namespace {
struct Candidate {
    Vehicle* vehicle;
    Route route;
};
}

void DispatchEngine::allocate(Emergency& incident, std::vector<std::unique_ptr<Vehicle>>& fleet,
                              double now, const std::vector<VehicleType>& wanted,
                              DispatchResult& result) {
    std::ostringstream explain;
    explain << std::fixed << std::setprecision(1);

    std::vector<VehicleType> types;
    for (VehicleType t : wanted)
        if (std::find(types.begin(), types.end(), t) == types.end()) types.push_back(t);

    for (VehicleType t : types) {
        int total = 0, unavailable = 0, incompatible = 0, unreachable = 0;
        std::vector<Candidate> candidates;

        for (auto& up : fleet) {
            Vehicle& v = *up;
            if (v.type() != t) continue;
            ++total;
            if (!v.isAvailable()) { ++unavailable; continue; }
            if (!v.canRespondTo(incident)) { ++incompatible; continue; }
            try {
                Route r = routing_.shortestPath(v.currentNode(), incident.nodeId());
                candidates.push_back(Candidate{&v, std::move(r)});
            } catch (const NoRouteException&) {
                ++unreachable;
            }
        }

        if (candidates.empty()) {
            result.unmet.push_back(t);
            explain << "No " << toString(t) << " dispatched for incident " << incident.id() << ": ";
            if (total == 0) explain << "none in fleet";
            else explain << unavailable << " unavailable, " << incompatible << " incompatible, "
                         << unreachable << " unreachable (out of " << total << ")";
            explain << "\n";
            continue;
        }

        auto best = std::min_element(candidates.begin(), candidates.end(),
            [](const Candidate& a, const Candidate& b) {
                if (a.route.totalMinutes() != b.route.totalMinutes())
                    return a.route.totalMinutes() < b.route.totalMinutes();
                return a.vehicle->id() < b.vehicle->id();
            });

        Vehicle& chosen = *best->vehicle;
        double eta = best->route.totalMinutes();
        chosen.assignIncident(incident.id());  // reserves the vehicle

        std::ostringstream reason;
        reason << std::fixed << std::setprecision(1);
        reason << toString(t) << " #" << chosen.id() << ": compatible and lowest ETA (" << eta
               << " min) among " << candidates.size() << " reachable available candidate(s)";

        result.records.emplace_back(nextDispatchId_++, incident.id(), chosen.id(), best->route,
                                    now, eta, reason.str());
        explain << reason.str() << "\n";
    }

    result.explanation += explain.str();
}

DispatchResult DispatchEngine::dispatch(Emergency& incident,
                                        std::vector<std::unique_ptr<Vehicle>>& fleet, double now) {
    if (incident.status() != EmergencyStatus::Pending)
        throw InvalidTransitionException("Only a pending incident can be dispatched");

    DispatchResult result;
    allocate(incident, fleet, now, incident.requiredResources(), result);

    if (result.anyAllocated()) incident.markDispatched();
    else result.explanation += "Incident " + std::to_string(incident.id()) +
                               " stays Pending / Awaiting Resource\n";
    return result;
}

DispatchResult DispatchEngine::supplement(Emergency& incident,
                                          std::vector<std::unique_ptr<Vehicle>>& fleet, double now,
                                          const std::vector<VehicleType>& types) {
    if (incident.status() != EmergencyStatus::Dispatched &&
        incident.status() != EmergencyStatus::OnScene)
        throw InvalidTransitionException("Only a dispatched or on-scene incident can be supplemented");

    DispatchResult result;
    allocate(incident, fleet, now, types, result);
    return result;
}

} // namespace ers