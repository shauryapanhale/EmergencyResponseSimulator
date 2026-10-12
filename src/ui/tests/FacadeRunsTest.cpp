#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include "ers/ui/ApplicationFacade.h"

class FacadeRunsTest : public QObject {
    Q_OBJECT
private slots:
    void saveAndListRuns();
    void playScenarioStepByStep();
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

QTEST_GUILESS_MAIN(FacadeRunsTest)
#include "FacadeRunsTest.moc"