#pragma once
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "ers/models/CityMap.h"
#include "ers/models/DispatchRecord.h"
#include "ers/models/EmergencyFactory.h"
#include "ers/models/Vehicle.h"
#include "ers/services/DispatchEngine.h"
#include "ers/services/EventLog.h"
#include "ers/services/PendingQueue.h"
#include "ers/services/RoutingEngine.h"
#include "ers/services/StatisticsService.h"

namespace ers {

struct SimulationConfig {
    double tickMinutes = 1.0;
    double autoResolveMinutes = -1.0;  // < 0: operator resolves manually
};

struct Point {
    double x;
    double y;
};

// Owns the run state (fleet, incidents, queue, stats, log). The city map is shared by reference.
// step() always advances one tick; start/pause/resume only record state for the GUI timer.
class SimulationEngine {
public:
    SimulationEngine(CityMap& city, std::vector<std::unique_ptr<Vehicle>> fleet,
                     SimulationConfig config = SimulationConfig());
    SimulationEngine(const SimulationEngine&) = delete;
    SimulationEngine& operator=(const SimulationEngine&) = delete;

    // Controls (reset = construct a new SimulationEngine)
    void start();
    void pause();
    void resume();
    bool isRunning() const { return running_; }
    void step();
    void runTicks(int n);

    // Operator actions
    EmergencyId reportIncident(EmergencyType type, NodeId node, Priority priority,
                               const std::string& description);
    void resolveIncident(EmergencyId id);  // requires On Scene and all assigned vehicles arrived
    void changeIncidentPriority(EmergencyId id, Priority p);
    void setTraffic(RoadId road, double factor);
    void setBlocked(RoadId road, bool blocked);

    // Read-only queries
    double time() const { return now_; }
    const CityMap& city() const { return city_; }
    const Emergency& incident(EmergencyId id) const;
    std::vector<EmergencyId> incidentIds() const;
    std::vector<EmergencyId> pendingIncidents() const { return queue_.snapshot(); }
    const std::vector<std::unique_ptr<Vehicle>>& fleet() const { return fleet_; }
    const Vehicle& vehicle(VehicleId id) const;
    std::vector<DispatchRecord> dispatchRecords() const;
    Point vehiclePosition(VehicleId id) const;
    std::string explanationFor(EmergencyId id) const;
    StatsSnapshot stats() const { return stats_.snapshot(); }
    const EventLog& log() const { return log_; }

private:
    enum class Phase { ToScene, OnScene, Returning, Done };

    struct Response {
        explicit Response(DispatchRecord r)
            : record(std::move(r)), phase(Phase::ToScene), nodes(record.route().nodes()),
              roads(record.route().roadIds()), seg(0), progress(0.0), arrivedAt(-1.0) {}
        DispatchRecord record;
        Phase phase;
        std::vector<NodeId> nodes;
        std::vector<RoadId> roads;
        std::size_t seg;
        double progress;
        double arrivedAt;
    };

    Emergency& findIncident(EmergencyId id);
    Vehicle& vehicleRef(VehicleId id);
    double moveAlong(Response& r, double dt, bool& reached);
    void handleArrival(Response& r, double time);
    void autoResolve();
    void processPending();
    void registerDispatch(Emergency& e, const DispatchResult& result);

    CityMap& city_;
    RoutingEngine routing_;
    DispatchEngine dispatcher_;
    std::vector<std::unique_ptr<Vehicle>> fleet_;
    SimulationConfig cfg_;
    std::unordered_map<VehicleId, std::size_t> vehicleIndex_;
    std::map<EmergencyId, std::unique_ptr<Emergency>> incidents_;
    PendingQueue queue_;
    std::vector<Response> responses_;
    std::unordered_map<VehicleId, std::size_t> activeByVehicle_;
    StatisticsService stats_;
    EventLog log_;
    std::map<EmergencyId, std::string> explanations_;
    std::unordered_set<EmergencyId> loggedPending_;
    double now_ = 0.0;
    bool running_ = false;
    EmergencyId nextIncidentId_ = 1;
};

} // namespace ers