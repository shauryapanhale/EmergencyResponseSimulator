#pragma once
#include <memory>
#include <string>
#include <vector>
#include "ers/models/DispatchRecord.h"
#include "ers/models/Emergency.h"
#include "ers/models/Vehicle.h"
#include "ers/services/RoutingEngine.h"

namespace ers {

struct DispatchResult {
    std::vector<DispatchRecord> records;  // one per allocated resource
    std::vector<VehicleType> unmet;       // resource types that could not be allocated
    std::string explanation;              // human-readable reasons

    bool anyAllocated() const { return !records.empty(); }
    bool fullyAllocated() const { return !records.empty() && unmet.empty(); }
};

class DispatchEngine {
public:
    explicit DispatchEngine(const RoutingEngine& routing) : routing_(routing) {}

    // Allocates each required resource type independently (partial allocation allowed).
    // Policy: filter unavailable/incompatible/unreachable, then lowest ETA, then lowest vehicle id.
    // The incident must be Pending. It becomes Dispatched if at least one vehicle was allocated.
    DispatchResult dispatch(Emergency& incident, std::vector<std::unique_ptr<Vehicle>>& fleet,
                            double now);

    // Retries only the given (previously unmet) resource types for an incident that is
    // already Dispatched or OnScene. The incident status is not changed.
    DispatchResult supplement(Emergency& incident, std::vector<std::unique_ptr<Vehicle>>& fleet,
                              double now, const std::vector<VehicleType>& types);

    int dispatchesMade() const { return nextDispatchId_ - 1; }

private:
    void allocate(Emergency& incident, std::vector<std::unique_ptr<Vehicle>>& fleet, double now,
                  const std::vector<VehicleType>& wanted, DispatchResult& result);

    const RoutingEngine& routing_;
    int nextDispatchId_ = 1;
};

} // namespace ers