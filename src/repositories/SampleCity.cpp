#include "ers/repositories/SampleCity.h"
#include "ers/models/VehicleFactory.h"

namespace ers {

namespace {

struct NodeSpec { int id; const char* name; double x; double y; NodeKind kind; };
struct RoadSpec { int id; int from; int to; double km; double minutes; };
struct StationSpec { int id; const char* name; int node; StationType type; };
struct VehicleSpec { int id; VehicleType type; int station; int node; double speed; };

const NodeSpec kNodes[] = {
    {1,  "Central Ambulance Base", 1.0, 1.0, NodeKind::Station},
    {2,  "North Fire Station",     8.0, 1.0, NodeKind::Station},
    {3,  "Police HQ",              4.0, 6.0, NodeKind::Station},
    {4,  "City Hospital",          4.0, 2.0, NodeKind::Landmark},
    {5,  "Market Square",          2.0, 3.0, NodeKind::Intersection},
    {6,  "Railway Station",        6.0, 3.0, NodeKind::Landmark},
    {7,  "Old Town",               1.0, 5.0, NodeKind::Intersection},
    {8,  "Riverside",              7.0, 5.0, NodeKind::Intersection},
    {9,  "Tech Park",              9.0, 4.0, NodeKind::Landmark},
    {10, "College Campus",         3.0, 8.0, NodeKind::Landmark},
    {11, "Ring Road East",        10.0, 7.0, NodeKind::Intersection},
    {12, "Stadium",                6.0, 8.0, NodeKind::Landmark},
};

const RoadSpec kRoads[] = {
    {1, 1, 5, 2.0, 4.0},  {2, 1, 4, 3.0, 5.0},  {3, 4, 5, 2.0, 3.0},
    {4, 4, 6, 2.5, 4.0},  {5, 2, 6, 3.0, 5.0},  {6, 2, 9, 3.5, 6.0},
    {7, 6, 9, 3.0, 5.0},  {8, 5, 7, 2.5, 4.0},  {9, 7, 3, 3.0, 5.0},
    {10, 3, 8, 3.5, 6.0}, {11, 6, 8, 2.5, 4.0}, {12, 8, 9, 2.0, 3.0},
    {13, 3, 10, 2.5, 4.0}, {14, 3, 12, 3.0, 5.0}, {15, 8, 12, 3.5, 6.0},
    {16, 9, 11, 4.0, 7.0}, {17, 12, 11, 5.0, 8.0}, {18, 5, 3, 4.0, 7.0},
};

const StationSpec kStations[] = {
    {1, "Central Ambulance Base", 1, StationType::AmbulanceBase},
    {2, "North Fire Station",     2, StationType::FireStation},
    {3, "Police HQ",              3, StationType::PoliceStation},
};

const VehicleSpec kVehicles[] = {
    {1, VehicleType::Ambulance, 1, 1, 60.0},
    {2, VehicleType::Ambulance, 1, 1, 60.0},
    {3, VehicleType::FireTruck, 2, 2, 45.0},
    {4, VehicleType::FireTruck, 2, 2, 45.0},
    {5, VehicleType::Police,    3, 3, 70.0},
    {6, VehicleType::Police,    3, 3, 70.0},
};

} // namespace

CityData buildSampleCity() {
    CityData data;
    for (const NodeSpec& n : kNodes) data.map.addNode(CityNode(n.id, n.name, n.x, n.y, n.kind));
    for (const RoadSpec& r : kRoads) data.map.addRoad(Road(r.id, r.from, r.to, r.km, r.minutes));
    for (const StationSpec& s : kStations) data.stations.emplace_back(s.id, s.name, s.node, s.type);
    for (const VehicleSpec& v : kVehicles) {
        for (Station& s : data.stations) {
            if (s.id() == v.station) s.addVehicle(v.id);
        }
    }
    return data;
}

std::vector<std::unique_ptr<Vehicle>> buildSampleFleet() {
    std::vector<std::unique_ptr<Vehicle>> fleet;
    for (const VehicleSpec& v : kVehicles) {
        fleet.push_back(createVehicle(v.type, v.id, v.station, v.node, v.speed));
    }
    return fleet;
}

bool seedSampleCity(Database& db) {
    DatabaseRepository repo(db);
    if (!repo.loadCity().map.nodes().empty()) return false;
    CityData city = buildSampleCity();
    repo.saveCity(city.map, city.stations);
    repo.saveFleet(buildSampleFleet());
    return true;
}

} // namespace ers