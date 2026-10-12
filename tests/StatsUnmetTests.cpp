#include <iostream>
#include "ers/repositories/SampleCity.h"
#include "ers/services/Scenario.h"
#include "ers/services/SimulationEngine.h"
#include "ers/services/StatisticsService.h"

namespace {
int failures = 0;
}

#define EXPECT(cond)                                                          \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "FAIL line " << __LINE__ << ": " #cond << "\n";      \
            ++failures;                                                       \
        }                                                                     \
    } while (0)

static ers::StatsSnapshot runNamed(const char* name) {
    ers::CityData city = ers::buildSampleCity();
    ers::SimulationConfig cfg;
    cfg.autoResolveMinutes = 3.0;
    ers::SimulationEngine sim(city.map, ers::buildSampleFleet(), cfg);
    ers::runScenario(sim, ers::findScenario(name));
    return sim.stats();
}

int main() {
    {
        ers::StatisticsService s;
        s.incidentCreated(1, 0.0);
        s.incidentDispatched(1, 1.0);
        s.incidentResolved(1, 5.0, true);
        s.incidentCreated(2, 0.0);
        s.incidentDispatched(2, 1.0);
        s.incidentResolved(2, 6.0);
        const ers::StatsSnapshot snap = s.snapshot();
        EXPECT(snap.resolved == 2);
        EXPECT(snap.resolvedWithUnmet == 1);
    }
    {
        const ers::StatsSnapshot blocked = runNamed("blocked-road");
        std::cout << "blocked-road: resolved=" << blocked.resolved
                  << " resolvedWithUnmet=" << blocked.resolvedWithUnmet << "\n";
        EXPECT(blocked.resolved == 3);
        EXPECT(blocked.resolvedWithUnmet == 0);
    }
    {
        const ers::StatsSnapshot sim = runNamed("simultaneous-incidents");
        std::cout << "simultaneous-incidents: resolved=" << sim.resolved
                  << " resolvedWithUnmet=" << sim.resolvedWithUnmet << "\n";
        EXPECT(sim.resolved == 6);
        EXPECT(sim.resolvedWithUnmet >= 1);
        EXPECT(sim.resolvedWithUnmet <= sim.resolved);
    }
    if (failures == 0) std::cout << "StatsUnmetTests: all checks passed\n";
    return failures == 0 ? 0 : 1;
}