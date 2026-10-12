#include <iostream>
#include <string>
#include <vector>
#include "ers/repositories/SampleCity.h"
#include "ers/services/Scenario.h"
#include "ers/services/SimulationEngine.h"

using namespace ers;

static int failures = 0;
static void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
}
#define CHECK(cond) check((cond), std::string(#cond) + " (line " + std::to_string(__LINE__) + ")")

static bool busy(VehicleStatus s) {
    return s == VehicleStatus::EnRoute || s == VehicleStatus::OnScene;
}

// Consistency between vehicles, dispatch records and incidents (sample fleet ids 1..6).
static void checkInvariants(const SimulationEngine& sim, const std::string& label) {
    const auto recs = sim.dispatchRecords();
    const std::string at = label + " t=" + std::to_string(sim.time());

    for (int v = 1; v <= 6; ++v) {
        int active = 0;
        EmergencyId incident = -1;
        bool arrived = false;
        for (const auto& r : recs) {
            if (r.vehicleId() != v || r.hasCompleted()) continue;
            ++active;
            incident = r.emergencyId();
            arrived = r.hasArrived();
        }
        const Vehicle& veh = sim.vehicle(v);
        check(active <= 1, at + ": vehicle " + std::to_string(v) + " has more than one active dispatch");
        if (busy(veh.status())) {
            check(active == 1, at + ": busy vehicle " + std::to_string(v) + " has no active dispatch");
            if (active == 1) {
                check(veh.assignedIncident().has_value() && *veh.assignedIncident() == incident,
                      at + ": vehicle " + std::to_string(v) + " assignedIncident differs from its dispatch");
                check(arrived ? veh.status() == VehicleStatus::OnScene : veh.status() == VehicleStatus::EnRoute,
                      at + ": vehicle " + std::to_string(v) + " status does not match arrival state");
            }
        } else {
            check(active == 0, at + ": idle vehicle " + std::to_string(v) + " still has an active dispatch");
        }
    }
    for (const auto& r : recs)
        if (sim.incident(r.emergencyId()).status() == EmergencyStatus::Resolved)
            check(r.hasCompleted(), at + ": resolved incident " + std::to_string(r.emergencyId()) + " has an open dispatch");
}

int main() {
    // 1. Every prepared scenario, tick by tick, keeps vehicles and dispatches consistent
    for (const Scenario& sc : sampleScenarios()) {
        CityData city = buildSampleCity();
        SimulationConfig cfg;
        cfg.autoResolveMinutes = 3.0;
        SimulationEngine sim(city.map, buildSampleFleet(), cfg);
        ScenarioPlayer player(sim, sc);

        player.applyDueEvents();
        checkInvariants(sim, sc.name);
        int guard = 0;
        while (!player.finished() && guard++ < 1000) {
            player.step();
            checkInvariants(sim, sc.name);
        }
        CHECK(player.finished());
        CHECK(sim.stats().created == static_cast<int>(sc.incidents.size()));
    }

    // 2. Priority and reservation: the freed ambulance goes to the Critical fire, not the older Medium incident
    {
        CityData city = buildSampleCity();
        SimulationEngine sim(city.map, buildSampleFleet());
        EmergencyId a = sim.reportIncident(EmergencyType::Medical, 7, Priority::Low, "first ambulance");
        EmergencyId b = sim.reportIncident(EmergencyType::Medical, 10, Priority::Low, "second ambulance");
        EmergencyId medium = sim.reportIncident(EmergencyType::Medical, 9, Priority::Medium, "waits for an ambulance");
        EmergencyId fire = sim.reportIncident(EmergencyType::Fire, 8, Priority::Critical, "needs fire truck and ambulance");

        CHECK(sim.incident(medium).status() == EmergencyStatus::Pending);
        CHECK(sim.incident(fire).status() == EmergencyStatus::Dispatched);
        CHECK(!sim.unmetResources(fire).empty());
        checkInvariants(sim, "priority");

        for (int i = 0; i < 4; ++i) { sim.step(); checkInvariants(sim, "priority"); }
        CHECK(sim.incident(a).status() == EmergencyStatus::OnScene);
        sim.resolveIncident(a);
        for (int i = 0; i < 3; ++i) { sim.step(); checkInvariants(sim, "priority"); }
        CHECK(sim.vehicle(1).status() == VehicleStatus::Returning);
        CHECK(!sim.unmetResources(fire).empty());

        sim.step();  // t = 8: ambulance 1 is back at base
        checkInvariants(sim, "priority");
        CHECK(sim.unmetResources(fire).empty());
        CHECK(sim.vehicle(1).status() == VehicleStatus::EnRoute);
        CHECK(sim.vehicle(1).assignedIncident().has_value() && *sim.vehicle(1).assignedIncident() == fire);
        CHECK(sim.incident(medium).status() == EmergencyStatus::Pending);
        CHECK(sim.incident(b).status() != EmergencyStatus::Pending);
    }

    if (failures == 0) std::cout << "integration tests passed\n";
    return failures == 0 ? 0 : 1;
}