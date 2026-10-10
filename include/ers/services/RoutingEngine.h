#pragma once
#include "ers/models/CityMap.h"
#include "ers/models/Route.h"

namespace ers {

class RoutingEngine {
public:
    explicit RoutingEngine(const CityMap& map) : map_(map) {}

    // Dijkstra on edge cost = baseMinutes * trafficFactor; blocked roads skipped
    Route shortestPath(NodeId from, NodeId to) const;

    // Function overloading (compile-time polymorphism)
    double calculateETA(const Route& route) const;
    double calculateETA(double distanceKm, double speedKmph) const;
    double calculateETA(const Route& route, double extraMultiplier) const;

private:
    const CityMap& map_;
};

} // namespace ers