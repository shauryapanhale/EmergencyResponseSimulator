#include "ers/repositories/SampleCity.h"
#include "ers/services/SimulationEngine.h"
#include <iostream>
#include <string>

static int g_failures = 0;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        std::cerr << "CHECK failed: " #cond " (line " << __LINE__ << ")\n"; } } while (0)

using namespace ers;

static int countFor(const SimulationEngine& sim, EmergencyId id) {
    int n = 0;
    for (const auto& r : sim.dispatchRecords()) if (r.emergencyId() == id) ++n;
    return n;
}

static bool hasAmbulanceFor(const SimulationEngine& sim, EmergencyId id) {
    for (const auto& r : sim.dispatchRecords())
        if (r.emergencyId() == id && sim.vehicle(r.vehicleId()).type() == VehicleType::Ambulance)
            return true;
    return false;
}

int main() {
    // 1. Critical fire is missing an ambulance; the Critical incident gets the first free
    //    ambulance before an older Medium incident that is still pending.
    {
        CityData city = buildSampleCity();
        SimulationEngine sim(city.map, buildSampleFleet());  // manual resolution
        sim.reportIncident(EmergencyType::Medical, 7, Priority::High, "m1");     // id 1
        sim.reportIncident(EmergencyType::Medical, 7, Priority::High, "m2");     // id 2
        sim.reportIncident(EmergencyType::Medical, 10, Priority::Medium, "m3");  // id 3 pending
        sim.reportIncident(EmergencyType::Fire, 9, Priority::Critical, "fire"); // id 4

        CHECK(sim.incident(3).status() == EmergencyStatus::Pending);
        CHECK(sim.incident(4).status() == EmergencyStatus::Dispatched);
        auto unmet = sim.unmetResources(4);
        CHECK(unmet.size() == 1 && unmet[0] == VehicleType::Ambulance);
        CHECK(countFor(sim, 4) == 1);

        sim.runTicks(10);
        sim.resolveIncident(1);
        sim.runTicks(15);

        CHECK(sim.unmetResources(4).empty());
        CHECK(countFor(sim, 4) == 2);
        CHECK(hasAmbulanceFor(sim, 4));
        CHECK(sim.incident(3).status() == EmergencyStatus::Pending);

        sim.resolveIncident(2);
        sim.runTicks(15);
        CHECK(sim.incident(3).status() != EmergencyStatus::Pending);
    }

    // 2. A resolved incident drops its unmet types and never gets late vehicles.
    {
        CityData city = buildSampleCity();
        SimulationEngine sim(city.map, buildSampleFleet());
        sim.reportIncident(EmergencyType::Medical, 7, Priority::High, "m1");
        sim.reportIncident(EmergencyType::Medical, 7, Priority::High, "m2");
        sim.reportIncident(EmergencyType::Fire, 9, Priority::Critical, "fire");  // id 3
        CHECK(!sim.unmetResources(3).empty());

        sim.runTicks(8);
        sim.resolveIncident(3);
        sim.runTicks(1);
        CHECK(sim.unmetResources(3).empty());
        CHECK(countFor(sim, 3) == 1);
        CHECK(!sim.log().filterByType("UnmetDropped").empty());
    }

    if (g_failures == 0) std::cout << "SupplementTests: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}