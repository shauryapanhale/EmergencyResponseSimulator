#include "ers/services/StatisticsService.h"
#include <algorithm>
#include <numeric>

namespace ers {

StatisticsService::IncidentRecord& StatisticsService::find(EmergencyId id) {
    auto it = incidents_.find(id);
    if (it == incidents_.end()) throw ValidationException("Statistics: unknown incident id");
    return it->second;
}

void StatisticsService::incidentCreated(EmergencyId id, double simTime) {
    if (incidents_.count(id)) throw ValidationException("Statistics: duplicate incident id");
    incidents_[id] = IncidentRecord{simTime, std::nullopt, std::nullopt};
}

void StatisticsService::incidentDispatched(EmergencyId id, double simTime) {
    IncidentRecord& r = find(id);
    if (simTime < r.createdAt) throw ValidationException("Statistics: dispatch precedes creation");
    if (!r.firstDispatchAt) r.firstDispatchAt = simTime;
}

void StatisticsService::vehicleArrived(EmergencyId id, double dispatchedAt, double arrivedAt) {
    find(id);
    if (arrivedAt < dispatchedAt) throw ValidationException("Statistics: arrival precedes dispatch");
    responseTimes_.push_back(arrivedAt - dispatchedAt);
}

void StatisticsService::incidentResolved(EmergencyId id, double simTime, bool hadUnmetResources) {
    IncidentRecord& r = find(id);
    if (r.resolvedAt) throw ValidationException("Statistics: incident already resolved");
    if (simTime < r.createdAt) throw ValidationException("Statistics: resolution precedes creation");
    r.resolvedAt = simTime;
    r.resolvedWithUnmet = hadUnmetResources;
}

void StatisticsService::trackVehicle(VehicleId id, bool busy, double dtMinutes) {
    if (dtMinutes <= 0) throw ValidationException("Statistics: time step must be positive");
    auto& bt = busyTotal_[id];
    if (busy) bt.first += dtMinutes;
    bt.second += dtMinutes;
}

void StatisticsService::reset() {
    incidents_.clear();
    responseTimes_.clear();
    busyTotal_.clear();
}

StatsSnapshot StatisticsService::snapshot() const {
    StatsSnapshot s;
    s.created = static_cast<int>(incidents_.size());
    double waitSum = 0, resSum = 0;
    int waitN = 0, resN = 0;
    for (const auto& kv : incidents_) {
        const IncidentRecord& r = kv.second;
        if (r.firstDispatchAt) { waitSum += *r.firstDispatchAt - r.createdAt; ++waitN; }
        if (r.resolvedAt) {
            ++s.resolved;
            if (r.resolvedWithUnmet) ++s.resolvedWithUnmet;
            resSum += *r.resolvedAt - r.createdAt;
            ++resN;
        }
        else if (r.firstDispatchAt) ++s.active;
        else ++s.pending;
    }
    s.unserved = s.pending;
    if (waitN) s.avgWaiting = waitSum / waitN;
    if (resN) s.avgResolution = resSum / resN;

    if (!responseTimes_.empty()) {
        s.responseCount = static_cast<int>(responseTimes_.size());
        s.avgResponse = std::accumulate(responseTimes_.begin(), responseTimes_.end(), 0.0) / s.responseCount;
        s.minResponse = *std::min_element(responseTimes_.begin(), responseTimes_.end());
        s.maxResponse = *std::max_element(responseTimes_.begin(), responseTimes_.end());
    }
    for (const auto& kv : busyTotal_)
        s.utilization[kv.first] = kv.second.second > 0 ? kv.second.first / kv.second.second : 0.0;
    return s;
}

} // namespace ers