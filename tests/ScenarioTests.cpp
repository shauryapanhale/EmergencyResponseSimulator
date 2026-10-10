#include "ers/common/Exceptions.h"
#include "ers/repositories/SampleCity.h"
#include "ers/services/Scenario.h"
#include <iostream>
#include <string>

static int g_failures = 0;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        std::cerr << "CHECK failed: " #cond " (line " << __LINE__ << ")\n"; } } while (0)

using namespace ers;

struct Outcome {
    int created = 0;
    std::size_t dispatches = 0;
    double avgResponse = 0.0;
    double endTime = 0.0;
    std::size_t firstRouteNodes = 0;
};

static Outcome run(const std::string& name) {
    const Scenario& sc = findScenario(name);
    CityData city = buildSampleCity();
    SimulationConfig cfg;
    cfg.autoResolveMinutes = 3.0;
    SimulationEngine sim(city.map, buildSampleFleet(), cfg);
    runScenario(sim, sc);

    Outcome o;
    o.created = sim.stats().created;
    o.avgResponse = sim.stats().avgResponse;
    o.endTime = sim.time();
    auto records = sim.dispatchRecords();
    o.dispatches = records.size();
    for (const auto& r : records) {
        if (r.emergencyId() == 1) { o.firstRouteNodes = r.route().nodes().size(); break; }
    }
    return o;
}

int main() {
    CHECK(sampleScenarios().size() == 4);

    bool unknown = false;
    try { findScenario("no-such-scenario"); } catch (const ValidationException&) { unknown = true; }
    CHECK(unknown);

    for (const Scenario& sc : sampleScenarios()) {
        Outcome a = run(sc.name);
        Outcome b = run(sc.name);
        CHECK(a.created == static_cast<int>(sc.incidents.size()));
        CHECK(a.endTime >= sc.durationMinutes);
        CHECK(a.dispatches > 0);
        CHECK(a.dispatches == b.dispatches);          // deterministic
        CHECK(a.avgResponse == b.avgResponse);
    }

    // Blocked road: police from node 3 to Old Town (7) cannot use roads 9 or 18.
    Outcome blocked = run("blocked-road");
    CHECK(blocked.firstRouteNodes > 2);

    if (g_failures == 0) std::cout << "ScenarioTests: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}