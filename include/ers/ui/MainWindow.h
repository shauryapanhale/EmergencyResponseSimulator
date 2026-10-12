#pragma once
#include <QMainWindow>
#include <QString>
#include "ers/ui/ApplicationFacade.h"

class QTableWidget;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QComboBox;

namespace ers {

class MapView;

// Presentation only: renders ApplicationFacade view structs and forwards user actions.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void refreshTables();
    void updateDetails();
    void onTimeChanged(double minutes);
    void onLogLine(const QString& line);
    void onError(const QString& message);
    void onRunningChanged(bool running);
    void onReset();
    void runSelectedScenario();
    void onReportIncident();
    void onRoadDialog();
    void onResolve();
    void onChangePriority();
    void onShowStats();
    void onSaveRun();
    void onShowRuns();
    void onResetClicked();
    void onPlayScenario();

private:
    void buildUi();
    int selectedIncidentId() const;
    bool confirmDiscard();

    ApplicationFacade facade_;
    MapView* mapView_ = nullptr;
    QTableWidget* incidentTable_ = nullptr;
    QTableWidget* vehicleTable_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QPlainTextEdit* detailsView_ = nullptr;
    QLabel* timeLabel_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    QPushButton* resumeBtn_ = nullptr;
    QPushButton* stepBtn_ = nullptr;
    QPushButton* resetBtn_ = nullptr;
    QPushButton* scenarioBtn_ = nullptr;
    QPushButton* reportBtn_ = nullptr;
    QPushButton* roadBtn_ = nullptr;
    QPushButton* statsBtn_ = nullptr;
    QPushButton* saveBtn_ = nullptr;
    QPushButton* runsBtn_ = nullptr;
    QPushButton* playBtn_ = nullptr;
    QPushButton* resolveBtn_ = nullptr;
    QPushButton* priorityBtn_ = nullptr;
    QComboBox* scenarioBox_ = nullptr;
    QComboBox* speedBox_ = nullptr;
};

} // namespace ers