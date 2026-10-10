#include <iostream>
#include <string>
#include "ers/repositories/DatabaseRepository.h"
#include "ers/repositories/RunRepository.h"
#include "ers/repositories/SampleCity.h"
#include "ers/services/Scenario.h"
#include "ers/services/SimulationEngine.h"

using namespace ers;

// Usage: ers_persistence_demo [dbPath] [scenarioName | list]
int main(int argc, char** argv) {
    const std::string path = argc > 1 ? argv[1] : "ers_demo.db";
    const std::string scenarioName = argc > 2 ? argv[2] : "normal-traffic";

    std::cout << "Emergency Response Simulator - persistence demo\n"
              << "Educational simulation only - not for real emergency use.\n\n";

    if (scenarioName == "list") {
        for (const Scenario& s : sampleScenarios()) {
            std::cout << s.name << " (" << s.durationMinutes << " min): " << s.description << "\n";
        }
        return 0;
    }

    try {
        const Scenario& scenario = findScenario(scenarioName);

        Database db(path);
        bool seeded = seedSampleCity(db);
        std::cout << "Database: " << path << (seeded ? " (seeded sample city)\n" : " (existing city loaded)\n");

        DatabaseRepository repo(db);
        CityData city = repo.loadCity();
        auto fleet = repo.loadFleet();
        std::cout << "Loaded " << city.map.nodes().size() << " nodes, " << city.map.roads().size()
                  << " roads, " << city.stations.size() << " stations, " << fleet.size()
                  << " vehicles\n";
        std::cout << "Scenario: " << scenario.name << " - " << scenario.description << "\n";

        SimulationConfig cfg;
        cfg.autoResolveMinutes = 3.0;
        SimulationEngine sim(city.map, std::move(fleet), cfg);
        runScenario(sim, scenario);

        RunRepository runs(db);
        long long runId = runs.saveRun(sim, scenario.name);
        std::cout << "\nSaved run " << runId << " at sim time " << sim.time() << " min\n\n";

        for (const RunInfo& r : runs.listRuns()) {
            std::cout << "Run " << r.runId << " [" << r.scenarioName << "] ends at " << r.endTime
                      << "\n  " << r.summary << "\n";
        }

        std::cout << "\nDispatches stored for run " << runId << ":\n";
        for (const DispatchRow& d : runs.loadDispatches(runId)) {
            std::cout << "  #" << d.dispatchId << " incident " << d.incidentId << " <- vehicle "
                      << d.vehicleId << " route " << d.routeText << " eta " << d.etaMinutes
                      << " min, arrived " << (d.arrivedAt ? "yes" : "no") << "\n";
        }
        std::cout << "\nEvent log rows stored: " << runs.loadLog(runId).size() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}