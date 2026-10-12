#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ers/models/EmergencyFactory.h"
#include "ers/models/VehicleFactory.h"
#include "ers/services/DispatchEngine.h"
#include "ers/services/SimulationEngine.h"

using namespace ers;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; ++failures; } } while (0)
static bool nearEq(double a, double b) { return std::fabs(a - b) < 1e-9; }

using Fleet = std::vector<std::unique_ptr<Vehicle>>;

// Nodes 0, 1, 2 on a line; road 1 joins 0-1 (6 min), road 2 joins 2-1 (10 min).
static CityMap makeCity() {
    CityMap m;
    for (int i = 0; i <= 2; ++i) m.addNode(CityNode(i, "N" + std::to_string(i), i * 10.0, 0));
    m.addRoad(Road(1, 0, 1, 2.0, 6.0));
    m.addRoad(Road(2, 2, 1, 3.0, 10.0));
    return m;
}

int main() {
    // 1. ETA = route minutes * reference speed / vehicle speed; the route itself is unchanged
    {
        const double speeds[] = {60.0, 30.0, 15.0};
        const double expected[] = {3.0, 6.0, 12.0};
        for (int i = 0; i < 3; ++i) {
            CityMap city = makeCity(); RoutingEngine eng(city); DispatchEngine de(eng);
            Fleet fleet;
            fleet.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, speeds[i]));
            auto e = createEmergency(EmergencyType::Medical, 1, 1, Priority::High, 0.0, "");
            DispatchResult r = de.dispatch(*e, fleet, 0.0);
            CHECK(r.records.size() == 1);
            CHECK(nearEq(r.records[0].route().totalMinutes(), 6.0));
            CHECK(nearEq(r.records[0].etaMinutes(), expected[i]));
        }
        CHECK(nearEq(createVehicle(VehicleType::Ambulance, 1, 0, 0, 60.0)->timeFactor(), 0.5));
    }
    // 2. A faster vehicle on a longer route beats a slower vehicle on a shorter route
    {
        CityMap city = makeCity(); RoutingEngine eng(city); DispatchEngine de(eng);
        Fleet fleet;
        fleet.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 30.0));  // 6 min route, ETA 6
        fleet.push_back(createVehicle(VehicleType::Ambulance, 2, 0, 2, 60.0));  // 10 min route, ETA 5
        auto e = createEmergency(EmergencyType::Medical, 1, 1, Priority::High, 0.0, "");
        DispatchResult r = de.dispatch(*e, fleet, 0.0);
        CHECK(r.records.size() == 1 && r.records[0].vehicleId() == 2);
        CHECK(nearEq(r.records[0].route().totalMinutes(), 10.0));
        CHECK(nearEq(r.records[0].etaMinutes(), 5.0));
    }
    // 3. Simulated arrival time equals the stored ETA, and the return trip uses the same factor
    {
        CityMap city = makeCity();
        Fleet fleet;
        fleet.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 60.0));
        SimulationEngine sim(city, std::move(fleet));
        EmergencyId id = sim.reportIncident(EmergencyType::Medical, 1, Priority::High, "");
        CHECK(nearEq(sim.dispatchRecords()[0].etaMinutes(), 3.0));
        sim.runTicks(1);
        CHECK(nearEq(sim.vehiclePosition(1).x, 10.0 / 3.0));  // 1 min = 2 of 6 road minutes
        sim.runTicks(1);
        CHECK(sim.incident(id).status() == EmergencyStatus::Dispatched);
        sim.step();  // t = 3
        CHECK(sim.incident(id).status() == EmergencyStatus::OnScene);
        CHECK(nearEq(sim.dispatchRecords()[0].responseTime(), sim.dispatchRecords()[0].etaMinutes()));
        sim.resolveIncident(id);
        sim.runTicks(2);  // t = 5
        CHECK(sim.vehicle(1).status() == VehicleStatus::Returning);
        sim.step();       // t = 6
        CHECK(sim.vehicle(1).isAvailable() && sim.vehicle(1).currentNode() == 0);
    }
    // 4. A slow vehicle (15 km/h) takes twice the road minutes
    {
        CityMap city = makeCity();
        Fleet fleet;
        fleet.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 15.0));
        SimulationEngine sim(city, std::move(fleet));
        EmergencyId id = sim.reportIncident(EmergencyType::Medical, 1, Priority::High, "");
        sim.runTicks(11);
        CHECK(sim.incident(id).status() == EmergencyStatus::Dispatched);
        sim.step();  // t = 12
        CHECK(sim.incident(id).status() == EmergencyStatus::OnScene);
        CHECK(nearEq(sim.dispatchRecords()[0].responseTime(), 12.0));
    }

    if (failures == 0) std::cout << "speed eta tests passed\n";
    return failures == 0 ? 0 : 1;
}