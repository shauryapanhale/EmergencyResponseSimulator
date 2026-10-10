#include <iostream>
#include <string>
#include "ers/repositories/DatabaseRepository.h"
#include "ers/repositories/RunRepository.h"
#include "ers/repositories/SampleCity.h"
#include "ers/services/SimulationEngine.h"

using namespace ers;

int main(int argc, char** argv) {
    const std::string path = argc > 1 ? argv[1] : "ers_demo.db";
    std::cout << "Emergency Response Simulator - persistence demo\n"
              << "Educational simulation only - not for real emergency use.\n\n";

    try {
        Database db(path);
        bool seeded = seedSampleCity(db);
        std::cout << "Database: " << path << (seeded ? " (seeded sample city)\n" : " (existing city loaded)\n");

        DatabaseRepository repo(db);
        CityData city = repo.loadCity();
        auto fleet = repo.loadFleet();
        std::cout << "Loaded " << city.map.nodes().size() << " nodes, " << city.map.roads().size()
                  << " roads, " << city.stations.size() << " stations, " << fleet.size()
                  << " vehicles\n";

        SimulationConfig cfg;
        cfg.autoResolveMinutes = 3.0;
        SimulationEngine sim(city.map, std::move(fleet), cfg);
        sim.setBlocked(4, true);  // City Hospital - Railway Station closed
        sim.reportIncident(EmergencyType::Medical, 7, Priority::High, "fall in Old Town");
        sim.reportIncident(EmergencyType::Fire, 9, Priority::Critical, "fire at Tech Park");
        sim.reportIncident(EmergencyType::Crime, 10, Priority::Medium, "theft at College Campus");
        sim.reportIncident(EmergencyType::RoadAccident, 8, Priority::Critical, "collision at Riverside");
        sim.runTicks(80);

        RunRepository runs(db);
        long long runId = runs.saveRun(sim, "persistence-demo");
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