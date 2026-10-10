#include "ers/models/CityMap.h"

namespace ers {

void CityMap::addNode(const CityNode& node) {
    if (nodes_.count(node.id())) throw ValidationException("Duplicate node id");
    nodes_.emplace(node.id(), node);
    adjacency_[node.id()];
}

void CityMap::addRoad(const Road& road) {
    if (roads_.count(road.id())) throw ValidationException("Duplicate road id");
    if (!hasNode(road.from()) || !hasNode(road.to()))
        throw ValidationException("Road endpoint does not exist");
    roads_.emplace(road.id(), road);
    adjacency_[road.from()].push_back({road.id(), road.to()});
    if (road.isBidirectional())
        adjacency_[road.to()].push_back({road.id(), road.from()});
}

bool CityMap::hasNode(NodeId id) const { return nodes_.count(id) > 0; }
bool CityMap::hasRoad(RoadId id) const { return roads_.count(id) > 0; }

const CityNode& CityMap::node(NodeId id) const {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) throw ValidationException("Unknown node id");
    return it->second;
}

const Road& CityMap::road(RoadId id) const {
    auto it = roads_.find(id);
    if (it == roads_.end()) throw ValidationException("Unknown road id");
    return it->second;
}

void CityMap::setTraffic(RoadId id, double factor) {
    auto it = roads_.find(id);
    if (it == roads_.end()) throw ValidationException("Unknown road id");
    it->second.setTrafficFactor(factor);
}

void CityMap::setBlocked(RoadId id, bool blocked) {
    auto it = roads_.find(id);
    if (it == roads_.end()) throw ValidationException("Unknown road id");
    it->second.setBlocked(blocked);
}

const std::vector<Edge>& CityMap::neighbours(NodeId id) const {
    auto it = adjacency_.find(id);
    return it == adjacency_.end() ? empty_ : it->second;
}

} // namespace ers