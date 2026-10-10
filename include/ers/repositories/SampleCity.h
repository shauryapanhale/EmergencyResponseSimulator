#pragma once
#include <memory>
#include <vector>
#include "ers/models/Vehicle.h"
#include "ers/repositories/Database.h"
#include "ers/repositories/DatabaseRepository.h"

namespace ers {

// Fictional city used for demos and tests (not a map of any real place).
CityData buildSampleCity();
std::vector<std::unique_ptr<Vehicle>> buildSampleFleet();

// Saves the sample city and fleet only if the database has no city yet.
// Returns true if seeding happened, false if data already existed.
bool seedSampleCity(Database& db);

} // namespace ers