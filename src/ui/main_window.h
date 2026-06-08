#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QLabel>

class ToolRegistry;
class ToolManager;
class ToolManifest;
class ToolInstance;
class SidebarWidget;
class DashboardWidget;
class OutputPanel;

/// Main application window.
/// Layout: Sidebar (left) | Main area (dashboard + output, right)
///
/// Two operating modes:
///   Tab     — normal single-tool usage
///   Compare — side-by-side comparison (dual-input form → left/right output)
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ToolRegistry *registry, ToolManager *manager, QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onModeChanged(int index);
    void onToolActivated(const QString &toolName);
    void onToolRunRequested(const QString &toolName);
    void onToolStopRequested(const QString &toolName);
    void onToolsChanged();

private:
    void setupUi();
    void setupMenuBar();
    void setupStatusBar();
    void refreshToolList();

    // Mode-dependent launch paths
    void runNormal(const ToolManifest &manifest);
    void runCompare(const ToolManifest &manifest);

    // Wire a ToolInstance to output (tab mode)
    void wireInstanceToTab(ToolInstance *inst, const QString &toolName);

    // Wire a ToolInstance to left/right compare panel
    void wireInstanceToLeft(ToolInstance *inst);
    void wireInstanceToRight(ToolInstance *inst);

    ToolRegistry *registry_;
    ToolManager *manager_;

    // UI
    QSplitter *splitter_;
    SidebarWidget *sidebar_;
    QStackedWidget *mainStack_;
    DashboardWidget *dashboard_;
    OutputPanel *outputPanel_;
    QLabel *statusLabel_;
    QLabel *welcomeLabel_;

    // Current mode: false=Tab, true=Compare
    bool compareMode_ = false;

    // Flag: skip default instanceCreated → tab wiring (used in compare mode)
    bool skipDefaultWiring_ = false;
};

#endif // MAIN_WINDOW_H
