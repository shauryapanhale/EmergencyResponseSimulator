#include <QApplication>
#include "ers/ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    ers::MainWindow window;
    window.show();
    return app.exec();
}