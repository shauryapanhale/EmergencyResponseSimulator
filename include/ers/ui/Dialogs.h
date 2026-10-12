#pragma once
#include <QDialog>
#include <QString>
#include "ers/common/Types.h"
#include "ers/models/CityMap.h"
#include "ers/ui/ApplicationFacade.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;

namespace ers {

// Collects input only; MainWindow forwards it to ApplicationFacade.
class ReportIncidentDialog : public QDialog {
public:
    ReportIncidentDialog(const CityMap& city, int preselectNode, QWidget* parent = nullptr);
    EmergencyType type() const;
    int nodeId() const;
    Priority priority() const;
    QString description() const;

private:
    QComboBox* typeBox_;
    QComboBox* nodeBox_;
    QComboBox* prioBox_;
    QLineEdit* descEdit_;
};

class RoadDialog : public QDialog {
public:
    explicit RoadDialog(const CityMap& city, QWidget* parent = nullptr);
    int roadId() const;
    double factor() const;
    bool blocked() const;

private:
    void syncFromRoad();

    const CityMap* city_;
    QComboBox* roadBox_;
    QDoubleSpinBox* factorBox_;
    QCheckBox* blockedBox_;
};

// Lists runs stored in the database file; selecting one shows its event log (read-only).
class RunsDialog : public QDialog {
public:
    explicit RunsDialog(ApplicationFacade& facade, QWidget* parent = nullptr);

private:
    ApplicationFacade& facade_;
    QTableWidget* table_;
    QPlainTextEdit* log_;
    std::vector<RunSummary> runs_;
};
class StatsDialog : public QDialog {
public:
    StatsDialog(const StatsView& stats, int unmetNow, QWidget* parent = nullptr);
};

} // namespace ers