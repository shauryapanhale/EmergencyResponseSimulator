#pragma once
#include <memory>
#include <vector>
#include "ers/models/CityMap.h"
#include "ers/models/Station.h"
#include "ers/models/Vehicle.h"
#include "ers/repositories/Database.h"

namespace ers {

// City configuration loaded from the database.
struct CityData {
    CityMap map;
    std::vector<Station> stations;  // vehicleIds() filled from the vehicle table
};

// All SQL for configuration persistence lives here (no SQL in models, services or UI).
class DatabaseRepository {
public:
    // Creates the schema on first use.
    explicit DatabaseRepository(Database& db);

    // Upserts nodes, roads and stations in one transaction.
    void saveCity(const CityMap& map, const std::vector<Station>& stations);
    CityData loadCity();

    // Upserts vehicles in one transaction (stations must already be saved).
    void saveFleet(const std::vector<std::unique_ptr<Vehicle>>& fleet);
    std::vector<std::unique_ptr<Vehicle>> loadFleet();

private:
    Database& db_;
};

} // namespace ers