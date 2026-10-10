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

// Presentation only: renders ApplicationFacade view structs and forwards user actions.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void refreshTables();
    void onTimeChanged(double minutes);
    void onLogLine(const QString& line);
    void onError(const QString& message);
    void onRunningChanged(bool running);
    void onReset();
    void runSelectedScenario();

private:
    void buildUi();

    ApplicationFacade facade_;
    QTableWidget* incidentTable_ = nullptr;
    QTableWidget* vehicleTable_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QLabel* timeLabel_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    QPushButton* resumeBtn_ = nullptr;
    QPushButton* stepBtn_ = nullptr;
    QPushButton* resetBtn_ = nullptr;
    QPushButton* scenarioBtn_ = nullptr;
    QComboBox* scenarioBox_ = nullptr;
};

} // namespace ers