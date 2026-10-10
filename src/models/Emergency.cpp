#include "ers/models/Emergency.h"
#include <utility>

namespace ers {

int Emergency::totalCreated_ = 0;

Emergency::Emergency(EmergencyId id, NodeId nodeId, Priority priority, double createdAt,
                     std::string description)
    : id_(id), nodeId_(nodeId), priority_(priority), createdAt_(createdAt),
      status_(EmergencyStatus::Pending), description_(std::move(description)) {
    if (id_ < 0) throw InvalidIncidentException("Incident id must be non-negative");
    if (nodeId_ < 0) throw InvalidIncidentException("Incident location is invalid");
    if (createdAt_ < 0) throw InvalidIncidentException("Creation time must be non-negative");
    if (description_.size() > 200)
        throw InvalidIncidentException("Description must be at most 200 characters");
    ++totalCreated_;
}

int Emergency::totalCreated() { return totalCreated_; }

void Emergency::changePriority(Priority p) {
    if (status_ == EmergencyStatus::Resolved || status_ == EmergencyStatus::Unserved)
        throw InvalidTransitionException("Cannot change priority of a closed incident");
    priority_ = p;
}

void Emergency::markDispatched() {
    if (status_ != EmergencyStatus::Pending)
        throw InvalidTransitionException("Only a pending incident can be dispatched");
    status_ = EmergencyStatus::Dispatched;
}

void Emergency::markOnScene() {
    if (status_ != EmergencyStatus::Dispatched)
        throw InvalidTransitionException("Only a dispatched incident can be on scene");
    status_ = EmergencyStatus::OnScene;
}

void Emergency::markResolved() {
    if (status_ != EmergencyStatus::OnScene)
        throw InvalidTransitionException("Only an on-scene incident can be resolved");
    status_ = EmergencyStatus::Resolved;
}

void Emergency::markUnserved() {
    if (status_ != EmergencyStatus::Pending)
        throw InvalidTransitionException("Only a pending incident can be marked unserved");
    status_ = EmergencyStatus::Unserved;
}

} // namespace ers