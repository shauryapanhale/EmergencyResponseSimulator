#include "ers/ui/Dialogs.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

namespace ers {

namespace {
QDialogButtonBox* addButtons(QDialog* dlg, QDialogButtonBox::StandardButtons buttons) {
    auto* box = new QDialogButtonBox(buttons, dlg);
    QObject::connect(box, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    QObject::connect(box, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    return box;
}
} // namespace

ReportIncidentDialog::ReportIncidentDialog(const CityMap& city, int preselectNode, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Report incident");
    auto* form = new QFormLayout(this);

    typeBox_ = new QComboBox(this);
    typeBox_->addItem("Medical", static_cast<int>(EmergencyType::Medical));
    typeBox_->addItem("Fire", static_cast<int>(EmergencyType::Fire));
    typeBox_->addItem("Road accident", static_cast<int>(EmergencyType::RoadAccident));
    typeBox_->addItem("Crime", static_cast<int>(EmergencyType::Crime));

    nodeBox_ = new QComboBox(this);
    for (const auto& entry : city.nodes()) {
        const CityNode& n = entry.second;
        nodeBox_->addItem(QString("%1 - %2").arg(n.id()).arg(QString::fromStdString(n.name())), n.id());
    }
    if (preselectNode >= 0) {
        const int idx = nodeBox_->findData(preselectNode);
        if (idx >= 0) nodeBox_->setCurrentIndex(idx);
    }

    prioBox_ = new QComboBox(this);
    prioBox_->addItem("Critical", static_cast<int>(Priority::Critical));
    prioBox_->addItem("High", static_cast<int>(Priority::High));
    prioBox_->addItem("Medium", static_cast<int>(Priority::Medium));
    prioBox_->addItem("Low", static_cast<int>(Priority::Low));
    prioBox_->setCurrentIndex(1);

    descEdit_ = new QLineEdit(this);
    descEdit_->setPlaceholderText("Short description (optional)");

    form->addRow("Type:", typeBox_);
    form->addRow("Location:", nodeBox_);
    form->addRow("Priority:", prioBox_);
    form->addRow("Description:", descEdit_);
    form->addRow(new QLabel("Educational simulation only - not for real emergency use.", this));
    form->addRow(addButtons(this, QDialogButtonBox::Ok | QDialogButtonBox::Cancel));
}

EmergencyType ReportIncidentDialog::type() const {
    return static_cast<EmergencyType>(typeBox_->currentData().toInt());
}
int ReportIncidentDialog::nodeId() const { return nodeBox_->currentData().toInt(); }
Priority ReportIncidentDialog::priority() const {
    return static_cast<Priority>(prioBox_->currentData().toInt());
}
QString ReportIncidentDialog::description() const {
    const QString text = descEdit_->text().trimmed();
    return text.isEmpty() ? QString("Reported from GUI") : text;
}

RoadDialog::RoadDialog(const CityMap& city, QWidget* parent) : QDialog(parent), city_(&city) {
    setWindowTitle("Traffic and road closures");
    auto* form = new QFormLayout(this);

    roadBox_ = new QComboBox(this);
    for (const auto& entry : city.roads()) {
        const Road& r = entry.second;
        const QString from = city.hasNode(r.from()) ? QString::fromStdString(city.node(r.from()).name()) : "?";
        const QString to = city.hasNode(r.to()) ? QString::fromStdString(city.node(r.to()).name()) : "?";
        roadBox_->addItem(QString("R%1: %2 - %3 (%4 min base)").arg(r.id()).arg(from, to).arg(r.baseMinutes()),
                          r.id());
    }

    factorBox_ = new QDoubleSpinBox(this);
    factorBox_->setRange(0.5, 5.0);
    factorBox_->setSingleStep(0.5);
    factorBox_->setDecimals(1);
    factorBox_->setValue(1.0);

    blockedBox_ = new QCheckBox("Road blocked", this);

    form->addRow("Road:", roadBox_);
    form->addRow("Traffic factor:", factorBox_);
    form->addRow("", blockedBox_);
    form->addRow(new QLabel("1.0 normal, 1.5 heavy, 2.0 severe. Only new dispatches use the change.", this));
    form->addRow(addButtons(this, QDialogButtonBox::Ok | QDialogButtonBox::Cancel));

    connect(roadBox_, &QComboBox::currentIndexChanged, this, [this](int) { syncFromRoad(); });
    syncFromRoad();
}

void RoadDialog::syncFromRoad() {
    const int id = roadBox_->currentData().toInt();
    if (!city_->hasRoad(id)) return;
    const Road& r = city_->road(id);
    factorBox_->setValue(r.trafficFactor());
    blockedBox_->setChecked(r.isBlocked());
}

int RoadDialog::roadId() const { return roadBox_->currentData().toInt(); }
double RoadDialog::factor() const { return factorBox_->value(); }
bool RoadDialog::blocked() const { return blockedBox_->isChecked(); }

StatsDialog::StatsDialog(const StatsView& s, int unmetNow, QWidget* parent) : QDialog(parent) {
    setWindowTitle("Statistics");
    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    auto num = [](double v) { return QString::number(v, 'f', 2) + " min"; };

    form->addRow("Incidents created:", new QLabel(QString::number(s.created)));
    form->addRow("Resolved:", new QLabel(QString::number(s.resolved)));
    form->addRow("Active:", new QLabel(QString::number(s.active)));
    form->addRow("Pending:", new QLabel(QString::number(s.pending)));
    form->addRow("Open with unmet resources now:", new QLabel(QString::number(unmetNow)));
    form->addRow("Resolved with unmet resources:", new QLabel(QString::number(s.resolvedWithUnmet)));
    form->addRow("Responses (arrivals):", new QLabel(QString::number(s.responseCount)));
    form->addRow("Average response:", new QLabel(num(s.avgResponse)));
    form->addRow("Minimum response:", new QLabel(num(s.minResponse)));
    form->addRow("Maximum response:", new QLabel(num(s.maxResponse)));
    form->addRow("Average waiting:", new QLabel(num(s.avgWaiting)));
    form->addRow("Average resolution:", new QLabel(num(s.avgResolution)));
    layout->addLayout(form);

    auto* note = new QLabel(
        "Note: an incident resolved while a resource type was still unmet counts as resolved. "
        "The row Resolved with unmet resources counts those.\nSimulated estimates only.");
    note->setWordWrap(true);
    layout->addWidget(note);
    layout->addWidget(addButtons(this, QDialogButtonBox::Close));
}

} // namespace ers