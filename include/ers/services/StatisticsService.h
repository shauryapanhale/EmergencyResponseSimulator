#pragma once
#include <map>
#include <optional>
#include <utility>
#include <vector>
#include "ers/common/Exceptions.h"
#include "ers/common/Types.h"

namespace ers {

struct StatsSnapshot {
    int created = 0;
    int resolved = 0;
    int resolvedWithUnmet = 0;  // resolved while a required resource type was still unmet
    int active = 0;      // dispatched but not resolved
    int pending = 0;     // never dispatched, not resolved
    int unserved = 0;    // incidents still waiting for any resource (same as pending)
    int responseCount = 0;
    double avgResponse = 0.0, minResponse = 0.0, maxResponse = 0.0;  // arrival - dispatch
    double avgWaiting = 0.0;                                         // first dispatch - creation
    double avgResolution = 0.0;                                      // resolved - creation
    std::map<VehicleId, double> utilization;                         // busy time / tracked time
};

class StatisticsService {
public:
    void incidentCreated(EmergencyId id, double simTime);
    void incidentDispatched(EmergencyId id, double simTime);  // only the first dispatch counts
    void vehicleArrived(EmergencyId id, double dispatchedAt, double arrivedAt);
    void incidentResolved(EmergencyId id, double simTime, bool hadUnmetResources = false);
    void trackVehicle(VehicleId id, bool busy, double dtMinutes);
    void reset();
    StatsSnapshot snapshot() const;

private:
    struct IncidentRecord {
        double createdAt;
        std::optional<double> firstDispatchAt;
        std::optional<double> resolvedAt;
        bool resolvedWithUnmet = false;
    };
    IncidentRecord& find(EmergencyId id);

    std::map<EmergencyId, IncidentRecord> incidents_;
    std::vector<double> responseTimes_;
    std::map<VehicleId, std::pair<double, double>> busyTotal_;  // busy, total
};

} // namespace ers