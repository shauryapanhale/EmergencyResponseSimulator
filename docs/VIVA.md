# Viva guide

Every OOP concept used in ERS, with where to find it. Paths come from SRS Section 7 and the repository listing; open each file once before the viva to confirm the line you want to show.

## Concept map

| Concept | Where | What to say |
|---|---|---|
| Classes and objects | include/ers/models/*.h | Emergency, Vehicle, CityMap, Road, Route, DispatchRecord model the domain |
| Encapsulation | include/ers/models/Vehicle.h | status is private; only validated methods (setStatus, assignIncident, markArrived, markReturning) change it |
| Abstraction | Emergency.h, Vehicle.h | DispatchEngine works through the base interface and never needs the concrete type |
| Inheritance | MedicalEmergency.h, FireEmergency.h, CrimeEmergency.h, RoadAccidentEmergency.h; Ambulance.h, FireTruck.h, PoliceVehicle.h | Specialised incidents and vehicles derive from abstract bases |
| Pure virtual functions, abstract classes | Emergency.h, Vehicle.h | type(), canRespondTo(), requiredResources() have no base implementation |
| Runtime polymorphism, dynamic binding | Vehicle::canRespondTo, Emergency::requiredResources, used in src/services/DispatchEngine.cpp | The dispatcher calls Vehicle& and Emergency& and gets the derived behaviour |
| Function and constructor overloading | include/ers/models/Road.h, RoutingEngine.h (calculateETA), include/ers/repositories/Database.h (Statement::bind) | Simple and fully configured constructors; overloaded ETA and bind |
| Delegating constructors, default arguments | Road.h; Vehicle.h (equipment = {}); StatisticsService.h (hadUnmetResources = false) | The simple Road constructor delegates to the full one |
| Copy and move semantics | include/ers/models/Route.h; include/ers/repositories/Database.h | Route is a value type with move; Database and Statement are move-only |
| Destructors and RAII | Database.h (Database, Statement, Transaction) | Transaction rolls back unless commit() is called; the connection closes in the destructor |
| Static members and functions | Vehicle::canTransition, Vehicle::kReferenceSpeedKmph, Emergency::totalCreated, EventLog::format, ApplicationFacade::disclaimer | Class-wide state and helpers |
| Friend function | include/ers/models/DispatchRecord.h (dispatchAudit) | One narrowly scoped friend for audit output |
| Operator overloading | include/ers/models/PriorityKey.h | operator< orders incidents by priority, creation time, id |
| Templates | include/ers/common/Result.h; ApplicationFacade::guarded | A class template and a member template that wraps every operation |
| Exception handling | include/ers/common/Exceptions.h; validation in Road.h; caught in src/ui/ApplicationFacade.cpp | One ErsException hierarchy; the facade turns every exception into errorOccurred |
| Composition and association | CityMap.h, SimulationEngine.h, DispatchRecord.h | CityMap owns nodes and roads; SimulationEngine owns fleet, incidents, queue, statistics, log |
| Smart pointers | VehicleFactory.h (createVehicle), SimulationEngine.h | unique_ptr for owned polymorphic objects, no manual delete |
| STL and algorithms | PendingQueue.h, src/services/RoutingEngine.cpp, SimulationEngine.cpp | priority_queue, maps, sets, stable_sort, min_element |
| File handling | include/ers/services/EventLog.h (exportToFile) | Plain-text log through C++ streams |
| Qt object model, signals and slots | include/ers/ui/ApplicationFacade.h | QObject controller with signals stateChanged, logLine, errorOccurred; observer pattern |
| Inheritance from framework classes | MainWindow.h (QMainWindow), MapView.h (QGraphicsView), Dialogs.h (QDialog) | MapView overrides wheelEvent, mousePressEvent and other virtual handlers |
| Lambdas | src/ui/MainWindow.cpp, src/ui/Dialogs.cpp, ApplicationFacade::guarded | Signal connections and the guarded wrapper |
| Namespaces, const correctness | All code in namespace ers; const references and const queries | |

## Five-minute demo script

1. Start ers_gui.exe: banner, 12-node map, 6 vehicles.
2. Click Old Town, Report incident (Medical, High): an ambulance is dispatched and the details pane shows the reason and ETA.
3. Press Start (F5): the vehicle moves; Resolve selected after it is OnScene.
4. Traffic / roads: block road 9, report another incident at Old Town: the route avoids it.
5. Play scenario simultaneous-incidents at 10x: show unmet resources and the Statistics dialog (resolved with unmet).
6. Save run, close, reopen, Saved runs: the run is still listed.

## Likely questions

- Why abstract Emergency and Vehicle? Each type decides its own compatibility and resource needs; the dispatcher stays unchanged when a new type is added.
- Why Dijkstra? Road weights are non-negative minutes; A* is listed as a future extension.
- How does vehicle speed change ETA without changing routes? Every edge is scaled by the same factor 30 / speed, so the shortest path is unchanged; ETA, ranking and movement use the scaled time. RoutingTests are untouched and the demo routes are identical before and after.
- What changed when speed was applied? Blocked-road average response went from 17.75 to 9.29 minutes and the simultaneous-incidents run gained an ambulance dispatch for incident 6 (7 to 8 responses, resolved with unmet 2 to 1).
- How is double booking prevented? A vehicle is reserved at dispatch (Available to EnRoute), transitions are validated, and IntegrationTests checks every tick of all four scenarios.
- What if no vehicle is available? The incident stays Pending with an explanation and is retried every tick in priority order; unmet resource types are retried the same way.
- Why plain SQLite and not Qt SQL? The core library and its tests stay free of Qt, so they build on any machine and in GitHub Actions.
- How is the GUI separated from logic? MainWindow and MapView read plain view structs from ApplicationFacade and forward actions; they contain no routing, dispatch or persistence code.
- How was the GUI tested? By the manual script in SRS Appendix E; everything below the widgets has automated tests.
- What are the limits? Simulated estimates only, vehicles keep their route when roads change, and the map redraws the whole scene on each update.