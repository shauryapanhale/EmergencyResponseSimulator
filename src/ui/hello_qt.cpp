#include <QApplication>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QTimer>
#include "ers/repositories/SampleCity.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    ers::CityData city = ers::buildSampleCity();
    const QString text = QString("Emergency Response Simulator\n\n"
                                 "Sample city: %1 nodes, %2 roads, %3 stations\n\n"
                                 "Educational simulation only - not for real emergency use.")
                             .arg(city.map.nodes().size())
                             .arg(city.map.roads().size())
                             .arg(city.stations.size());

    QLabel label(text);
    label.setAlignment(Qt::AlignCenter);
    label.setMinimumSize(460, 180);
    label.setWindowTitle("ERS - Qt smoke test");
    label.show();

    if (app.arguments().contains("--smoke")) {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }
    return app.exec();
}