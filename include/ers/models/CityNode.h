#pragma once
#include <string>
#include <utility>
#include "ers/common/Types.h"
#include "ers/common/Exceptions.h"

namespace ers {

enum class NodeKind { Intersection, Landmark, Station };

class CityNode {
public:
    CityNode(NodeId id, std::string name, double x, double y,
             NodeKind kind = NodeKind::Intersection)
        : id_(id), name_(std::move(name)), x_(x), y_(y), kind_(kind) {
        if (id_ < 0) throw ValidationException("Node id must be non-negative");
        if (name_.empty()) throw ValidationException("Node name must not be empty");
    }

    NodeId id() const { return id_; }
    const std::string& name() const { return name_; }
    double x() const { return x_; }
    double y() const { return y_; }
    NodeKind kind() const { return kind_; }

private:
    NodeId id_;
    std::string name_;
    double x_;
    double y_;
    NodeKind kind_;
};

} // namespace ers