#pragma once
#include <map>
#include <vector>
#include "ers/models/CityNode.h"
#include "ers/models/Road.h"

namespace ers {

struct Edge {
    RoadId roadId;
    NodeId to;
};

class CityMap {
public:
    void addNode(const CityNode& node);
    void addRoad(const Road& road);

    bool hasNode(NodeId id) const;
    bool hasRoad(RoadId id) const;
    const CityNode& node(NodeId id) const;
    const Road& road(RoadId id) const;

    void setTraffic(RoadId id, double factor);
    void setBlocked(RoadId id, bool blocked);

    const std::vector<Edge>& neighbours(NodeId id) const;
    const std::map<NodeId, CityNode>& nodes() const { return nodes_; }
    const std::map<RoadId, Road>& roads() const { return roads_; }

private:
    std::map<NodeId, CityNode> nodes_;
    std::map<RoadId, Road> roads_;
    std::map<NodeId, std::vector<Edge>> adjacency_;
    std::vector<Edge> empty_;
};

} // namespace ers