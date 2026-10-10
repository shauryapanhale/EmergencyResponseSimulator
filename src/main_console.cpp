#include <iomanip>
#include <iostream>
#include <memory>
#include "ers/common/Strings.h"
#include "ers/common/Version.h"
#include "ers/models/CityMap.h"
#include "ers/models/VehicleFactory.h"
#include "ers/services/SimulationEngine.h"

using namespace ers;

static CityMap buildCity() {
    CityMap m;
    m.addNode(CityNode(0, "Central Hospital", 0, 0, NodeKind::Station));
    m.addNode(CityNode(1, "Market Square", 20, 0));
    m.addNode(CityNode(2, "Riverside Park", 40, 10, NodeKind::Landmark));
    m.addNode(CityNode(3, "Mall Junction", 30, 20));
    m.addNode(CityNode(4, "Fire Station", 0, 25, NodeKind::Station));
    m.addNode(CityNode(5, "Police HQ", 20, -20, NodeKind::Station));
    m.addRoad(Road(1, 0, 1, 2.0, 4.0));
    m.addRoad(Road(2, 1, 2, 3.0, 6.0));
    m.addRoad(Road(3, 1, 3, 2.0, 4.0));
    m.addRoad(Road(4, 3, 2, 1.0, 2.0));
    m.addRoad(Road(5, 0, 4, 3.0, 6.0));
    m.addRoad(Road(6, 4, 3, 2.0, 5.0));
    m.addRoad(Road(7, 5, 1, 2.0, 3.0));
    return m;
}

int main() {
    std::cout << ers::projectName() << "\n" << ers::disclaimer() << "\n\n";
    std::cout << std::fixed << std::setprecision(2);

    CityMap city = buildCity();
    std::vector<std::unique_ptr<Vehicle>> fleet;
    fleet.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 40.0));
    fleet.push_back(createVehicle(VehicleType::Ambulance, 2, 0, 5, 40.0));
    fleet.push_back(createVehicle(VehicleType::FireTruck, 3, 1, 4, 35.0));
    fleet.push_back(createVehicle(VehicleType::Police, 4, 2, 5, 50.0));

    SimulationConfig cfg;
    cfg.tickMinutes = 1.0;
    cfg.autoResolveMinutes = 3.0;
    SimulationEngine sim(city, std::move(fleet), cfg);

    sim.start();
    sim.reportIncident(EmergencyType::Medical, 2, Priority::High, "collapsed pedestrian");
    sim.reportIncident(EmergencyType::Fire, 3, Priority::Critical, "mall fire");
    sim.reportIncident(EmergencyType::Crime, 3, Priority::Low, "shoplifting");
    sim.reportIncident(EmergencyType::Medical, 1, Priority::Medium, "minor injury");
    sim.runTicks(40);

    std::cout << "--- Event log ---\n";
    for (const auto& e : sim.log().entries()) std::cout << EventLog::format(e) << "\n";

    StatsSnapshot s = sim.stats();
    std::cout << "\n--- Statistics at t=" << sim.time() << " min ---\n";
    std::cout << "Incidents created/resolved/active/pending: " << s.created << "/" << s.resolved << "/"
              << s.active << "/" << s.pending << "\n";
    std::cout << "Response time (min): avg " << s.avgResponse << ", min " << s.minResponse
              << ", max " << s.maxResponse << " over " << s.responseCount << " arrivals\n";
    std::cout << "Average waiting before first dispatch: " << s.avgWaiting << " min\n";
    for (const auto& kv : s.utilization)
        std::cout << "Vehicle #" << kv.first << " utilization: " << kv.second * 100.0 << "%\n";
    return 0;
}