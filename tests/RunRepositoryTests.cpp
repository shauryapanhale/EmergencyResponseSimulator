#include "ers/repositories/DatabaseRepository.h"
#include "ers/repositories/RunRepository.h"
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
    m.addNode(CityNode(2, "Market", 3.0, 0.0));
    m.addNode(CityNode(3, "Hospital", 3.0, 4.0, NodeKind::Landmark));
    m.addNode(CityNode(4, "Fire HQ", 6.0, 4.0, NodeKind::Station));
    m.addRoad(Road(1, 1, 2, 3.0, 4.0));
    m.addRoad(Road(2, 2, 3, 4.0, 5.0));
    m.addRoad(Road(3, 3, 4, 3.0, 4.0));
    return m;
}

static std::vector<Station> buildStations() {
    std::vector<Station> s;
    s.emplace_back(1, "Central Base", 1, StationType::AmbulanceBase);
    s.emplace_back(2, "Fire HQ", 4, StationType::FireStation);
    return s;
}

static std::vector<std::unique_ptr<Vehicle>> buildFleet() {
    std::vector<std::unique_ptr<Vehicle>> f;
    f.push_back(createVehicle(VehicleType::Ambulance, 1, 1, 1, 60.0));
    f.push_back(createVehicle(VehicleType::FireTruck, 2, 2, 4, 45.0));
    return f;
}

int main() {
    auto p = std::filesystem::temp_directory_path() / "ers_run_repository_test.db";
    std::filesystem::remove(p);
    const std::string path = p.string();

    CityMap map = buildMap();
    SimulationConfig cfg;
    cfg.autoResolveMinutes = 2.0;
    SimulationEngine sim(map, buildFleet(), cfg);
    sim.reportIncident(EmergencyType::Medical, 3, Priority::High, "chest pain");
    sim.reportIncident(EmergencyType::Fire, 3, Priority::Critical, "kitchen fire");
    sim.runTicks(40);

    {
        // Transaction safety: fleet not saved, so dispatch rows violate foreign keys.
        Database db(path);
        DatabaseRepository cfgRepo(db);
        cfgRepo.saveCity(map, buildStations());
        RunRepository runs(db);
        bool rejected = false;
        try { runs.saveRun(sim, "no-fleet"); } catch (const DatabaseException&) { rejected = true; }
        CHECK(rejected);
        CHECK(runs.listRuns().empty());
    }

    long long run1 = 0, run2 = 0;
    {
        Database db(path);
        DatabaseRepository cfgRepo(db);
        cfgRepo.saveCity(map, buildStations());
        cfgRepo.saveFleet(buildFleet());
        RunRepository runs(db);
        run1 = runs.saveRun(sim, "basic");
        run2 = runs.saveRun(sim, "basic-again");
        CHECK(run1 != run2);
    }

    {
        Database db(path);
        RunRepository runs(db);

        auto list = runs.listRuns();
        CHECK(list.size() == 2);
        CHECK(list[0].runId == run1 && list[0].scenarioName == "basic");
        CHECK(list[0].endTime == sim.time());
        CHECK(list[0].summary.find("created=2") != std::string::npos);
        CHECK(list[1].scenarioName == "basic-again");

        auto ids = sim.incidentIds();
        auto incidents = runs.loadIncidents(run1);
        CHECK(incidents.size() == ids.size());
        CHECK(runs.loadIncidents(run2).size() == ids.size());
        for (const auto& row : incidents) {
            const Emergency& e = sim.incident(row.incidentId);
            CHECK(row.nodeId == e.nodeId());
            CHECK(row.description == e.description());
            CHECK(row.createdAt == e.createdAt());
            CHECK(!row.type.empty() && !row.priority.empty() && !row.status.empty());
        }

        auto records = sim.dispatchRecords();
        auto rows = runs.loadDispatches(run1);
        CHECK(!records.empty());
        CHECK(rows.size() == records.size());
        for (const auto& row : rows) {
            bool found = false;
            for (const auto& r : records) {
                if (r.id() != row.dispatchId) continue;
                found = true;
                CHECK(r.emergencyId() == row.incidentId);
                CHECK(r.vehicleId() == row.vehicleId);
                CHECK(r.dispatchedAt() == row.dispatchedAt);
                CHECK(r.etaMinutes() == row.etaMinutes);
                CHECK(r.hasArrived() == row.arrivedAt.has_value());
                if (r.hasArrived()) CHECK(r.arrivedAt() == *row.arrivedAt);
                CHECK(r.hasCompleted() == row.completedAt.has_value());
                CHECK(!row.routeText.empty());
            }
            CHECK(found);
        }

        const auto& original = sim.log().entries();
        auto logRows = runs.loadLog(run1);
        CHECK(!logRows.empty());
        CHECK(logRows.size() == original.size());
        if (!logRows.empty() && logRows.size() == original.size()) {
            CHECK(logRows.front().eventType == original.front().eventType);
            CHECK(logRows.back().details == original.back().details);
            CHECK(logRows.back().simTime == original.back().simTime);
        }
    }

    std::filesystem::remove(p);
    if (g_failures == 0) std::cout << "RunRepositoryTests: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}