# Architecture

Emergency Response Simulator (ERS) is an educational simulation of a fictional city. It is not a dispatch product and must not be used for real emergencies.

## Layers

    Qt GUI: MainWindow, MapView, dialogs        (include/ers/ui, src/ui)
        |  user action / Qt signal
    ApplicationFacade (QObject controller)      (include/ers/ui/ApplicationFacade.h)
        |  validates, catches every exception, exposes view structs
    Services: SimulationEngine, DispatchEngine, RoutingEngine,
              StatisticsService, EventLog, PendingQueue, Scenario / ScenarioPlayer
        |
    Models: Emergency and Vehicle hierarchies, CityMap, Road, Route, DispatchRecord
        |
    Repositories: Database / Statement / Transaction, DatabaseRepository,
                  RunRepository, SampleCity, Schema
        |
    SQLite database file

Rule: nothing below the facade knows about Qt. The core library (ers_core) and its 21 plain tests build on any machine with CMake and a C++17 compiler.

## Modules

| Folder | Responsibility |
|---|---|
| include/ers/common, src/common | Types, exceptions, Result template, string helpers, version and disclaimer |
| include/ers/models, src/models | Domain classes: incidents, vehicles, city graph, routes, dispatch records, factories |
| include/ers/services, src/services | Routing, dispatch, simulation loop, statistics, event log, scenarios |
| include/ers/repositories, src/repositories | SQLite wrapper, schema, config and run repositories, sample city |
| include/ers/ui, src/ui | Facade, main window, map, dialogs, GUI entry point (main_gui.cpp) |
| tests, src/ui/tests | 21 plain test executables and 2 Qt Test executables |

## Ownership and lifetime

- MainWindow owns ApplicationFacade by value; MapView holds a reference to it.
- ApplicationFacade owns CityData, then SimulationEngine, then the optional ScenarioPlayer (std::unique_ptr declared after the engine so it is destroyed first).
- SimulationEngine holds a reference to the CityMap, so the city is declared before the engine and outlives it.
- Polymorphic objects (Emergency, Vehicle) are owned through std::unique_ptr and cannot be copied.

## Key flows

Reporting an incident: dialog -> ApplicationFacade::reportIncident -> SimulationEngine::reportIncident -> PendingQueue and DispatchEngine::dispatch -> RoutingEngine::shortestPath per candidate vehicle -> vehicle reserved, DispatchRecord created, event logged -> facade emits stateChanged -> tables and map re-read the view structs.

One tick: move every vehicle that is ToScene or Returning, handle arrivals, advance time, auto-resolve if configured, retry pending incidents and unmet resource types (Critical first, then creation time, then id), track vehicle utilization.

Scenario playback: ScenarioPlayer applies the events that are due (traffic, road blocks, incidents) and then advances the engine one tick. The batch runScenario produces identical statistics, which ScenarioPlayerTests verifies.

## Timing model

Road costs are stored in minutes at a reference speed of 30 km/h:

    edgeMinutes = baseMinutes * trafficFactor
    vehicle time factor = 30 / vehicleSpeedKmph       (Vehicle::timeFactor)
    ETA = route road-minutes * time factor

The factor is applied to the ranking and stored ETA (DispatchEngine), to movement along the route and the return trip (SimulationEngine::moveAlong). Dijkstra works on road minutes only: scaling every edge by the same constant never changes the shortest path, so routes are the same for every vehicle type.

| Vehicle | Speed | Factor |
|---|---|---|
| Ambulance | 60 km/h | 0.5 |
| Fire truck | 45 km/h | 0.667 |
| Police | 70 km/h | 0.429 |

## Persistence

SQLite is accessed through a thin C++ RAII wrapper over the SQLite C library (amalgamation vendored in third_party/sqlite). Nine tables; incident and dispatch use composite keys (run_id, id) because incident ids restart for each run. saveRun is a single transaction, so a failure leaves no partial run. The GUI stores saved runs in ers_gui.db inside the per-user data folder (%APPDATA%\ERS\EmergencyResponseSimulator on Windows).

## Threading

Single-threaded. The facade drives stepping with a QTimer on the UI thread; the engine is not thread-safe.

## Tests

23 CTest executables, all passing at the time of writing: DispatchTests, EdgeCaseTests, EmergencyTests, IntegrationTests, PerformanceTests, PersistenceTests, PriorityTests, RepositoryTests, RoutingTests, RunRepositoryTests, SampleCityTests, ScenarioPlayerTests, ScenarioTests, SchemaTests, SimulationTests, SmokeTests, SpeedEtaTests, StatisticsTests, StatsUnmetTests, SupplementTests, VehicleStateTests, plus the Qt Test executables ApplicationFacadeTest and FacadeRunsTest. The widgets (MainWindow, MapView, dialogs) are checked by the manual script in SRS Appendix E.

Measured on a generated 500-node, 955-road city (Debug build, one machine): corner-to-corner route 1.8 ms, one dispatch with 20 candidates 28 ms, 20 incidents plus 30 ticks 214 ms.

## Known limitations

- Vehicles already travelling keep their route when roads change; only new dispatches use the updated graph.
- The map redraws the whole scene on every update, which is fine for the 12-node sample city but would need incremental updates for very large cities.
- The selected dispatch route is not highlighted on the map.
- Packaging has not yet been tested on a machine without Qt or Visual C++ installed.