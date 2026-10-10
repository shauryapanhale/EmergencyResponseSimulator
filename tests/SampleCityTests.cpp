#include "ers/repositories/SampleCity.h"
#include "ers/repositories/RunRepository.h"
#include "ers/services/SimulationEngine.h"
#include <filesystem>
#include <iostream>
#include <queue>
#include <set>
#include <string>

static int g_failures = 0;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        std::cerr << "CHECK failed: " #cond " (line " << __LINE__ << ")\n"; } } while (0)

using namespace ers;

static std::size_t reachableFrom(const CityMap& map, NodeId start) {
    std::set<NodeId> seen{start};
    std::queue<NodeId> q;
    q.push(start);
    while (!q.empty()) {
        NodeId cur = q.front();
        q.pop();
        for (const Edge& e : map.neighbours(cur)) {
            if (seen.insert(e.to).second) q.push(e.to);
        }
    }
    return seen.size();
}

int main() {
    CityData city = buildSampleCity();
    CHECK(city.map.nodes().size() == 12);
    CHECK(city.map.roads().size() == 18);
    CHECK(city.stations.size() == 3);
    CHECK(buildSampleFleet().size() == 6);
    CHECK(reachableFrom(city.map, 1) == 12);
    CHECK(city.stations[0].vehicleIds().size() == 2);

    auto p = std::filesystem::temp_directory_path() / "ers_sample_city_test.db";
    std::filesystem::remove(p);
    {
        Database db(p.string());
        CHECK(seedSampleCity(db));
        CHECK(!seedSampleCity(db));  // already seeded

        DatabaseRepository repo(db);
        CityData loaded = repo.loadCity();
        auto fleet = repo.loadFleet();
        CHECK(loaded.map.nodes().size() == 12);
        CHECK(loaded.map.roads().size() == 18);
        CHECK(loaded.stations.size() == 3);
        CHECK(loaded.stations[1].vehicleIds().size() == 2);
        CHECK(fleet.size() == 6);

        SimulationConfig cfg;
        cfg.autoResolveMinutes = 3.0;
        SimulationEngine sim(loaded.map, std::move(fleet), cfg);
        sim.reportIncident(EmergencyType::Medical, 7, Priority::High, "fall in Old Town");
        sim.reportIncident(EmergencyType::Fire, 9, Priority::Critical, "fire at Tech Park");
        sim.runTicks(60);
        CHECK(sim.stats().created == 2);
        CHECK(!sim.dispatchRecords().empty());

        RunRepository runs(db);
        long long id = runs.saveRun(sim, "sample-city-e2e");
        CHECK(runs.listRuns().size() == 1);
        CHECK(runs.loadIncidents(id).size() == 2);
        CHECK(runs.loadDispatches(id).size() == sim.dispatchRecords().size());
    }

    std::filesystem::remove(p);
    if (g_failures == 0) std::cout << "SampleCityTests: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}