#include "ers/repositories/DatabaseRepository.h"
#include "ers/common/Exceptions.h"
#include "ers/models/VehicleFactory.h"
#include "ers/repositories/Schema.h"
#include <set>
#include <sstream>
#include <string>

namespace ers {

namespace {

std::string toText(NodeKind k) {
    switch (k) {
        case NodeKind::Intersection: return "Intersection";
        case NodeKind::Landmark: return "Landmark";
        case NodeKind::Station: return "Station";
    }
    return "Intersection";
}

NodeKind nodeKindFromText(const std::string& s) {
    if (s == "Intersection") return NodeKind::Intersection;
    if (s == "Landmark") return NodeKind::Landmark;
    if (s == "Station") return NodeKind::Station;
    throw DatabaseException("Unknown node type in database: " + s);
}

std::string toText(StationType t) {
    switch (t) {
        case StationType::AmbulanceBase: return "AmbulanceBase";
        case StationType::FireStation: return "FireStation";
        case StationType::PoliceStation: return "PoliceStation";
        case StationType::Shared: return "Shared";
    }
    return "Shared";
}

StationType stationTypeFromText(const std::string& s) {
    if (s == "AmbulanceBase") return StationType::AmbulanceBase;
    if (s == "FireStation") return StationType::FireStation;
    if (s == "PoliceStation") return StationType::PoliceStation;
    if (s == "Shared") return StationType::Shared;
    throw DatabaseException("Unknown station type in database: " + s);
}

std::string toText(VehicleType t) {
    switch (t) {
        case VehicleType::Ambulance: return "Ambulance";
        case VehicleType::FireTruck: return "FireTruck";
        case VehicleType::Police: return "Police";
    }
    return "Ambulance";
}

VehicleType vehicleTypeFromText(const std::string& s) {
    if (s == "Ambulance") return VehicleType::Ambulance;
    if (s == "FireTruck") return VehicleType::FireTruck;
    if (s == "Police") return VehicleType::Police;
    throw DatabaseException("Unknown vehicle type in database: " + s);
}

std::string toText(VehicleStatus s) {
    switch (s) {
        case VehicleStatus::Available: return "Available";
        case VehicleStatus::EnRoute: return "EnRoute";
        case VehicleStatus::OnScene: return "OnScene";
        case VehicleStatus::Returning: return "Returning";
        case VehicleStatus::OutOfService: return "OutOfService";
    }
    return "Available";
}

std::string joinEquipment(const std::set<std::string>& items) {
    std::ostringstream out;
    bool first = true;
    for (const auto& item : items) {
        if (!first) out << ';';
        out << item;
        first = false;
    }
    return out.str();
}

} // namespace

DatabaseRepository::DatabaseRepository(Database& db) : db_(db) { initSchema(db_); }

void DatabaseRepository::saveCity(const CityMap& map, const std::vector<Station>& stations) {
    Transaction tx(db_);

    Statement node(db_,
        "INSERT INTO city_node (node_id, name, x, y, node_type) VALUES (?, ?, ?, ?, ?) "
        "ON CONFLICT(node_id) DO UPDATE SET name=excluded.name, x=excluded.x, "
        "y=excluded.y, node_type=excluded.node_type;");
    for (const auto& entry : map.nodes()) {
        const CityNode& n = entry.second;
        node.bind(1, n.id());
        node.bind(2, n.name());
        node.bind(3, n.x());
        node.bind(4, n.y());
        node.bind(5, toText(n.kind()));
        node.step();
        node.reset();
    }

    Statement road(db_,
        "INSERT INTO road (road_id, from_node, to_node, distance_km, base_minutes, "
        "traffic_factor, is_blocked, is_bidirectional) VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(road_id) DO UPDATE SET from_node=excluded.from_node, "
        "to_node=excluded.to_node, distance_km=excluded.distance_km, "
        "base_minutes=excluded.base_minutes, traffic_factor=excluded.traffic_factor, "
        "is_blocked=excluded.is_blocked, is_bidirectional=excluded.is_bidirectional;");
    for (const auto& entry : map.roads()) {
        const Road& r = entry.second;
        road.bind(1, r.id());
        road.bind(2, r.from());
        road.bind(3, r.to());
        road.bind(4, r.distanceKm());
        road.bind(5, r.baseMinutes());
        road.bind(6, r.trafficFactor());
        road.bind(7, r.isBlocked() ? 1 : 0);
        road.bind(8, r.isBidirectional() ? 1 : 0);
        road.step();
        road.reset();
    }

    Statement st(db_,
        "INSERT INTO station (station_id, name, node_id, station_type) VALUES (?, ?, ?, ?) "
        "ON CONFLICT(station_id) DO UPDATE SET name=excluded.name, "
        "node_id=excluded.node_id, station_type=excluded.station_type;");
    for (const Station& s : stations) {
        st.bind(1, s.id());
        st.bind(2, s.name());
        st.bind(3, s.nodeId());
        st.bind(4, toText(s.type()));
        st.step();
        st.reset();
    }

    tx.commit();
}

CityData DatabaseRepository::loadCity() {
    CityData data;

    Statement nodes(db_, "SELECT node_id, name, x, y, node_type FROM city_node ORDER BY node_id;");
    while (nodes.step()) {
        data.map.addNode(CityNode(nodes.columnInt(0), nodes.columnText(1), nodes.columnDouble(2),
                                  nodes.columnDouble(3), nodeKindFromText(nodes.columnText(4))));
    }

    Statement roads(db_,
        "SELECT road_id, from_node, to_node, distance_km, base_minutes, traffic_factor, "
        "is_blocked, is_bidirectional FROM road ORDER BY road_id;");
    while (roads.step()) {
        data.map.addRoad(Road(roads.columnInt(0), roads.columnInt(1), roads.columnInt(2),
                              roads.columnDouble(3), roads.columnDouble(4), roads.columnDouble(5),
                              roads.columnInt(6) != 0, roads.columnInt(7) != 0));
    }

    Statement stations(db_,
        "SELECT station_id, name, node_id, station_type FROM station ORDER BY station_id;");
    while (stations.step()) {
        data.stations.emplace_back(stations.columnInt(0), stations.columnText(1),
                                   stations.columnInt(2),
                                   stationTypeFromText(stations.columnText(3)));
    }

    Statement vehicles(db_,
        "SELECT vehicle_id FROM vehicle WHERE station_id = ? ORDER BY vehicle_id;");
    for (Station& s : data.stations) {
        vehicles.bind(1, s.id());
        while (vehicles.step()) s.addVehicle(vehicles.columnInt(0));
        vehicles.reset();
    }

    return data;
}

void DatabaseRepository::saveFleet(const std::vector<std::unique_ptr<Vehicle>>& fleet) {
    Transaction tx(db_);
    Statement st(db_,
        "INSERT INTO vehicle (vehicle_id, type, station_id, current_node, speed_kmph, "
        "status, capabilities) VALUES (?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(vehicle_id) DO UPDATE SET type=excluded.type, "
        "station_id=excluded.station_id, current_node=excluded.current_node, "
        "speed_kmph=excluded.speed_kmph, status=excluded.status, "
        "capabilities=excluded.capabilities;");
    for (const auto& v : fleet) {
        st.bind(1, v->id());
        st.bind(2, toText(v->type()));
        st.bind(3, v->stationId());
        st.bind(4, v->currentNode());
        st.bind(5, v->speedKmph());
        st.bind(6, toText(v->status()));
        st.bind(7, joinEquipment(v->equipment()));
        st.step();
        st.reset();
    }
    tx.commit();
}

std::vector<std::unique_ptr<Vehicle>> DatabaseRepository::loadFleet() {
    std::vector<std::unique_ptr<Vehicle>> fleet;
    Statement q(db_,
        "SELECT vehicle_id, type, station_id, current_node, speed_kmph, status "
        "FROM vehicle ORDER BY vehicle_id;");
    while (q.step()) {
        auto v = createVehicle(vehicleTypeFromText(q.columnText(1)), q.columnInt(0),
                               q.columnInt(2), q.columnInt(3), q.columnDouble(4));
        if (q.columnText(5) == "OutOfService") v->markOutOfService();
        fleet.push_back(std::move(v));
    }
    return fleet;
}

} // namespace ers