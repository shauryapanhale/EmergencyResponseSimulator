#pragma once
#include <string>
#include <vector>
#include "ers/common/Types.h"
#include "ers/common/Exceptions.h"

namespace ers {

// Abstract base class: cannot be instantiated, derived types implement the pure virtuals
class Emergency {
public:
    Emergency(EmergencyId id, NodeId nodeId, Priority priority, double createdAt,
              std::string description);
    virtual ~Emergency() = default;

    // Polymorphic objects are not copied
    Emergency(const Emergency&) = delete;
    Emergency& operator=(const Emergency&) = delete;

    // Pure virtual functions (abstraction)
    virtual EmergencyType type() const = 0;
    virtual std::string typeName() const = 0;
    virtual std::vector<VehicleType> requiredResources() const = 0;

    EmergencyId id() const { return id_; }
    NodeId nodeId() const { return nodeId_; }
    Priority priority() const { return priority_; }
    double createdAt() const { return createdAt_; }
    EmergencyStatus status() const { return status_; }
    const std::string& description() const { return description_; }

    // Encapsulation: state changes go through validated methods
    void changePriority(Priority p);
    void markDispatched();
    void markOnScene();
    void markResolved();
    void markUnserved();

    // Static member: class-wide count of incidents ever constructed
    static int totalCreated();

private:
    EmergencyId id_;
    NodeId nodeId_;
    Priority priority_;
    double createdAt_;
    EmergencyStatus status_;
    std::string description_;

    static int totalCreated_;
};

} // namespace ers