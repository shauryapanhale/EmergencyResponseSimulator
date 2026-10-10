#include "ers/repositories/RunRepository.h"
#include "ers/common/Exceptions.h"
#include "ers/repositories/Schema.h"
#include <sstream>

namespace ers {

namespace {

std::string toText(EmergencyType t) {
    switch (t) {
        case EmergencyType::Medical: return "Medical";
        case EmergencyType::Fire: return "Fire";
        case EmergencyType::RoadAccident: return "RoadAccident";
        case EmergencyType::Crime: return "Crime";
    }
    return "Medical";
}

std::string toText(Priority p) {
    switch (p) {
        case Priority::Critical: return "Critical";
        case Priority::High: return "High";
        case Priority::Medium: return "Medium";
        case Priority::Low: return "Low";
    }
    return "Low";
}

std::string toText(EmergencyStatus s) {
    switch (s) {
        case EmergencyStatus::Pending: return "Pending";
        case EmergencyStatus::Dispatched: return "Dispatched";
        case EmergencyStatus::OnScene: return "OnScene";
        case EmergencyStatus::Resolved: return "Resolved";
        case EmergencyStatus::Unserved: return "Unserved";
    }
    return "Pending";
}

std::string routeText(const Route& route) {
    std::ostringstream out;
    bool first = true;
    for (NodeId n : route.nodes()) {
        if (!first) out << '-';
        out << n;
        first = false;
    }
    return out.str();
}

std::string summaryText(const StatsSnapshot& s) {
    std::ostringstream out;
    out << "created=" << s.created << ";resolved=" << s.resolved << ";active=" << s.active
        << ";pending=" << s.pending << ";unserved=" << s.unserved
        << ";responses=" << s.responseCount << ";avgResponse=" << s.avgResponse
        << ";minResponse=" << s.minResponse << ";maxResponse=" << s.maxResponse
        << ";avgWaiting=" << s.avgWaiting << ";avgResolution=" << s.avgResolution;
    return out.str();
}

} // namespace

RunRepository::RunRepository(Database& db) : db_(db) { initSchema(db_); }

long long RunRepository::saveRun(const SimulationEngine& sim, const std::string& scenarioName) {
    if (scenarioName.empty()) throw ValidationException("Scenario name must not be empty");

    Transaction tx(db_);

    Statement run(db_,
        "INSERT INTO simulation_run (scenario_name, start_time, end_time, settings_text) "
        "VALUES (?, ?, ?, ?);");
    run.bind(1, scenarioName);
    run.bind(2, 0.0);
    run.bind(3, sim.time());
    run.bind(4, summaryText(sim.stats()));
    run.step();
    const long long runId = db_.lastInsertRowId();

    Statement inc(db_,
        "INSERT INTO incident (run_id, incident_id, type, priority, node_id, description, "
        "created_at, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
    for (EmergencyId id : sim.incidentIds()) {
        const Emergency& e = sim.incident(id);
        inc.bind(1, runId);
        inc.bind(2, e.id());
        inc.bind(3, toText(e.type()));
        inc.bind(4, toText(e.priority()));
        inc.bind(5, e.nodeId());
        inc.bind(6, e.description());
        inc.bind(7, e.createdAt());
        inc.bind(8, toText(e.status()));
        inc.step();
        inc.reset();
    }

    Statement disp(db_,
        "INSERT INTO dispatch (run_id, dispatch_id, incident_id, vehicle_id, route_text, "
        "dispatched_at, arrived_at, completed_at, eta_minutes, outcome) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    const std::vector<DispatchRecord> records = sim.dispatchRecords();
    for (const DispatchRecord& r : records) {
        disp.bind(1, runId);
        disp.bind(2, r.id());
        disp.bind(3, r.emergencyId());
        disp.bind(4, r.vehicleId());
        disp.bind(5, routeText(r.route()));
        disp.bind(6, r.dispatchedAt());
        if (r.hasArrived()) disp.bind(7, r.arrivedAt()); else disp.bindNull(7);
        if (r.hasCompleted()) disp.bind(8, r.completedAt()); else disp.bindNull(8);
        disp.bind(9, r.etaMinutes());
        disp.bind(10, r.reason());
        disp.step();
        disp.reset();
    }

    Statement ev(db_,
        "INSERT INTO event_log (run_id, sim_time, entity_type, entity_id, event_type, details) "
        "VALUES (?, ?, ?, ?, ?, ?);");
    for (const LogEntry& e : sim.log().entries()) {
        ev.bind(1, runId);
        ev.bind(2, e.simTime);
        ev.bind(3, e.entityType);
        ev.bind(4, e.entityId);
        ev.bind(5, e.eventType);
        ev.bind(6, e.details);
        ev.step();
        ev.reset();
    }

    tx.commit();
    return runId;
}

std::vector<RunInfo> RunRepository::listRuns() {
    std::vector<RunInfo> out;
    Statement q(db_,
        "SELECT run_id, scenario_name, start_time, end_time, settings_text "
        "FROM simulation_run ORDER BY run_id;");
    while (q.step()) {
        RunInfo r;
        r.runId = q.columnInt64(0);
        r.scenarioName = q.columnText(1);
        r.startTime = q.columnDouble(2);
        r.endTime = q.columnIsNull(3) ? 0.0 : q.columnDouble(3);
        r.summary = q.columnText(4);
        out.push_back(r);
    }
    return out;
}

std::vector<IncidentRow> RunRepository::loadIncidents(long long runId) {
    std::vector<IncidentRow> out;
    Statement q(db_,
        "SELECT incident_id, type, priority, node_id, description, created_at, status "
        "FROM incident WHERE run_id = ? ORDER BY incident_id;");
    q.bind(1, runId);
    while (q.step()) {
        IncidentRow r;
        r.incidentId = q.columnInt(0);
        r.type = q.columnText(1);
        r.priority = q.columnText(2);
        r.nodeId = q.columnInt(3);
        r.description = q.columnText(4);
        r.createdAt = q.columnDouble(5);
        r.status = q.columnText(6);
        out.push_back(r);
    }
    return out;
}

std::vector<DispatchRow> RunRepository::loadDispatches(long long runId) {
    std::vector<DispatchRow> out;
    Statement q(db_,
        "SELECT dispatch_id, incident_id, vehicle_id, route_text, dispatched_at, arrived_at, "
        "completed_at, eta_minutes, outcome FROM dispatch WHERE run_id = ? ORDER BY dispatch_id;");
    q.bind(1, runId);
    while (q.step()) {
        DispatchRow r;
        r.dispatchId = q.columnInt(0);
        r.incidentId = q.columnInt(1);
        r.vehicleId = q.columnInt(2);
        r.routeText = q.columnText(3);
        r.dispatchedAt = q.columnDouble(4);
        if (!q.columnIsNull(5)) r.arrivedAt = q.columnDouble(5);
        if (!q.columnIsNull(6)) r.completedAt = q.columnDouble(6);
        r.etaMinutes = q.columnDouble(7);
        r.outcome = q.columnText(8);
        out.push_back(r);
    }
    return out;
}

std::vector<LogEntry> RunRepository::loadLog(long long runId) {
    std::vector<LogEntry> out;
    Statement q(db_,
        "SELECT sim_time, entity_type, entity_id, event_type, details "
        "FROM event_log WHERE run_id = ? ORDER BY event_id;");
    q.bind(1, runId);
    while (q.step()) {
        LogEntry e;
        e.simTime = q.columnDouble(0);
        e.entityType = q.columnText(1);
        e.entityId = q.columnIsNull(2) ? 0 : q.columnInt(2);
        e.eventType = q.columnText(3);
        e.details = q.columnText(4);
        out.push_back(e);
    }
    return out;
}

} // namespace ers