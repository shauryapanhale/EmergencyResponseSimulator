#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ers/models/VehicleFactory.h"
#include "ers/services/SimulationEngine.h"

using namespace ers;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; ++failures; } } while (0)
static bool nearEq(double a, double b) { return std::fabs(a - b) < 1e-9; }

template <typename Ex, typename F>
static bool throwsType(F f) {
    try { f(); } catch (const Ex&) { return true; } catch (...) { return false; }
    return false;
}

using Fleet = std::vector<std::unique_ptr<Vehicle>>;

// Nodes 0, 1, 2 on a line; road 1 joins 0-1 (4 min), road 2 joins 1-2 (6 min).
static CityMap line3() {
    CityMap m;
    for (int i = 0; i < 3; ++i) m.addNode(CityNode(i, "N" + std::to_string(i), i * 10.0, 0));
    m.addRoad(Road(1, 0, 1, 2.0, 4.0));
    m.addRoad(Road(2, 1, 2, 3.0, 6.0));
    return m;
}

static Fleet oneAmbulance() {
    Fleet f;
    f.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 60.0));
    return f;
}

int main() {
    // 1. Road validation
    {
        CHECK(throwsType<ValidationException>([] { Road(1, 0, 1, -2.0, 4.0); }));
        CHECK(throwsType<ValidationException>([] { Road(1, 0, 1, 0.0, 4.0); }));
        CHECK(throwsType<ValidationException>([] { Road(1, 0, 1, 2.0, -4.0); }));
        CHECK(throwsType<ValidationException>([] { Road(1, 0, 1, 2.0, 0.0); }));
        CHECK(throwsType<ValidationException>([] { Road(1, 0, 1, 2.0, 4.0, 0.0, false, true); }));
        CHECK(throwsType<ValidationException>([] { Road(1, 3, 3, 2.0, 4.0); }));
        CHECK(throwsType<ValidationException>([] { Road(-1, 0, 1, 2.0, 4.0); }));
        Road r(1, 0, 1, 2.0, 4.0);
        CHECK(throwsType<ValidationException>([&] { r.setTrafficFactor(-1.0); }));
        CHECK(nearEq(r.trafficFactor(), 1.0));
    }
    // 2. Vehicle speed must be positive
    {
        CHECK(throwsType<ErsException>([] { createVehicle(VehicleType::Ambulance, 1, 0, 0, 0.0); }));
        CHECK(throwsType<ErsException>([] { createVehicle(VehicleType::FireTruck, 1, 0, 0, -10.0); }));
        CHECK(throwsType<ErsException>([] { createVehicle(VehicleType::Police, 1, 0, 0, -0.5); }));
    }
    // 3. City validation
    {
        CityMap m = line3();
        CHECK(throwsType<ErsException>([&] { m.addNode(CityNode(1, "duplicate", 5.0, 5.0)); }));
        CHECK(throwsType<ErsException>([&] { m.addRoad(Road(9, 0, 99, 1.0, 1.0)); }));
        CHECK(throwsType<ErsException>([&] { m.setTraffic(99, 1.0); }));
        CHECK(throwsType<ErsException>([&] { m.setBlocked(99, true); }));
    }
    // 4. Pending incident behind a blocked road is dispatched once the road reopens
    {
        CityMap city = line3();
        SimulationEngine sim(city, oneAmbulance());
        sim.setBlocked(2, true);
        EmergencyId id = sim.reportIncident(EmergencyType::Medical, 2, Priority::High, "");
        CHECK(sim.incident(id).status() == EmergencyStatus::Pending);
        CHECK(sim.explanationFor(id).find("unreachable") != std::string::npos);
        sim.runTicks(3);
        CHECK(sim.incident(id).status() == EmergencyStatus::Pending);
        CHECK(sim.vehicle(1).isAvailable());

        sim.setBlocked(2, false);
        sim.step();
        CHECK(sim.incident(id).status() == EmergencyStatus::Dispatched);
        CHECK(sim.vehicle(1).status() == VehicleStatus::EnRoute);
    }
    // 5. A vehicle already travelling keeps its route when a road ahead is blocked (SRS 10.5)
    {
        CityMap city = line3();
        SimulationEngine sim(city, oneAmbulance());
        EmergencyId id = sim.reportIncident(EmergencyType::Medical, 2, Priority::High, "");
        CHECK(nearEq(sim.dispatchRecords()[0].etaMinutes(), 5.0));  // 10 road min at 60 km/h
        sim.runTicks(2);
        sim.setBlocked(2, true);
        sim.runTicks(2);
        CHECK(sim.incident(id).status() == EmergencyStatus::Dispatched);
        sim.step();  // t = 5
        CHECK(sim.incident(id).status() == EmergencyStatus::OnScene && nearEq(sim.time(), 5.0));
    }

    if (failures == 0) std::cout << "edge case tests passed\n";
    return failures == 0 ? 0 : 1;
}