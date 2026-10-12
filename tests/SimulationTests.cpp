#include <cmath>
#include <cstdio>
#include <fstream>
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

static CityMap makeCity() {
    CityMap m;
    for (int i = 0; i <= 5; ++i) m.addNode(CityNode(i, "N" + std::to_string(i), i * 10.0, 0));
    m.addRoad(Road(1, 0, 1, 2.0, 4.0));
    m.addRoad(Road(2, 1, 2, 3.0, 6.0));
    m.addRoad(Road(3, 1, 3, 2.0, 4.0));
    m.addRoad(Road(4, 3, 2, 1.0, 2.0));
    m.addRoad(Road(5, 0, 4, 3.0, 6.0));
    m.addRoad(Road(6, 4, 3, 2.0, 5.0));
    m.addRoad(Road(7, 5, 1, 2.0, 3.0));
    return m;
}

static Fleet oneAmbulance() {
    Fleet f;
    f.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 40.0));
    return f;
}

static Fleet fullFleet() {
    Fleet f;
    f.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 40.0));
    f.push_back(createVehicle(VehicleType::Ambulance, 2, 0, 5, 40.0));
    f.push_back(createVehicle(VehicleType::FireTruck, 3, 1, 4, 35.0));
    f.push_back(createVehicle(VehicleType::Police, 4, 2, 5, 50.0));
    return f;
}

