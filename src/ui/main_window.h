#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QLabel>

class ToolRegistry;
class ToolManager;
class SidebarWidget;
class DashboardWidget;
class OutputPanel;

/// Main application window.
/// Layout: Sidebar (left) | Main area (dashboard + output tabs, right)
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ToolRegistry *registry, ToolManager *manager, QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onToolActivated(const QString &toolName);
    void onToolRunRequested(const QString &toolName);
    void onToolStopRequested(const QString &toolName);
    void onToolsChanged();

private:
    void setupUi();
    void setupMenuBar();
    void setupStatusBar();
    void refreshToolList();

    ToolRegistry *registry_;
    ToolManager *manager_;

    // UI components
    QSplitter *splitter_;
    SidebarWidget *sidebar_;
    QStackedWidget *mainStack_;
    DashboardWidget *dashboard_;
    OutputPanel *outputPanel_;
    QLabel *statusLabel_;
    QLabel *welcomeLabel_;
};

#endif // MAIN_WINDOW_H
