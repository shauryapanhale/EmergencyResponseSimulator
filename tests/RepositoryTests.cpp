#include "ers/repositories/DatabaseRepository.h"
#include "ers/models/VehicleFactory.h"
#include "ers/common/Exceptions.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

static int g_failures = 0;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        std::cerr << "CHECK failed: " #cond " (line " << __LINE__ << ")\n"; } } while (0)

using namespace ers;

static CityMap buildMap() {
    CityMap m;
    m.addNode(CityNode(1, "Central Station", 0.0, 0.0, NodeKind::Station));
    m.addNode(CityNode(2, "Market", 3.0, 0.0, NodeKind::Intersection));
    m.addNode(CityNode(3, "Hospital", 3.0, 4.0, NodeKind::Landmark));
    m.addNode(CityNode(4, "Fire HQ", 6.0, 4.0, NodeKind::Station));
    m.addRoad(Road(1, 1, 2, 3.0, 4.0));
    m.addRoad(Road(2, 2, 3, 4.0, 5.0, 1.5, false, true));
    m.addRoad(Road(3, 3, 4, 3.0, 4.0, 2.0, true, false));
    return m;
}

static std::vector<Station> buildStations() {
    std::vector<Station> s;
    s.emplace_back(1, "Central Base", 1, StationType::AmbulanceBase);
    s.emplace_back(2, "Fire HQ", 4, StationType::FireStation);
    s[0].addVehicle(1);
    s[0].addVehicle(3);
    s[1].addVehicle(2);
    return s;
}

static std::vector<std::unique_ptr<Vehicle>> buildFleet() {
    std::vector<std::unique_ptr<Vehicle>> f;
    f.push_back(createVehicle(VehicleType::Ambulance, 1, 1, 1, 60.0));
    f.push_back(createVehicle(VehicleType::FireTruck, 2, 2, 4, 45.0));
    f.push_back(createVehicle(VehicleType::Police, 3, 1, 1, 70.0));
    return f;
}

int main() {
    auto p = std::filesystem::temp_directory_path() / "ers_repository_test.db";
    std::filesystem::remove(p);
    const std::string path = p.string();

    CityMap map = buildMap();
    auto stations = buildStations();
    auto fleet = buildFleet();
    fleet[2]->markOutOfService();

    {
        Database db(path);
        DatabaseRepository repo(db);
        repo.saveCity(map, stations);
        repo.saveFleet(fleet);
    }

    {
        Database db(path);
        DatabaseRepository repo(db);
        CityData city = repo.loadCity();

        CHECK(city.map.nodes().size() == 4);
        CHECK(city.map.node(3).name() == "Hospital");
        CHECK(city.map.node(3).x() == 3.0 && city.map.node(3).y() == 4.0);
        CHECK(city.map.node(3).kind() == NodeKind::Landmark);
        CHECK(city.map.node(1).kind() == NodeKind::Station);

        CHECK(city.map.roads().size() == 3);
        CHECK(city.map.road(1).trafficFactor() == 1.0);
        CHECK(city.map.road(2).trafficFactor() == 1.5);
        CHECK(city.map.road(2).currentMinutes() == 7.5);
        CHECK(!city.map.road(2).isBlocked());
        CHECK(city.map.road(3).isBlocked());
        CHECK(!city.map.road(3).isBidirectional());
        CHECK(city.map.road(1).isBidirectional());

        CHECK(city.stations.size() == 2);
        CHECK(city.stations[0].name() == "Central Base");
        CHECK(city.stations[0].type() == StationType::AmbulanceBase);
        CHECK(city.stations[0].vehicleIds() == stations[0].vehicleIds());
        CHECK(city.stations[1].vehicleIds() == stations[1].vehicleIds());

        auto loaded = repo.loadFleet();
        CHECK(loaded.size() == 3);
        CHECK(loaded[0]->type() == VehicleType::Ambulance);
        CHECK(loaded[1]->type() == VehicleType::FireTruck);
        CHECK(loaded[2]->type() == VehicleType::Police);
        CHECK(loaded[0]->stationId() == 1);
        CHECK(loaded[1]->homeNode() == 4);
        CHECK(loaded[1]->speedKmph() == 45.0);
        CHECK(loaded[0]->equipment() == fleet[0]->equipment());
        CHECK(loaded[0]->status() == VehicleStatus::Available);
        CHECK(loaded[2]->status() == VehicleStatus::OutOfService);
    }

    {
        // Upsert: saving again with a changed road updates it and creates no duplicates.
        map.setTraffic(1, 2.0);
        map.setBlocked(2, true);
        Database db(path);
        DatabaseRepository repo(db);
        repo.saveCity(map, stations);
        repo.saveFleet(fleet);
        CityData city = repo.loadCity();
        CHECK(city.map.nodes().size() == 4);
        CHECK(city.map.roads().size() == 3);
        CHECK(city.stations.size() == 2);
        CHECK(city.map.road(1).trafficFactor() == 2.0);
        CHECK(city.map.road(2).isBlocked());
        CHECK(repo.loadFleet().size() == 3);
    }

    {
        // Foreign keys: a vehicle cannot reference a station that was never saved.
        std::filesystem::remove(p);
        Database db(path);
        DatabaseRepository repo(db);
        bool rejected = false;
        try { repo.saveFleet(fleet); } catch (const DatabaseException&) { rejected = true; }
        CHECK(rejected);
        CHECK(repo.loadFleet().empty());
    }

    std::filesystem::remove(p);
    if (g_failures == 0) std::cout << "RepositoryTests: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}