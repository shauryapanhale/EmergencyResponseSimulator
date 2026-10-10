#include "ers/ui/MainWindow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace ers {

namespace {
QTableWidget* makeTable(const QStringList& headers) {
    auto* t = new QTableWidget();
    t->setColumnCount(headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setAlternatingRowColors(true);
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setStretchLastSection(true);
    return t;
}
} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Emergency Response Simulator");
    buildUi();

    connect(&facade_, &ApplicationFacade::stateChanged, this, &MainWindow::refreshTables);
    connect(&facade_, &ApplicationFacade::timeChanged, this, &MainWindow::onTimeChanged);
    connect(&facade_, &ApplicationFacade::logLine, this, &MainWindow::onLogLine);
    connect(&facade_, &ApplicationFacade::errorOccurred, this, &MainWindow::onError);
    connect(&facade_, &ApplicationFacade::runningChanged, this, &MainWindow::onRunningChanged);
    connect(&facade_, &ApplicationFacade::simulationReset, this, &MainWindow::onReset);

    connect(startBtn_, &QPushButton::clicked, &facade_, &ApplicationFacade::start);
    connect(pauseBtn_, &QPushButton::clicked, &facade_, &ApplicationFacade::pause);
    connect(resumeBtn_, &QPushButton::clicked, &facade_, &ApplicationFacade::resume);
    connect(stepBtn_, &QPushButton::clicked, &facade_, &ApplicationFacade::stepOnce);
    connect(resetBtn_, &QPushButton::clicked, &facade_, &ApplicationFacade::reset);
    connect(scenarioBtn_, &QPushButton::clicked, this, &MainWindow::runSelectedScenario);

    onRunningChanged(false);
    onTimeChanged(facade_.time());
    refreshTables();
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    auto* banner = new QLabel(ApplicationFacade::disclaimer());
    banner->setWordWrap(true);
    banner->setStyleSheet("background:#fff3cd; color:#664d03; padding:6px; border:1px solid #ffecb5;");
    root->addWidget(banner);

    auto* controls = new QHBoxLayout();
    startBtn_ = new QPushButton("Start");
    pauseBtn_ = new QPushButton("Pause");
    resumeBtn_ = new QPushButton("Resume");
    stepBtn_ = new QPushButton("Step");
    resetBtn_ = new QPushButton("Reset");
    controls->addWidget(startBtn_);
    controls->addWidget(pauseBtn_);
    controls->addWidget(resumeBtn_);
    controls->addWidget(stepBtn_);
    controls->addWidget(resetBtn_);
    controls->addStretch();
    controls->addWidget(new QLabel("Scenario:"));
    scenarioBox_ = new QComboBox();
    scenarioBox_->addItems(facade_.scenarioNames());
    scenarioBtn_ = new QPushButton("Run scenario");
    controls->addWidget(scenarioBox_);
    controls->addWidget(scenarioBtn_);
    root->addLayout(controls);

    incidentTable_ = makeTable({"ID", "Type", "Priority", "Status", "Location", "Vehicles", "Unmet"});
    vehicleTable_ = makeTable({"ID", "Type", "Status", "Station", "Node", "Incident"});

    auto* tables = new QSplitter(Qt::Horizontal);
    auto* incBox = new QWidget();
    auto* incLay = new QVBoxLayout(incBox);
    incLay->setContentsMargins(0, 0, 0, 0);
    incLay->addWidget(new QLabel("Incidents"));
    incLay->addWidget(incidentTable_);
    auto* vehBox = new QWidget();
    auto* vehLay = new QVBoxLayout(vehBox);
    vehLay->setContentsMargins(0, 0, 0, 0);
    vehLay->addWidget(new QLabel("Vehicles"));
    vehLay->addWidget(vehicleTable_);
    tables->addWidget(incBox);
    tables->addWidget(vehBox);

    logView_ = new QPlainTextEdit();
    logView_->setReadOnly(true);
    logView_->setMaximumBlockCount(5000);

    auto* vertical = new QSplitter(Qt::Vertical);
    vertical->addWidget(tables);
    auto* logBox = new QWidget();
    auto* logLay = new QVBoxLayout(logBox);
    logLay->setContentsMargins(0, 0, 0, 0);
    logLay->addWidget(new QLabel("Event log"));
    logLay->addWidget(logView_);
    vertical->addWidget(logBox);
    vertical->setStretchFactor(0, 3);
    vertical->setStretchFactor(1, 2);
    root->addWidget(vertical, 1);

    setCentralWidget(central);

    timeLabel_ = new QLabel();
    statusBar()->addPermanentWidget(timeLabel_);
    statusBar()->showMessage("Ready. Simulated estimates only.");
    resize(1100, 720);
}

void MainWindow::refreshTables() {
    const auto incidents = facade_.incidents();
    incidentTable_->setRowCount(static_cast<int>(incidents.size()));
    int row = 0;
    for (const auto& i : incidents) {
        QStringList vehicles;
        for (int v : i.vehicleIds) vehicles << QString::number(v);
        const QStringList cells = {QString::number(i.id), i.type, i.priority, i.status,
                                   i.nodeName, vehicles.join(", "), i.unmet};
        for (int c = 0; c < cells.size(); ++c)
            incidentTable_->setItem(row, c, new QTableWidgetItem(cells[c]));
        ++row;
    }

    const auto fleet = facade_.vehicles();
    vehicleTable_->setRowCount(static_cast<int>(fleet.size()));
    row = 0;
    for (const auto& v : fleet) {
        const QString inc = v.assignedIncident < 0 ? QString("-") : QString::number(v.assignedIncident);
        const QStringList cells = {QString::number(v.id), v.type, v.status,
                                   QString::number(v.stationId), QString::number(v.nodeId), inc};
        for (int c = 0; c < cells.size(); ++c)
            vehicleTable_->setItem(row, c, new QTableWidgetItem(cells[c]));
        ++row;
    }
}

void MainWindow::onTimeChanged(double minutes) {
    timeLabel_->setText(QString("Simulation time: %1 min").arg(minutes, 0, 'f', 1));
}

void MainWindow::onLogLine(const QString& line) {
    logView_->appendPlainText(line);
}

void MainWindow::onError(const QString& message) {
    statusBar()->showMessage("Error: " + message, 8000);
}

void MainWindow::onRunningChanged(bool running) {
    startBtn_->setEnabled(!running);
    resumeBtn_->setEnabled(!running);
    stepBtn_->setEnabled(!running);
    pauseBtn_->setEnabled(running);
    statusBar()->showMessage(running ? "Running" : "Paused / stopped", 3000);
}

void MainWindow::onReset() {
    logView_->clear();
    onTimeChanged(facade_.time());
}

void MainWindow::runSelectedScenario() {
    const QString name = scenarioBox_->currentText();
    if (facade_.runScenario(name))
        statusBar()->showMessage("Scenario finished: " + name, 5000);
}

} // namespace ers