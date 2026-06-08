#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

#include "core/tool_registry.h"
#include "core/tool_manager.h"
#include "ui/main_window.h"

static void ensureToolDirectories()
{
    // Create the user's tool directory if it doesn't exist
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QString toolsPath = configPath + "/tooldeck/tools";
    QDir dir;
    if (!dir.exists(toolsPath)) {
        dir.mkpath(toolsPath);
        qDebug() << "Created tools directory:" << toolsPath;
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ToolDeck");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("tooldeck");

    // Ensure the tool infrastructure exists
    ensureToolDirectories();

    // Set up the tool registry and manager
    ToolRegistry registry;
    ToolManager manager;

    // Add project-local examples directory as additional search path
    QString appDir = QCoreApplication::applicationDirPath();
    QString examplesPath = appDir + "/../examples";
    if (QDir(examplesPath).exists()) {
        registry.addSearchPath(examplesPath);
    }
    // Also try relative to source during development
    QString devExamplesPath = appDir + "/../../examples";
    if (QDir(devExamplesPath).exists()) {
        registry.addSearchPath(devExamplesPath);
    }

    // Initial tool scan
    registry.reload();

    // Show the main window
    MainWindow window(&registry, &manager);
    window.show();

    return app.exec();
}
