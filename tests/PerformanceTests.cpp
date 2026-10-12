#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ers/models/EmergencyFactory.h"
#include "ers/models/VehicleFactory.h"
#include "ers/services/DispatchEngine.h"
#include "ers/services/SimulationEngine.h"

using namespace ers;
using Clock = std::chrono::steady_clock;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; ++failures; } } while (0)

using Fleet = std::vector<std::unique_ptr<Vehicle>>;

static const int ROWS = 20;
static const int COLS = 25;
static const int NODES = ROWS * COLS;  // 500 nodes, 955 roads

static double msSince(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

static CityMap makeGrid() {
    CityMap m;
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c) {
            int id = r * COLS + c;
            m.addNode(CityNode(id, "N" + std::to_string(id), c * 10.0, r * 10.0));
        }
    int roadId = 1;
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c) {
            int id = r * COLS + c;
            if (c + 1 < COLS) { m.addRoad(Road(roadId, id, id + 1, 1.0 + roadId % 3, 2.0 + roadId % 5)); ++roadId; }
            if (r + 1 < ROWS) { m.addRoad(Road(roadId, id, id + COLS, 1.0 + roadId % 3, 2.0 + roadId % 5)); ++roadId; }
        }
    return m;
}

static Fleet ambulances(int n) {
    Fleet f;
    for (int i = 0; i < n; ++i)
        f.push_back(createVehicle(VehicleType::Ambulance, i + 1, 0, (i * 23) % NODES, 60.0));
    return f;
}

int main() {
    // 1. Corner-to-corner route on a 500-node grid
    {
        CityMap city = makeGrid(); RoutingEngine eng(city);
        auto t0 = Clock::now();
        Route r = eng.shortestPath(0, NODES - 1);
        double ms = msSince(t0);
        CHECK(r.nodes().front() == 0 && r.nodes().back() == NODES - 1);
        CHECK(r.nodes().size() >= 44);        // at least 43 steps across the grid
        CHECK(r.totalMinutes() >= 86.0);      // every road takes at least 2 minutes
        CHECK(ms < 1000.0);
        std::cout << "corner-to-corner route: " << ms << " ms\n";
    }
    // 2. 100 routes between spread-out nodes
    {
        CityMap city = makeGrid(); RoutingEngine eng(city);
        auto t0 = Clock::now();
        int done = 0;
        for (int i = 0; i < 100; ++i) {
            int from = (i * 37) % NODES, to = (i * 91 + 7) % NODES;
            if (from == to) continue;
            Route r = eng.shortestPath(from, to);
            if (!r.nodes().empty()) ++done;
        }
        double total = msSince(t0);
        CHECK(done >= 99);
        CHECK(total / 100.0 < 1000.0);
        std::cout << "100 routes: " << total << " ms total, " << total / 100.0 << " ms average\n";
    }
    // 3. One dispatch decision with 20 candidate ambulances (20 routes computed)
    {
        CityMap city = makeGrid(); RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet = ambulances(20);
        auto e = createEmergency(EmergencyType::Medical, 1, NODES - 1, Priority::High, 0.0, "");
        auto t0 = Clock::now();
        DispatchResult r = de.dispatch(*e, fleet, 0.0);
        double ms = msSince(t0);
        CHECK(r.fullyAllocated() && r.records.size() == 1);
        CHECK(ms < 1000.0);
        std::cout << "dispatch with 20 candidates: " << ms << " ms\n";
    }
    // 4. Whole simulation: 20 incidents on the large city, 30 ticks
    {
        CityMap city = makeGrid();
        SimulationEngine sim(city, ambulances(20));
        auto t0 = Clock::now();
        for (int i = 0; i < 20; ++i)
            sim.reportIncident(EmergencyType::Medical, (i * 37 + 11) % NODES, Priority::High, "load test");
        sim.runTicks(30);
        double ms = msSince(t0);
        CHECK(sim.stats().created == 20);
        std::cout << "20 incidents + 30 ticks: " << ms << " ms\n";
    }

    if (failures == 0) std::cout << "performance tests passed\n";
    return failures == 0 ? 0 : 1;
}