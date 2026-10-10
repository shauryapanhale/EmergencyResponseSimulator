#pragma once
#include <optional>
#include <string>
#include <vector>
#include "ers/repositories/Database.h"
#include "ers/services/EventLog.h"
#include "ers/services/SimulationEngine.h"

namespace ers {

struct RunInfo {
    long long runId = 0;
    std::string scenarioName;
    double startTime = 0.0;
    double endTime = 0.0;
    std::string summary;  // "created=2;resolved=1;..."
};

struct IncidentRow {
    int incidentId = 0;
    std::string type;
    std::string priority;
    int nodeId = 0;
    std::string description;
    double createdAt = 0.0;
    std::string status;
};

struct DispatchRow {
    int dispatchId = 0;
    int incidentId = 0;
    int vehicleId = 0;
    std::string routeText;  // node ids joined by '-'
    double dispatchedAt = 0.0;
    std::optional<double> arrivedAt;
    std::optional<double> completedAt;
    double etaMinutes = 0.0;
    std::string outcome;    // dispatch reason
};

// Saves and loads completed simulation runs (FR-21). City and fleet must be saved first.
class RunRepository {
public:
    explicit RunRepository(Database& db);

    // One transaction: run row, incidents, dispatches and event log. Returns the new run id.
    long long saveRun(const SimulationEngine& sim, const std::string& scenarioName);

    std::vector<RunInfo> listRuns();
    std::vector<IncidentRow> loadIncidents(long long runId);
    std::vector<DispatchRow> loadDispatches(long long runId);
    std::vector<LogEntry> loadLog(long long runId);

private:
    Database& db_;
};

} // namespace ers