#include "ers/models/DispatchRecord.h"
#include <iomanip>
#include <sstream>
#include <utility>

namespace ers {

DispatchRecord::DispatchRecord(int id, EmergencyId emergencyId, VehicleId vehicleId, Route route,
                               double dispatchedAt, double etaMinutes, std::string reason)
    : id_(id), emergencyId_(emergencyId), vehicleId_(vehicleId), route_(std::move(route)),
      dispatchedAt_(dispatchedAt), etaMinutes_(etaMinutes), reason_(std::move(reason)) {
    if (dispatchedAt_ < 0) throw ValidationException("Dispatch time must be non-negative");
    if (etaMinutes_ < 0) throw ValidationException("ETA must be non-negative");
}

double DispatchRecord::arrivedAt() const {
    if (!arrivedAt_) throw ValidationException("Vehicle has not arrived yet");
    return *arrivedAt_;
}

double DispatchRecord::completedAt() const {
    if (!completedAt_) throw ValidationException("Response is not completed yet");
    return *completedAt_;
}

void DispatchRecord::markArrived(double simTime) {
    if (arrivedAt_) throw ValidationException("Arrival already recorded");
    if (simTime < dispatchedAt_) throw ValidationException("Arrival cannot precede dispatch");
    arrivedAt_ = simTime;
}

void DispatchRecord::markCompleted(double simTime) {
    if (!arrivedAt_) throw ValidationException("Cannot complete before arrival");
    if (completedAt_) throw ValidationException("Completion already recorded");
    if (simTime < *arrivedAt_) throw ValidationException("Completion cannot precede arrival");
    completedAt_ = simTime;
}

double DispatchRecord::responseTime() const { return arrivedAt() - dispatchedAt_; }

std::string dispatchAudit(const DispatchRecord& r) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1);
    os << "Dispatch #" << r.id_ << ": Emergency " << r.emergencyId_ << " -> Vehicle " << r.vehicleId_
       << ", dispatched at t=" << r.dispatchedAt_ << ", ETA " << r.etaMinutes_ << " min";
    if (r.arrivedAt_) os << ", arrived at t=" << *r.arrivedAt_;
    else os << ", not arrived yet";
    os << " | " << r.reason_;
    return os.str();
}

} // namespace ers