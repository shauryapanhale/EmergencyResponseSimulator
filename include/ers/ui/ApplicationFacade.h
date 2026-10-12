#pragma once
#include <memory>
#include <vector>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include "ers/models/CityMap.h"
#include "ers/repositories/DatabaseRepository.h"
#include "ers/services/SimulationEngine.h"

namespace ers {

// Plain view structs: the GUI renders these and never touches engine classes directly.
struct IncidentView {
    int id = 0;
    QString type;
    QString priority;
    QString status;
    int nodeId = 0;
    QString nodeName;
    double createdAt = 0.0;
    QString description;
    QList<int> vehicleIds;
    QString unmet;         // resource types still missing, e.g. "Ambulance"
    QString explanation;   // dispatch reasoning (NFR-09)
};

struct VehicleView {
    int id = 0;
    QString type;
    QString status;
    int stationId = 0;
    int nodeId = 0;
    double x = 0.0;
    double y = 0.0;
    int assignedIncident = -1;  // -1 when none
};

struct StatsView {
    int created = 0;
    int resolved = 0;
    int active = 0;
    int pending = 0;
    int responseCount = 0;
    double avgResponse = 0.0;
    double minResponse = 0.0;
    double maxResponse = 0.0;
    double avgWaiting = 0.0;
    double avgResolution = 0.0;
    int resolvedWithUnmet = 0;
};

// Controller layer between the Qt GUI and the C++ engine (SRS 6.2). Single-threaded:
// all calls and the timer run on the UI thread.
class ApplicationFacade : public QObject {
    Q_OBJECT
public:
    explicit ApplicationFacade(QObject* parent = nullptr);
    ~ApplicationFacade() override;

    // Operator actions. On failure they emit errorOccurred() and return -1 / false.
    int reportIncident(EmergencyType type, int nodeId, Priority priority, const QString& description);
    bool resolveIncident(int id);
    bool changePriority(int id, Priority priority);
    bool setTraffic(int roadId, double factor);
    bool setBlocked(int roadId, bool blocked);
    bool runScenario(const QString& name);   // resets, then runs the scenario to its end
    void setTickIntervalMs(int ms);
    void setAutoResolveMinutes(double minutes);  // < 0 = manual; resets the simulation

    // Read-only state
    double time() const;
    bool isRunning() const { return running_; }
    const CityMap& city() const { return city_.map; }
    std::vector<IncidentView> incidents() const;
    std::vector<VehicleView> vehicles() const;
    StatsView stats() const;
    QStringList logLines() const;
    QString explanationFor(int incidentId) const;
    QStringList scenarioNames() const;
    static QString disclaimer();

public slots:
    void start();
    void pause();
    void resume();
    void stepOnce();
    void reset();

signals:
    void stateChanged();
    void timeChanged(double minutes);
    void logLine(const QString& line);
    void runningChanged(bool running);
    void simulationReset();
    void scenarioFinished(const QString& name);
    void errorOccurred(const QString& message);

private:
    template <typename F>
    bool guarded(F&& action);
    void createSimulation();
    void notify();

    CityData city_;                              // declared before sim_ (engine references it)
    std::unique_ptr<SimulationEngine> sim_;
    SimulationConfig cfg_;
    QTimer timer_;
    bool running_ = false;
    std::size_t logIndex_ = 0;
};

} // namespace ers