#include <QApplication>
#include "ers/ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("ERS");
    QApplication::setApplicationName("EmergencyResponseSimulator");
    ers::MainWindow window;
    window.show();
    return app.exec();
}