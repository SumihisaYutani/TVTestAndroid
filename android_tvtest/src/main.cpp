#include <QtWidgets/QApplication>
#include <QtCore/QLoggingCategory>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include "mainwindow.h"
#include "utils/Logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Application info
    app.setApplicationName("TVTest Android");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("TVTest");
    app.setOrganizationDomain("tvtest.android");
    
    // Initialize logging system
    Logger::initialize();
    Logger::info("TVTest Android starting...");
    
    // Create main window
    MainWindow window;
    window.show();
    
    Logger::info("TVTest Android UI initialized");
    
    int result = app.exec();
    
    Logger::info("TVTest Android shutting down");
    return result;
}