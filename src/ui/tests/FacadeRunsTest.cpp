#include <QTemporaryDir>
#include <QtTest>
#include "ers/ui/ApplicationFacade.h"

class FacadeRunsTest : public QObject {
    Q_OBJECT
private slots:
    void saveAndListRuns();
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

QTEST_GUILESS_MAIN(FacadeRunsTest)
#include "FacadeRunsTest.moc"