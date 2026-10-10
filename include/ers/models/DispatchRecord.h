#pragma once
#include <optional>
#include <string>
#include "ers/common/Types.h"
#include "ers/common/Exceptions.h"
#include "ers/models/Route.h"

namespace ers {

// Audit record: why a vehicle was chosen and how the response progressed
class DispatchRecord {
public:
    DispatchRecord(int id, EmergencyId emergencyId, VehicleId vehicleId, Route route,
                   double dispatchedAt, double etaMinutes, std::string reason);

    int id() const { return id_; }
    EmergencyId emergencyId() const { return emergencyId_; }
    VehicleId vehicleId() const { return vehicleId_; }
    const Route& route() const { return route_; }
    double dispatchedAt() const { return dispatchedAt_; }
    double etaMinutes() const { return etaMinutes_; }
    const std::string& reason() const { return reason_; }

    bool hasArrived() const { return arrivedAt_.has_value(); }
    bool hasCompleted() const { return completedAt_.has_value(); }
    double arrivedAt() const;
    double completedAt() const;

    void markArrived(double simTime);
    void markCompleted(double simTime);
    double responseTime() const;  // arrival - dispatch

    // Narrow friend: the audit printer may read private fields directly
    friend std::string dispatchAudit(const DispatchRecord& r);

private:
    int id_;
    EmergencyId emergencyId_;
    VehicleId vehicleId_;
    Route route_;
    double dispatchedAt_;
    double etaMinutes_;
    std::string reason_;
    std::optional<double> arrivedAt_;
    std::optional<double> completedAt_;
};

std::string dispatchAudit(const DispatchRecord& r);

} // namespace ers