// Speed-aware timing: real minutes = road minutes * 30 / vehicle speed.
// Ambulance 40 km/h -> factor 0.75, fire truck 35 km/h -> 0.857.
int main() {
    // A. Full lifecycle of one incident (route 0-1-2 = 10 road min = 7.5 min at 40 km/h)
    {
        CityMap city = makeCity();
        SimulationEngine sim(city, oneAmbulance());
        EmergencyId id = sim.reportIncident(EmergencyType::Medical, 2, Priority::High, "test");
        CHECK(id == 1);
        CHECK(sim.incident(id).status() == EmergencyStatus::Dispatched);
        CHECK(sim.vehicle(1).status() == VehicleStatus::EnRoute);

        sim.runTicks(2);
        Point p = sim.vehiclePosition(1);
        CHECK(nearEq(p.x, 20.0 / 3.0) && nearEq(p.y, 0.0));  // 2 min = 2.667 road min of the 4-min road

        sim.runTicks(5);  // t = 7
        CHECK(sim.incident(id).status() == EmergencyStatus::Dispatched);
        sim.step();       // t = 8
        CHECK(nearEq(sim.time(), 8.0));
        CHECK(sim.incident(id).status() == EmergencyStatus::OnScene);
        auto recs = sim.dispatchRecords();
        CHECK(recs.size() == 1 && recs[0].hasArrived() && nearEq(recs[0].responseTime(), 7.5));
        CHECK(sim.stats().responseCount == 1 && nearEq(sim.stats().avgResponse, 7.5));

        sim.resolveIncident(id);
        CHECK(sim.incident(id).status() == EmergencyStatus::Resolved);
        CHECK(sim.vehicle(1).status() == VehicleStatus::Returning);
        sim.runTicks(7);  // t = 15, the return trip takes 7.5 min
        CHECK(sim.vehicle(1).status() == VehicleStatus::Returning);
        sim.step();       // t = 16
        CHECK(sim.vehicle(1).isAvailable() && sim.vehicle(1).currentNode() == 0);

        StatsSnapshot s = sim.stats();
        CHECK(s.created == 1 && s.resolved == 1 && s.active == 0 && s.pending == 0);
        CHECK(nearEq(s.utilization.at(1), 0.5));  // 8 busy ticks of 16
        CHECK(!sim.log().filterByType("Arrived").empty());
    }
    // B. Pending incident is served once the vehicle returns (auto-resolve 2 min)
    {
        CityMap city = makeCity();
        SimulationConfig cfg; cfg.tickMinutes = 1.0; cfg.autoResolveMinutes = 2.0;
        SimulationEngine sim(city, oneAmbulance(), cfg);
        EmergencyId a = sim.reportIncident(EmergencyType::Medical, 1, Priority::High, "");
        EmergencyId b = sim.reportIncident(EmergencyType::Medical, 1, Priority::High, "");
        CHECK(sim.pendingIncidents() == std::vector<EmergencyId>{b});
        CHECK(sim.explanationFor(b).find("unavailable") != std::string::npos);

        sim.runTicks(4);  // t = 4, arrived at t = 3 (4 road min * 0.75)
        CHECK(sim.incident(a).status() == EmergencyStatus::OnScene);
        sim.runTicks(2);  // t = 6, auto-resolved at t = 5
        CHECK(sim.incident(a).status() == EmergencyStatus::Resolved);
        sim.runTicks(1);  // t = 7, the 3-minute return ends at t = 8
        CHECK(sim.incident(b).status() == EmergencyStatus::Pending);
        sim.step();       // t = 8
        CHECK(sim.incident(b).status() == EmergencyStatus::Dispatched);
        CHECK(nearEq(sim.dispatchRecords().back().dispatchedAt(), 8.0));

        StatsSnapshot s = sim.stats();
        CHECK(s.created == 2 && s.resolved == 1 && s.active == 1 && s.pending == 0);
        CHECK(nearEq(s.avgWaiting, 4.0) && nearEq(s.avgResolution, 5.0));
        CHECK(s.responseCount == 1 && nearEq(s.avgResponse, 3.0));
    }
    // C. Multi-vehicle critical fire
    {
        CityMap city = makeCity();
        SimulationEngine sim(city, fullFleet());
        EmergencyId id = sim.reportIncident(EmergencyType::Fire, 3, Priority::Critical, "mall fire");
        CHECK(sim.dispatchRecords().size() == 2);
        sim.runTicks(5);
        CHECK(sim.incident(id).status() == EmergencyStatus::OnScene);
        CHECK(throwsType<InvalidTransitionException>([&] { sim.resolveIncident(id); }));
        sim.runTicks(2);
        sim.resolveIncident(id);
        CHECK(sim.vehicle(3).status() == VehicleStatus::Returning);
        CHECK(sim.vehicle(2).status() == VehicleStatus::Returning);
        StatsSnapshot s = sim.stats();
        CHECK(s.responseCount == 2);
        const double truck = 5.0 * 30.0 / 35.0;  // fire truck 3: 5 road min at 35 km/h
        const double amb = 7.0 * 30.0 / 40.0;    // ambulance 2: 7 road min at 40 km/h
        CHECK(nearEq(s.avgResponse, (truck + amb) / 2.0) && nearEq(s.minResponse, truck) && nearEq(s.maxResponse, amb));
    }
    // D. Priority change while queued
    {
        CityMap city = makeCity();
        SimulationEngine sim(city, oneAmbulance());
        sim.reportIncident(EmergencyType::Medical, 2, Priority::High, "");
        EmergencyId low = sim.reportIncident(EmergencyType::Medical, 1, Priority::Low, "");
        EmergencyId med = sim.reportIncident(EmergencyType::Medical, 1, Priority::Medium, "");
        CHECK((sim.pendingIncidents() == std::vector<EmergencyId>{med, low}));
        sim.changeIncidentPriority(low, Priority::Critical);
        CHECK((sim.pendingIncidents() == std::vector<EmergencyId>{low, med}));
        CHECK(throwsType<ValidationException>([&] { sim.changeIncidentPriority(999, Priority::Low); }));
    }
    // E. Validation, road events, duplicate vehicle ids, bad tick
    {
        CityMap city = makeCity();
        SimulationEngine sim(city, fullFleet());
        CHECK(throwsType<ValidationException>([&] { sim.reportIncident(EmergencyType::Fire, 99, Priority::Low, ""); }));
        EmergencyId id = sim.reportIncident(EmergencyType::Medical, 2, Priority::Low, "");
        CHECK(throwsType<InvalidTransitionException>([&] { sim.resolveIncident(id); }));
        CHECK(throwsType<ValidationException>([&] { sim.resolveIncident(999); }));
        sim.setBlocked(7, true);
        CHECK(sim.log().filterByType("Blocked").size() == 1);
        CHECK(throwsType<ValidationException>([&] { sim.setBlocked(99, true); }));

        Fleet dup;
        dup.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 40.0));
        dup.push_back(createVehicle(VehicleType::Police, 1, 0, 0, 40.0));
        CityMap c2 = makeCity();
        CHECK(throwsType<ValidationException>([&] { SimulationEngine s2(c2, std::move(dup)); }));

        SimulationConfig bad; bad.tickMinutes = 0.0;
        CityMap c3 = makeCity();
        CHECK(throwsType<ValidationException>([&] { SimulationEngine s3(c3, oneAmbulance(), bad); }));
    }
    // F. Blocked road affects later dispatch
    {
        CityMap city = makeCity();
        SimulationEngine sim(city, fullFleet());
        sim.setBlocked(7, true);
        sim.reportIncident(EmergencyType::Medical, 2, Priority::High, "");
        CHECK(sim.dispatchRecords()[0].vehicleId() == 1);
    }
    // G. Smaller tick still arrives exactly at the ETA (7.5 min)
    {
        CityMap city = makeCity();
        SimulationConfig cfg; cfg.tickMinutes = 0.5;
        SimulationEngine sim(city, oneAmbulance(), cfg);
        EmergencyId id = sim.reportIncident(EmergencyType::Medical, 2, Priority::High, "");
        sim.runTicks(14);  // t = 7.0
        CHECK(sim.incident(id).status() == EmergencyStatus::Dispatched);
        sim.step();        // t = 7.5
        CHECK(sim.incident(id).status() == EmergencyStatus::OnScene && nearEq(sim.time(), 7.5));
        CHECK(nearEq(sim.dispatchRecords()[0].responseTime(), 7.5));
    }
    // H. Log export (file handling)
    {
        CityMap city = makeCity();
        SimulationEngine sim(city, oneAmbulance());
        sim.reportIncident(EmergencyType::Medical, 2, Priority::High, "export test");
        sim.log().exportToFile("ers_test_log.txt");
        std::ifstream in("ers_test_log.txt");
        std::string line;
        CHECK(static_cast<bool>(std::getline(in, line)) && !line.empty());
        in.close();
        std::remove("ers_test_log.txt");
        CHECK(throwsType<ErsException>([&] { sim.log().exportToFile("no_such_dir/x/log.txt"); }));
    }

    if (failures == 0) std::cout << "simulation tests passed\n";
    return failures == 0 ? 0 : 1;
}