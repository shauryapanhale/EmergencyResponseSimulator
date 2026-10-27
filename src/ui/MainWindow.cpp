#include "ers/ui/MainWindow.h"

#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "ers/ui/Dialogs.h"
#include "ers/ui/MapView.h"

namespace ers {

namespace {
QTableWidget* makeTable(const QStringList& headers) {
    auto* t = new QTableWidget();
    t->setColumnCount(headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setAlternatingRowColors(true);
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setStretchLastSection(true);
    return t;
}

const char* kLegendHtml =
    "<b>Map controls</b><br>"
    "Wheel: zoom<br>"
    "Drag: pan<br>"
    "Double-click: zoom in<br>"
    "Click a node: select it<br>"
    "(Report incident uses it)<br><br>"
    "<b>Nodes</b><br>"
    "&#9632; Station (square)<br>"
    "&#9670; Landmark (diamond)<br>"
    "&#9679; Intersection (circle)<br><br>"
    "<b>Roads</b><br>"
    "Gray: normal traffic<br>"
    "Yellow: heavy traffic<br>"
    "Dark orange: severe traffic<br>"
    "Red dashed: blocked<br>"
    "R + number: road id<br><br>"
    "<b>Markers</b><br>"
    "Red ring: open incident<br>"
    "Thick ring: Critical<br>"
    "A / F / P + id: Ambulance,<br>"
    "Fire truck, Police vehicle<br><br>"
    "Hover over items for details.";

bool isOpen(const QString& status) { return status != "Resolved" && status != "Unserved"; }
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
    connect(resetBtn_, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(scenarioBtn_, &QPushButton::clicked, this, &MainWindow::runSelectedScenario);
    connect(playBtn_, &QPushButton::clicked, this, &MainWindow::onPlayScenario);
    connect(reportBtn_, &QPushButton::clicked, this, &MainWindow::onReportIncident);
    connect(roadBtn_, &QPushButton::clicked, this, &MainWindow::onRoadDialog);
    connect(statsBtn_, &QPushButton::clicked, this, &MainWindow::onShowStats);
    connect(saveBtn_, &QPushButton::clicked, this, &MainWindow::onSaveRun);
    connect(exportBtn_, &QPushButton::clicked, this, &MainWindow::onExportLog);
    connect(runsBtn_, &QPushButton::clicked, this, &MainWindow::onShowRuns);
    connect(resolveBtn_, &QPushButton::clicked, this, &MainWindow::onResolve);
    connect(priorityBtn_, &QPushButton::clicked, this, &MainWindow::onChangePriority);
    connect(incidentTable_, &QTableWidget::itemSelectionChanged, this, &MainWindow::updateDetails);

    connect(speedBox_, &QComboBox::currentIndexChanged, this, [this](int index) {
        static const int intervalsMs[] = {500, 250, 100, 50};
        if (index >= 0 && index < 4) facade_.setTickIntervalMs(intervalsMs[index]);
    });
    connect(&facade_, &ApplicationFacade::scenarioFinished, this, [this](const QString& name) {
        statusBar()->showMessage("Scenario finished: " + name + ". Use Save run to keep it.", 8000);
    });

    auto bindKey = [this](const QKeySequence& key, QPushButton* button, const QString& tip) {
        auto* shortcut = new QShortcut(key, this);
        connect(shortcut, &QShortcut::activated, button, &QPushButton::click);
        button->setToolTip(tip + " (" + key.toString() + ")");
    };
    bindKey(QKeySequence(Qt::Key_F5), startBtn_, "Start");
    bindKey(QKeySequence(Qt::Key_F6), pauseBtn_, "Pause");
    bindKey(QKeySequence(Qt::Key_F7), stepBtn_, "Step one tick");
    bindKey(QKeySequence("Ctrl+R"), reportBtn_, "Report incident");
    bindKey(QKeySequence("Ctrl+S"), saveBtn_, "Save run");

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
    reportBtn_ = new QPushButton("Report incident...");
    roadBtn_ = new QPushButton("Traffic / roads...");
    statsBtn_ = new QPushButton("Statistics...");
    saveBtn_ = new QPushButton("Save run");
    exportBtn_ = new QPushButton("Export log...");
    runsBtn_ = new QPushButton("Saved runs...");
    controls->addWidget(startBtn_);
    controls->addWidget(pauseBtn_);
    controls->addWidget(resumeBtn_);
    controls->addWidget(stepBtn_);
    controls->addWidget(resetBtn_);
    controls->addSpacing(8);
    controls->addWidget(new QLabel("Speed:"));
    speedBox_ = new QComboBox();
    speedBox_->addItems({"1x", "2x", "5x", "10x"});
    controls->addWidget(speedBox_);
    controls->addSpacing(16);
    controls->addWidget(reportBtn_);
    controls->addWidget(roadBtn_);
    controls->addWidget(statsBtn_);
    controls->addWidget(saveBtn_);
    controls->addWidget(runsBtn_);
    controls->addWidget(exportBtn_);
    controls->addStretch();
    controls->addWidget(new QLabel("Scenario:"));
    scenarioBox_ = new QComboBox();
    scenarioBox_->addItems(facade_.scenarioNames());
    scenarioBtn_ = new QPushButton("Run instantly");
    playBtn_ = new QPushButton("Play scenario");
    controls->addWidget(scenarioBox_);
    controls->addWidget(scenarioBtn_);
    controls->addWidget(playBtn_);
    root->addLayout(controls);

    mapView_ = new MapView(facade_);
    connect(mapView_, &MapView::nodeClicked, this, [this](int id) {
        if (!facade_.city().hasNode(id)) return;
        const CityNode& n = facade_.city().node(id);
        statusBar()->showMessage(
            QString("Selected node %1: %2 (Report incident will use it)")
                .arg(id)
                .arg(QString::fromStdString(n.name())),
            6000);
    });

    auto* legend = new QLabel(QString::fromLatin1(kLegendHtml));
    legend->setTextFormat(Qt::RichText);
    legend->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    legend->setFixedWidth(230);
    legend->setStyleSheet(
        "QLabel{color:#222222; background:#fafafa; border:1px solid #cccccc; padding:6px;}");
    auto* mapRow = new QWidget();
    auto* mapLay = new QHBoxLayout(mapRow);
    mapLay->setContentsMargins(0, 0, 0, 0);
    mapLay->addWidget(mapView_, 1);
    mapLay->addWidget(legend);

    incidentTable_ = makeTable({"ID", "Type", "Priority", "Status", "Location", "Vehicles", "Unmet"});
    vehicleTable_ = makeTable({"ID", "Type", "Status", "Station", "Node", "Incident"});

    resolveBtn_ = new QPushButton("Resolve selected");
    priorityBtn_ = new QPushButton("Change priority...");

    auto* tables = new QSplitter(Qt::Horizontal);
    auto* incBox = new QWidget();
    auto* incLay = new QVBoxLayout(incBox);
    incLay->setContentsMargins(0, 0, 0, 0);
    incLay->addWidget(new QLabel("Incidents"));
    incLay->addWidget(incidentTable_);
    auto* actions = new QHBoxLayout();
    actions->addWidget(resolveBtn_);
    actions->addWidget(priorityBtn_);
    actions->addStretch();
    incLay->addLayout(actions);

    auto* vehBox = new QWidget();
    auto* vehLay = new QVBoxLayout(vehBox);
    vehLay->setContentsMargins(0, 0, 0, 0);
    vehLay->addWidget(new QLabel("Vehicles"));
    vehLay->addWidget(vehicleTable_);

    detailsView_ = new QPlainTextEdit();
    detailsView_->setReadOnly(true);
    auto* detBox = new QWidget();
    auto* detLay = new QVBoxLayout(detBox);
    detLay->setContentsMargins(0, 0, 0, 0);
    detLay->addWidget(new QLabel("Dispatch details"));
    detLay->addWidget(detailsView_);

    tables->addWidget(incBox);
    tables->addWidget(vehBox);
    tables->addWidget(detBox);
    tables->setStretchFactor(0, 3);
    tables->setStretchFactor(1, 2);
    tables->setStretchFactor(2, 2);

    logView_ = new QPlainTextEdit();
    logView_->setReadOnly(true);
    logView_->setMaximumBlockCount(5000);

    auto* vertical = new QSplitter(Qt::Vertical);
    vertical->addWidget(mapRow);
    vertical->addWidget(tables);
    auto* logBox = new QWidget();
    auto* logLay = new QVBoxLayout(logBox);
    logLay->setContentsMargins(0, 0, 0, 0);
    logLay->addWidget(new QLabel("Event log"));
    logLay->addWidget(logView_);
    vertical->addWidget(logBox);
    vertical->setStretchFactor(0, 4);
    vertical->setStretchFactor(1, 2);
    vertical->setStretchFactor(2, 1);
    root->addWidget(vertical, 1);

    setCentralWidget(central);

    timeLabel_ = new QLabel();
    statusBar()->addPermanentWidget(timeLabel_);
    statusBar()->showMessage("Ready. Simulated estimates only.");
    resize(1300, 900);
}

int MainWindow::selectedIncidentId() const {
    const auto items = incidentTable_->selectedItems();
    if (items.isEmpty()) return -1;
    const QTableWidgetItem* idItem = incidentTable_->item(items.first()->row(), 0);
    return idItem ? idItem->text().toInt() : -1;
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
    updateDetails();
}

void MainWindow::updateDetails() {
    const int id = selectedIncidentId();
    resolveBtn_->setEnabled(id >= 0);
    priorityBtn_->setEnabled(id >= 0);
    if (id < 0) {
        detailsView_->setPlainText("Select an incident to see its dispatch details.");
        return;
    }
    for (const auto& i : facade_.incidents()) {
        if (i.id != id) continue;
        QStringList vehicles;
        for (int v : i.vehicleIds) vehicles << QString::number(v);
        QString text;
        text += QString("Incident #%1\n").arg(i.id);
        text += QString("Type: %1\nPriority: %2\nStatus: %3\n").arg(i.type, i.priority, i.status);
        text += QString("Location: %1 (node %2)\n").arg(i.nodeName).arg(i.nodeId);
        text += QString("Created at: %1 min\n").arg(i.createdAt, 0, 'f', 1);
        text += QString("Description: %1\n").arg(i.description);
        text += QString("Vehicles: %1\n").arg(vehicles.isEmpty() ? QString("none") : vehicles.join(", "));
        text += QString("Unmet resources: %1\n\n").arg(i.unmet.isEmpty() ? QString("none") : i.unmet);
        text += "Dispatch reasoning:\n" + facade_.explanationFor(id);
        detailsView_->setPlainText(text);
        return;
    }
    detailsView_->setPlainText("Incident not found.");
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
    if (!confirmDiscard()) return;
    if (facade_.runScenario(name))
        statusBar()->showMessage("Scenario finished: " + name, 5000);
}

void MainWindow::onReportIncident() {
    ReportIncidentDialog dlg(facade_.city(), mapView_->selectedNode(), this);
    if (dlg.exec() != QDialog::Accepted) return;
    const int id = facade_.reportIncident(dlg.type(), dlg.nodeId(), dlg.priority(), dlg.description());
    if (id >= 0) statusBar()->showMessage(QString("Incident %1 reported").arg(id), 5000);
}

void MainWindow::onRoadDialog() {
    RoadDialog dlg(facade_.city(), this);
    if (dlg.exec() != QDialog::Accepted) return;
    if (!facade_.setTraffic(dlg.roadId(), dlg.factor())) return;
    if (!facade_.setBlocked(dlg.roadId(), dlg.blocked())) return;
    statusBar()->showMessage(QString("Road %1 updated: traffic x%2%3")
                                 .arg(dlg.roadId())
                                 .arg(dlg.factor(), 0, 'f', 1)
                                 .arg(dlg.blocked() ? ", blocked" : ""),
                             5000);
}

void MainWindow::onResolve() {
    const int id = selectedIncidentId();
    if (id < 0) return;
    if (facade_.resolveIncident(id))
        statusBar()->showMessage(QString("Incident %1 resolved").arg(id), 5000);
}

void MainWindow::onChangePriority() {
    const int id = selectedIncidentId();
    if (id < 0) return;
    const QStringList levels = {"Critical", "High", "Medium", "Low"};
    bool ok = false;
    const QString choice = QInputDialog::getItem(this, "Change priority",
                                                 QString("New priority for incident %1:").arg(id),
                                                 levels, 1, false, &ok);
    if (!ok) return;
    const int index = levels.indexOf(choice);
    if (index < 0) return;
    if (facade_.changePriority(id, static_cast<Priority>(index)))
        statusBar()->showMessage(QString("Incident %1 priority set to %2").arg(id).arg(choice), 5000);
}

void MainWindow::onSaveRun() {
    const long long id = facade_.saveCurrentRun();
    if (id >= 0)
        statusBar()->showMessage(QString("Run saved as #%1 (scenario: %2) in %3")
                                     .arg(id)
                                     .arg(facade_.currentScenarioName(), facade_.databasePath()),
                                 8000);
}

void MainWindow::onExportLog() {
    const QString suggested = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                  .filePath("event_log.txt");
    const QString path = QFileDialog::getSaveFileName(this, "Export event log", suggested,
                                                      "Text files (*.txt);;All files (*)");
    if (path.isEmpty()) return;  // cancelled
    if (facade_.exportLog(path))
        statusBar()->showMessage("Event log exported to " + path, 8000);
}
void MainWindow::onShowRuns() {
    RunsDialog dlg(facade_, this);
    dlg.exec();
}
bool MainWindow::confirmDiscard() {
    if (facade_.incidents().empty()) return true;
    const auto answer = QMessageBox::question(
        this, "Discard current run?",
        "The current run is lost unless you used Save run. Discard it and continue?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void MainWindow::onResetClicked() {
    if (!confirmDiscard()) return;
    facade_.reset();
}

void MainWindow::onPlayScenario() {
    if (!confirmDiscard()) return;
    const QString name = scenarioBox_->currentText();
    if (facade_.playScenario(name))
        statusBar()->showMessage("Scenario loaded: " + name + ". Press Start (F5) or Step (F7).", 8000);
}

void MainWindow::onShowStats() {
    int unmetNow = 0;
    for (const auto& i : facade_.incidents())
        if (isOpen(i.status) && !i.unmet.isEmpty()) ++unmetNow;
    StatsDialog dlg(facade_.stats(), unmetNow, this);
    dlg.exec();
}

} // namespace ers