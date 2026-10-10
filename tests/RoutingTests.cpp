#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "ers/models/CityMap.h"
#include "ers/services/RoutingEngine.h"

using namespace ers;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; ++failures; } } while (0)
static bool nearEq(double a, double b) { return std::fabs(a - b) < 1e-9; }

static CityMap makeMap() {
    CityMap m;
    for (int i = 1; i <= 5; ++i) m.addNode(CityNode(i, "N" + std::to_string(i), i * 10.0, 0));
    m.addRoad(Road(1, 1, 2, 2.0, 5.0));
    m.addRoad(Road(2, 2, 4, 2.0, 5.0));
    m.addRoad(Road(3, 1, 3, 1.0, 3.0));
    m.addRoad(Road(4, 3, 4, 1.0, 3.0));
    return m; // node 5 is disconnected
}

int main() {
    CityMap m = makeMap();
    RoutingEngine eng(m);

    Route r = eng.shortestPath(1, 4);
    CHECK(nearEq(r.totalMinutes(), 6.0));
    CHECK((r.nodes() == std::vector<NodeId>{1, 3, 4}));
    CHECK((r.roadIds() == std::vector<RoadId>{3, 4}));

    Route same = eng.shortestPath(2, 2);
    CHECK(nearEq(same.totalMinutes(), 0.0) && same.nodes().size() == 1);

    Route again = eng.shortestPath(1, 4);
    CHECK(again.nodes() == r.nodes());

    m.setBlocked(3, true);
    Route alt = eng.shortestPath(1, 4);
    CHECK(nearEq(alt.totalMinutes(), 10.0));
    CHECK((alt.nodes() == std::vector<NodeId>{1, 2, 4}));
    m.setBlocked(3, false);

    m.setTraffic(4, 4.0);
    Route heavy = eng.shortestPath(1, 4);
    CHECK(nearEq(heavy.totalMinutes(), 10.0));
    m.setTraffic(4, 1.0);

    bool noRoute = false;
    try { eng.shortestPath(1, 5); } catch (const NoRouteException&) { noRoute = true; }
    CHECK(noRoute);

    bool invalid = false;
    try { eng.shortestPath(1, 99); } catch (const ValidationException&) { invalid = true; }
    CHECK(invalid);

    bool badRoad = false;
    try { m.addRoad(Road(9, 1, 77, 1.0, 1.0)); } catch (const ValidationException&) { badRoad = true; }
    CHECK(badRoad);

    CHECK(nearEq(eng.calculateETA(r), 6.0));
    CHECK(nearEq(eng.calculateETA(30.0, 60.0), 30.0));
    CHECK(nearEq(eng.calculateETA(r, 1.5), 9.0));

    if (failures == 0) std::cout << "routing tests passed\n";
    return failures == 0 ? 0 : 1;
}