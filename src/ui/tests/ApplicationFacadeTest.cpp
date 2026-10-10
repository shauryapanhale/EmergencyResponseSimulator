#include <QSignalSpy>
#include <QtTest>
#include "ers/ui/ApplicationFacade.h"

using namespace ers;

class ApplicationFacadeTest : public QObject {
    Q_OBJECT
private slots:
    void initialState();
    void reportIncidentEmitsSignals();
    void invalidInputEmitsErrors();
    void stepAdvancesTime();
    void timerDrivesSteps();
    void resolveFlow();
    void resetRestoresCleanCity();
    void scenarioRuns();
};

void ApplicationFacadeTest::initialState() {
    ApplicationFacade f;
    QCOMPARE(f.time(), 0.0);
    QVERIFY(!f.isRunning());
    QCOMPARE(int(f.vehicles().size()), 6);
    QVERIFY(f.incidents().empty());
    QCOMPARE(int(f.city().nodes().size()), 12);
    QCOMPARE(f.scenarioNames().size(), 4);
    QVERIFY(ApplicationFacade::disclaimer().contains("not for real emergency use"));
}

void ApplicationFacadeTest::reportIncidentEmitsSignals() {
    ApplicationFacade f;
    QSignalSpy state(&f, &ApplicationFacade::stateChanged);
    QSignalSpy log(&f, &ApplicationFacade::logLine);

    int id = f.reportIncident(EmergencyType::Medical, 7, Priority::High, "fall in Old Town");
    QCOMPARE(id, 1);
    QVERIFY(state.count() >= 1);
    QVERIFY(log.count() >= 3);

    auto inc = f.incidents();
    QCOMPARE(int(inc.size()), 1);
    QCOMPARE(inc[0].status, QString("Dispatched"));
    QCOMPARE(inc[0].nodeName, QString("Old Town"));
    QCOMPARE(inc[0].vehicleIds.size(), 1);
    QVERIFY(!inc[0].explanation.isEmpty());

    int enRoute = 0;
    for (const auto& v : f.vehicles())
        if (v.status == "EnRoute") ++enRoute;
    QCOMPARE(enRoute, 1);
}

void ApplicationFacadeTest::invalidInputEmitsErrors() {
    ApplicationFacade f;
    QSignalSpy err(&f, &ApplicationFacade::errorOccurred);

    QCOMPARE(f.reportIncident(EmergencyType::Medical, 999, Priority::High, "x"), -1);
    QCOMPARE(err.count(), 1);
    QVERIFY(!err.first().at(0).toString().isEmpty());

    QVERIFY(!f.resolveIncident(42));
    QVERIFY(!f.setBlocked(999, true));
    QVERIFY(!f.setTraffic(1, -1.0));
    QCOMPARE(err.count(), 4);
    QVERIFY(f.incidents().empty());
}

void ApplicationFacadeTest::stepAdvancesTime() {
    ApplicationFacade f;
    QSignalSpy t(&f, &ApplicationFacade::timeChanged);
    f.stepOnce();
    QCOMPARE(f.time(), 1.0);
    QCOMPARE(t.count(), 1);
    QCOMPARE(t.takeFirst().at(0).toDouble(), 1.0);
}

void ApplicationFacadeTest::timerDrivesSteps() {
    ApplicationFacade f;
    QSignalSpy running(&f, &ApplicationFacade::runningChanged);
    f.setTickIntervalMs(10);
    f.start();
    QVERIFY(f.isRunning());
    QTRY_VERIFY_WITH_TIMEOUT(f.time() >= 3.0, 3000);

    f.pause();
    QVERIFY(!f.isRunning());
    const double frozen = f.time();
    QTest::qWait(80);
    QCOMPARE(f.time(), frozen);

    f.resume();
    QVERIFY(f.isRunning());
    QTRY_VERIFY_WITH_TIMEOUT(f.time() > frozen, 3000);
    f.pause();
    QCOMPARE(running.count(), 4);
}

void ApplicationFacadeTest::resolveFlow() {
    ApplicationFacade f;
    int id = f.reportIncident(EmergencyType::Medical, 7, Priority::High, "fall");
    QVERIFY(id > 0);
    for (int i = 0; i < 10; ++i) f.stepOnce();
    QCOMPARE(f.incidents()[0].status, QString("OnScene"));

    QVERIFY(f.resolveIncident(id));
    QCOMPARE(f.incidents()[0].status, QString("Resolved"));
    QCOMPARE(f.stats().resolved, 1);

    QSignalSpy err(&f, &ApplicationFacade::errorOccurred);
    QVERIFY(!f.resolveIncident(id));
    QCOMPARE(err.count(), 1);
}

void ApplicationFacadeTest::resetRestoresCleanCity() {
    ApplicationFacade f;
    f.reportIncident(EmergencyType::Fire, 9, Priority::Critical, "fire");
    QVERIFY(f.setBlocked(1, true));
    QVERIFY(f.city().road(1).isBlocked());
    f.stepOnce();

    QSignalSpy resetSpy(&f, &ApplicationFacade::simulationReset);
    f.reset();
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(f.time(), 0.0);
    QVERIFY(f.incidents().empty());
    QVERIFY(!f.city().road(1).isBlocked());
    for (const auto& v : f.vehicles()) QCOMPARE(v.status, QString("Available"));
}

void ApplicationFacadeTest::scenarioRuns() {
    ApplicationFacade f;
    f.setAutoResolveMinutes(3.0);
    QSignalSpy done(&f, &ApplicationFacade::scenarioFinished);
    QVERIFY(f.runScenario("blocked-road"));
    QCOMPARE(done.count(), 1);
    QVERIFY(f.time() >= 60.0);
    QCOMPARE(f.stats().created, 3);
    QCOMPARE(f.stats().resolved, 3);

    QSignalSpy err(&f, &ApplicationFacade::errorOccurred);
    QVERIFY(!f.runScenario("no-such-scenario"));
    QCOMPARE(err.count(), 1);
    QCOMPARE(f.stats().created, 3);  // failed lookup leaves the previous run untouched
}

QTEST_GUILESS_MAIN(ApplicationFacadeTest)
#include "ApplicationFacadeTest.moc"