#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#include "ers/common/Strings.h"
#include "ers/common/Version.h"
#include "ers/models/CityMap.h"
#include "ers/models/EmergencyFactory.h"
#include "ers/models/VehicleFactory.h"
#include "ers/services/DispatchEngine.h"
#include "ers/services/PendingQueue.h"
#include "ers/services/RoutingEngine.h"

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
    std::cout << std::fixed << std::setprecision(1);

    CityMap city = buildCity();
    RoutingEngine routing(city);
    DispatchEngine dispatcher(routing);

    std::vector<std::unique_ptr<Vehicle>> fleet;
    fleet.push_back(createVehicle(VehicleType::Ambulance, 1, 0, 0, 40.0));
    fleet.push_back(createVehicle(VehicleType::Ambulance, 2, 0, 5, 40.0));
    fleet.push_back(createVehicle(VehicleType::FireTruck, 3, 1, 4, 35.0));
    fleet.push_back(createVehicle(VehicleType::Police, 4, 2, 5, 50.0));

    std::unordered_map<EmergencyId, std::unique_ptr<Emergency>> incidents;
    PendingQueue queue;
    auto report = [&](EmergencyType t, EmergencyId id, NodeId node, Priority p, double time,
                      const std::string& desc) {
        auto e = createEmergency(t, id, node, p, time, desc);
        queue.push(*e);
        std::cout << "Reported #" << id << " " << toString(t) << " (" << toString(p) << ") at "
                  << city.node(node).name() << "\n";
        incidents[id] = std::move(e);
    };

    report(EmergencyType::Medical, 1, 2, Priority::High, 0.0, "collapsed pedestrian");
    report(EmergencyType::Fire, 2, 3, Priority::Critical, 1.0, "mall fire");
    report(EmergencyType::Crime, 3, 3, Priority::Low, 2.0, "shoplifting");
    report(EmergencyType::Medical, 4, 1, Priority::Medium, 3.0, "minor injury");

    std::cout << "\n--- Dispatching in priority order ---\n";
    while (auto next = queue.popNext()) {
        Emergency& e = *incidents.at(*next);
        std::cout << "\nIncident #" << e.id() << " " << toString(e.type()) << " ["
                  << toString(e.priority()) << "]\n";
        DispatchResult r = dispatcher.dispatch(e, fleet, e.createdAt());
        std::cout << r.explanation;
        for (const auto& rec : r.records) {
            std::cout << "  Route: ";
            const auto& nodes = rec.route().nodes();
            for (std::size_t i = 0; i < nodes.size(); ++i)
                std::cout << (i ? " -> " : "") << city.node(nodes[i]).name();
            std::cout << "\n  ETA: " << rec.etaMinutes() << " min (simulated estimate)\n";
        }
    }

    std::cout << "\n--- Final state ---\n";
    for (const auto& v : fleet)
        std::cout << toString(v->type()) << " #" << v->id() << ": " << toString(v->status()) << "\n";
    for (EmergencyId id = 1; id <= 4; ++id)
        std::cout << "Incident #" << id << ": " << toString(incidents.at(id)->status()) << "\n";
    return 0;
}