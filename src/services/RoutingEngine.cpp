#include "ers/services/RoutingEngine.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <utility>

namespace ers {

Route RoutingEngine::shortestPath(NodeId from, NodeId to) const {
    if (!map_.hasNode(from) || !map_.hasNode(to))
        throw ValidationException("Route endpoint does not exist");

    if (from == to) return Route({from}, {}, 0.0);

    const double INF = std::numeric_limits<double>::infinity();
    std::map<NodeId, double> dist;
    std::map<NodeId, std::pair<NodeId, RoadId>> prev;
    for (const auto& kv : map_.nodes()) dist[kv.first] = INF;
    dist[from] = 0.0;

    using Item = std::pair<double, NodeId>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
    pq.push({0.0, from});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;
        if (u == to) break;
        for (const Edge& e : map_.neighbours(u)) {
            const Road& r = map_.road(e.roadId);
            if (r.isBlocked()) continue;
            double nd = d + r.currentMinutes();
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                prev[e.to] = {u, e.roadId};
                pq.push({nd, e.to});
            }
        }
    }

    if (dist[to] == INF) throw NoRouteException("No route available under current road conditions");

    std::vector<NodeId> nodes;
    std::vector<RoadId> roads;
    NodeId cur = to;
    nodes.push_back(cur);
    while (cur != from) {
        auto p = prev.at(cur);
        roads.push_back(p.second);
        cur = p.first;
        nodes.push_back(cur);
    }
    std::reverse(nodes.begin(), nodes.end());
    std::reverse(roads.begin(), roads.end());
    return Route(std::move(nodes), std::move(roads), dist[to]);
}

double RoutingEngine::calculateETA(const Route& route) const {
    return route.totalMinutes();
}

double RoutingEngine::calculateETA(double distanceKm, double speedKmph) const {
    if (distanceKm < 0 || speedKmph <= 0)
        throw ValidationException("Distance must be >= 0 and speed > 0");
    return distanceKm / speedKmph * 60.0;
}

double RoutingEngine::calculateETA(const Route& route, double extraMultiplier) const {
    if (extraMultiplier <= 0) throw ValidationException("Multiplier must be positive");
    return route.totalMinutes() * extraMultiplier;
}

} // namespace ers