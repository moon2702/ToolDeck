#include "main_window.h"
#include "sidebar_widget.h"
#include "dashboard_widget.h"
#include "output_panel.h"
#include "input_dialog.h"
#include "core/tool_registry.h"
#include "core/tool_manager.h"
#include "core/tool_instance.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QApplication>

MainWindow::MainWindow(ToolRegistry *registry, ToolManager *manager, QWidget *parent)
    : QMainWindow(parent)
    , registry_(registry)
    , manager_(manager)
{
    setWindowTitle("ToolDeck — 日常工具平台");
    resize(1000, 650);
    setMinimumSize(700, 400);

    setupUi();
    setupMenuBar();
    setupStatusBar();

    // Connect signals
    connect(registry_, &ToolRegistry::toolsChanged,
            this, &MainWindow::onToolsChanged);

    connect(sidebar_, &SidebarWidget::toolSelected,
            this, &MainWindow::onToolActivated);

    connect(sidebar_, &SidebarWidget::runRequested,
            this, &MainWindow::onToolRunRequested);

    connect(sidebar_, &SidebarWidget::stopRequested,
            this, &MainWindow::onToolStopRequested);

    connect(manager_, &ToolManager::instanceCreated,
            this, [this](const QString &name, ToolInstance *inst) {
        // Connect instance output to the output panel
        connect(inst, &ToolInstance::outputReady,
                outputPanel_, [this, name](const QString &text) {
            outputPanel_->appendOutput(name, text);
        });
        connect(inst, &ToolInstance::stateChanged,
                dashboard_, [this, name](ToolState state) {
            dashboard_->updateToolState(name, state);
        });

        // Show the output panel
        outputPanel_->addToolTab(name);
        dashboard_->addToolCard(name, inst);
        mainStack_->setCurrentWidget(outputPanel_);
    });

    // Initial load
    refreshToolList();
}

void MainWindow::setupUi()
{
    // Sidebar (left panel)
    sidebar_ = new SidebarWidget(this);

    // Main content stack (right panel)
    mainStack_ = new QStackedWidget(this);

    // Welcome page
    welcomeLabel_ = new QLabel(this);
    welcomeLabel_->setText(
        "<div style='text-align: center; margin-top: 120px;'>"
        "<h1>🛠️ ToolDeck</h1>"
        "<p style='font-size: 16px; color: #888;'>"
        "日常工具开发平台</p>"
        "<p style='color: #666; margin-top: 30px;'>"
        "从侧边栏选择一个工具开始使用<br/>"
        "或将新工具添加到 <code>~/.config/tooldeck/tools/</code></p>"
        "</div>"
    );
    welcomeLabel_->setAlignment(Qt::AlignCenter);

    // Dashboard page
    dashboard_ = new DashboardWidget(manager_, this);

    // Output page
    outputPanel_ = new OutputPanel(this);

    mainStack_->addWidget(welcomeLabel_);   // index 0: welcome
    mainStack_->addWidget(dashboard_);       // index 1: dashboard
    mainStack_->addWidget(outputPanel_);     // index 2: output tabs

    mainStack_->setCurrentIndex(0);

    // Splitter layout
    splitter_ = new QSplitter(Qt::Horizontal, this);
    splitter_->addWidget(sidebar_);
    splitter_->addWidget(mainStack_);
    splitter_->setStretchFactor(0, 1);  // sidebar
    splitter_->setStretchFactor(1, 3);  // main content
    splitter_->setSizes({220, 780});

    setCentralWidget(splitter_);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(tr("刷新工具列表"), QKeySequence::Refresh, this, [this]() {
        registry_->reload();
    });
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出(&Q)"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    viewMenu->addAction(tr("仪表盘"), this, [this]() {
        mainStack_->setCurrentWidget(dashboard_);
    });
    viewMenu->addAction(tr("输出面板"), this, [this]() {
        mainStack_->setCurrentWidget(outputPanel_);
    });

    QMenu *toolsMenu = menuBar()->addMenu(tr("工具(&T)"));
    toolsMenu->addAction(tr("停止全部工具"), this, [this]() {
        manager_->stopAll();
    });

    QMenu *helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(tr("关于 ToolDeck"), this, [this]() {
        statusLabel_->setText("ToolDeck v0.1.0 — 日常工具平台");
    });
}

void MainWindow::setupStatusBar()
{
    statusLabel_ = new QLabel("就绪");
    statusBar()->addWidget(statusLabel_);
}

void MainWindow::refreshToolList()
{
    sidebar_->clear();

    auto grouped = registry_->toolsByCategory();
    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        for (const auto &tool : it.value()) {
            sidebar_->addTool(tool);
        }
    }
}

void MainWindow::onToolActivated(const QString &toolName)
{
    const auto *manifest = registry_->findByName(toolName);
    if (manifest) {
        statusLabel_->setText(
            QString("工具: %1 | %2 | v%3")
                .arg(manifest->displayName)
                .arg(manifest->description)
                .arg(manifest->version));

        // If the tool is already running, show its output
        auto *inst = manager_->instance(toolName);
        if (inst && inst->state() == ToolState::Running) {
            mainStack_->setCurrentWidget(outputPanel_);
        } else {
            mainStack_->setCurrentWidget(dashboard_);
        }
    }
}

void MainWindow::onToolRunRequested(const QString &toolName)
{
    const auto *manifest = registry_->findByName(toolName);
    if (!manifest) {
        statusLabel_->setText(QString("错误: 未找到工具 '%1'").arg(toolName));
        return;
    }

    QStringList extraArgs;

    // Determine if tool requires user interaction
    bool hasRequiredInputs = false;
    for (const auto &def : manifest->inputs) {
        if (def.required) {
            hasRequiredInputs = true;
            break;
        }
    }

    if (hasRequiredInputs) {
        // ---- 有必填参数 → 弹出智能表单 ----
        InputDialog dlg(*manifest, this);
        if (dlg.exec() != QDialog::Accepted) {
            statusLabel_->setText("已取消运行");
            return;
        }
        extraArgs = manifest->buildArgs(dlg.values());
    } else if (!manifest->inputs.isEmpty()) {
        // ---- 有 inputs 但全是可选的 → 直接用默认值运行 ----
        QHash<QString, QString> defaults;
        for (const auto &def : manifest->inputs) {
            if (!def.defaultValue.isEmpty()) {
                defaults[def.name] = def.defaultValue;
            }
        }
        extraArgs = manifest->buildArgs(defaults);
    }
    // ---- 无 inputs → 直接运行，不弹窗 ----

    auto *inst = manager_->startTool(*manifest, extraArgs);
    if (!inst) {
        statusLabel_->setText(QString("启动工具失败: %1").arg(toolName));
    }
}

void MainWindow::onToolStopRequested(const QString &toolName)
{
    manager_->stopTool(toolName);
}

void MainWindow::onToolsChanged()
{
    refreshToolList();
    statusLabel_->setText(
        QString("已加载 %1 个工具").arg(registry_->count()));
}
