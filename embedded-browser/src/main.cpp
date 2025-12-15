#include <QApplication>
#include <QIcon>
#include "BrowserWindow.h"

int main(int argc, char *argv[])
{
    // В Qt 5.12 QWebEngine инициализируется автоматически при создании QWebEngineView
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    
    QApplication app(argc, argv);
    QApplication::setApplicationName("Embedded Browser Extension");
    QApplication::setOrganizationName("Vuzhyk");

    BrowserWindow window;
    window.show();

    return app.exec();
}

