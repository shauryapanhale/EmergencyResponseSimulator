#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#include "ers/ui/ApplicationFacade.h"

class FacadeRunsTest : public QObject {
    Q_OBJECT
private slots:
    void saveAndListRuns();
    void playScenarioStepByStep();
    void invalidDatabasePath();
    void defaultPathIsPerUser();
    void exportLogWritesFile();
};

void FacadeRunsTest::saveAndListRuns() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ers::ApplicationFacade facade;
    facade.setDatabasePath(dir.filePath(QStringLiteral("runs.db")));

    QVERIFY(facade.savedRuns().empty());
    QVERIFY(facade.runScenario(QStringLiteral("blocked-road")));
    QCOMPARE(facade.currentScenarioName(), QStringLiteral("blocked-road"));

    const long long first = facade.saveCurrentRun();
    QVERIFY(first > 0);

    facade.reset();
    QCOMPARE(facade.currentScenarioName(), QStringLiteral("manual"));
    const long long second = facade.saveCurrentRun();
    QVERIFY(second > first);

    const auto runs = facade.savedRuns();
    QCOMPARE(static_cast<int>(runs.size()), 2);
    QCOMPARE(runs[0].scenario, QStringLiteral("blocked-road"));
    QCOMPARE(runs[1].scenario, QStringLiteral("manual"));
    QVERIFY(!facade.savedRunLog(first).isEmpty());
}

void FacadeRunsTest::playScenarioStepByStep() {
    ers::ApplicationFacade facade;
    QSignalSpy finished(&facade, &ers::ApplicationFacade::scenarioFinished);

    QVERIFY(!facade.playScenario(QStringLiteral("no-such-scenario")));
    QVERIFY(!facade.isPlayingScenario());

    QVERIFY(facade.playScenario(QStringLiteral("simultaneous-incidents")));
    QVERIFY(facade.isPlayingScenario());
    QCOMPARE(facade.currentScenarioName(), QStringLiteral("simultaneous-incidents"));
    QCOMPARE(static_cast<int>(facade.incidents().size()), 6);
    QCOMPARE(facade.time(), 0.0);

    facade.stepOnce();
    QCOMPARE(facade.time(), 1.0);
    QCOMPARE(finished.count(), 0);

    for (int i = 0; i < 200 && finished.count() == 0; ++i) facade.stepOnce();
    QCOMPARE(finished.count(), 1);
    QVERIFY(!facade.isPlayingScenario());
    QVERIFY(facade.time() >= 90.0 - 1e-6);

    facade.reset();
    QVERIFY(!facade.isPlayingScenario());
    QCOMPARE(facade.time(), 0.0);
}

void FacadeRunsTest::invalidDatabasePath() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ers::ApplicationFacade facade;
    QSignalSpy errors(&facade, &ers::ApplicationFacade::errorOccurred);
    facade.setDatabasePath(dir.filePath(QStringLiteral("no_such_dir/runs.db")));

    QVERIFY(facade.reportIncident(ers::EmergencyType::Medical, 7, ers::Priority::High, QStringLiteral("test")) > 0);
    const double before = facade.time();

    QCOMPARE(facade.saveCurrentRun(), static_cast<long long>(-1));
    QVERIFY(errors.count() >= 1);
    QVERIFY(facade.savedRuns().empty());
    QVERIFY(facade.savedRunLog(1).isEmpty());

    QCOMPARE(facade.time(), before);
    QCOMPARE(static_cast<int>(facade.incidents().size()), 1);
}
void FacadeRunsTest::defaultPathIsPerUser() {
    QStandardPaths::setTestModeEnabled(true);
    ers::ApplicationFacade facade;
    const QString path = facade.databasePath();
    QVERIFY(QFileInfo(path).isAbsolute());
    QVERIFY(path.endsWith(QStringLiteral("ers_gui.db")));
    QVERIFY(QFileInfo(path).dir().exists());

    QFile::remove(path);
    QVERIFY(facade.saveCurrentRun() > 0);
    QCOMPARE(static_cast<int>(facade.savedRuns().size()), 1);
    QFile::remove(path);
}
void FacadeRunsTest::exportLogWritesFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ers::ApplicationFacade facade;
    QSignalSpy errors(&facade, &ers::ApplicationFacade::errorOccurred);

    QVERIFY(facade.reportIncident(ers::EmergencyType::Medical, 7, ers::Priority::High, QStringLiteral("export test")) > 0);
    const QString path = dir.filePath(QStringLiteral("event_log.txt"));
    QVERIFY(facade.exportLog(path));
    QCOMPARE(errors.count(), 0);

    QFile file(path);
    QVERIFY(file.exists());
    QVERIFY(file.size() > 0);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(QString::fromUtf8(file.readAll()).contains(QStringLiteral("Incident")));
    file.close();

    QVERIFY(!facade.exportLog(dir.filePath(QStringLiteral("no_such_dir/log.txt"))));
    QVERIFY(errors.count() >= 1);
    QVERIFY(!facade.exportLog(QString()));
}
QTEST_GUILESS_MAIN(FacadeRunsTest)
#include "FacadeRunsTest.moc"