#include "main_window.h"
#include "sidebar_widget.h"
#include "dashboard_widget.h"
#include "output_panel.h"
#include "input_dialog.h"
#include "compare_input_dialog.h"
#include "core/tool_registry.h"
#include "core/tool_manager.h"
#include "core/tool_instance.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QApplication>
#include <QActionGroup>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QSet>

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

    // ---- Signal wiring ----
    connect(registry_, &ToolRegistry::toolsChanged, this, &MainWindow::onToolsChanged);
    connect(sidebar_, &SidebarWidget::toolSelected, this, &MainWindow::onToolActivated);
    connect(sidebar_, &SidebarWidget::runRequested, this, &MainWindow::onToolRunRequested);
    connect(sidebar_, &SidebarWidget::stopRequested, this, &MainWindow::onToolStopRequested);

    connect(manager_, &ToolManager::instanceCreated, this,
            [this](const QString &name, ToolInstance *inst) {
        if (skipDefaultWiring_) return; // Compare mode handles wiring separately
        wireInstanceToTab(inst, name);
        dashboard_->addToolCard(name, inst);
        mainStack_->setCurrentWidget(outputPanel_);
    });

    refreshToolList();
}

void MainWindow::setupUi()
{
    sidebar_ = new SidebarWidget(this);
    mainStack_ = new QStackedWidget(this);

    // Welcome page
    welcomeLabel_ = new QLabel(this);
    welcomeLabel_->setText(
        "<div style='text-align: center; margin-top: 120px;'>"
        "<h1>🛠️ ToolDeck</h1>"
        "<p style='font-size: 16px; color: #888;'>日常工具开发平台</p>"
        "<p style='color: #666; margin-top: 30px;'>"
        "从侧边栏选择一个工具开始使用<br/>"
        "或通过 <b>文件 → 打开工具目录</b> 添加自定义工具</p>"
        "</div>"
    );
    welcomeLabel_->setAlignment(Qt::AlignCenter);

    dashboard_ = new DashboardWidget(manager_, this);
    outputPanel_ = new OutputPanel(this);

    mainStack_->addWidget(welcomeLabel_);
    mainStack_->addWidget(dashboard_);
    mainStack_->addWidget(outputPanel_);
    mainStack_->setCurrentIndex(0);

    // Splitter: sidebar | main
    splitter_ = new QSplitter(Qt::Horizontal, this);
    splitter_->addWidget(sidebar_);
    splitter_->addWidget(mainStack_);
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 3);
    splitter_->setSizes({220, 780});

    setCentralWidget(splitter_);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(tr("刷新工具列表"), QKeySequence::Refresh, this, [this]() {
        registry_->reload();
    });
    fileMenu->addAction(tr("打开工具目录"), this, [this]() {
        QString toolsDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                           + "/tooldeck/tools";
        QDir().mkpath(toolsDir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(toolsDir));
    });
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出(&Q)"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    viewMenu->addAction(tr("仪表盘"), this, [this]() { mainStack_->setCurrentWidget(dashboard_); });
    viewMenu->addAction(tr("输出面板"), this, [this]() { mainStack_->setCurrentWidget(outputPanel_); });

    // ---- Mode menu ----
    QMenu *modeMenu = menuBar()->addMenu(tr("模式(&M)"));
    QActionGroup *modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);

    QAction *tabAction = modeMenu->addAction("📑 普通模式");
    tabAction->setCheckable(true);
    tabAction->setChecked(true);
    tabAction->setActionGroup(modeGroup);

    QAction *compareAction = modeMenu->addAction("🔄 对比模式");
    compareAction->setCheckable(true);
    compareAction->setActionGroup(modeGroup);

    connect(tabAction, &QAction::triggered, this, [this]() { onModeChanged(0); });
    connect(compareAction, &QAction::triggered, this, [this]() { onModeChanged(1); });

    QMenu *toolsMenu = menuBar()->addMenu(tr("工具(&T)"));
    toolsMenu->addAction(tr("停止全部工具"), this, [this]() { manager_->stopAll(); });
    toolsMenu->addAction(tr("清理输出面板"), this, [this]() { outputPanel_->clearAll(); });

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

// ============================================================
//   Mode
// ============================================================

void MainWindow::onModeChanged(int index)
{
    compareMode_ = (index == 1);
    if (compareMode_) {
        outputPanel_->enterCompareMode();
        mainStack_->setCurrentWidget(outputPanel_);
        statusLabel_->setText("对比模式 — 点击工具开始对比");
    } else {
        outputPanel_->exitCompareMode();
        statusLabel_->setText("普通模式");
    }
}

// ============================================================
//   Tool launching
// ============================================================

void MainWindow::onToolRunRequested(const QString &toolName)
{
    const auto *manifest = registry_->findByName(toolName);
    if (!manifest) {
        statusLabel_->setText(QString("错误: 未找到工具 '%1'").arg(toolName));
        return;
    }

    bool isCompare = compareMode_;

    if (isCompare && !manifest->inputs.isEmpty()) {
        runCompare(*manifest);
    } else {
        runNormal(*manifest);
    }
}

void MainWindow::runNormal(const ToolManifest &manifest)
{
    QStringList extraArgs;

    bool hasRequired = false;
    for (const auto &def : manifest.inputs) {
        if (def.required) { hasRequired = true; break; }
    }

    if (hasRequired) {
        InputDialog dlg(manifest, this);
        if (dlg.exec() != QDialog::Accepted) {
            statusLabel_->setText("已取消运行");
            return;
        }
        extraArgs = manifest.buildArgs(dlg.values());
    } else if (!manifest.inputs.isEmpty()) {
        QHash<QString, QString> defaults;
        for (const auto &def : manifest.inputs) {
            if (!def.defaultValue.isEmpty()) defaults[def.name] = def.defaultValue;
        }
        extraArgs = manifest.buildArgs(defaults);
    }

    auto *inst = manager_->startTool(manifest, extraArgs);
    if (!inst) {
        statusLabel_->setText(QString("启动工具失败: %1").arg(manifest.name));
    }
}

void MainWindow::runCompare(const ToolManifest &manifest)
{
    CompareInputDialog dlg(manifest, this);
    if (dlg.exec() != QDialog::Accepted) {
        statusLabel_->setText("已取消对比");
        return;
    }

    outputPanel_->enterCompareMode();
    mainStack_->setCurrentWidget(outputPanel_);

    // Build left/right args
    QStringList leftArgs = manifest.buildArgs(dlg.leftValues());
    QStringList rightArgs = manifest.buildArgs(dlg.rightValues());

    // Create compare tab — auto-deduplicate label
    QString tabLabel = outputPanel_->uniqueCompareTabLabel(manifest.displayName);

    auto &view = outputPanel_->addCompareTab(tabLabel);

    // Build labels for the panel headers
    QString leftLabel = QString("A: %1 | %2").arg(manifest.displayName).arg(leftArgs.join(' '));
    QString rightLabel = QString("B: %1 | %2").arg(manifest.displayName).arg(rightArgs.join(' '));
    if (leftArgs.isEmpty()) leftLabel = QString("A: %1 (默认)").arg(manifest.displayName);
    if (rightArgs.isEmpty()) rightLabel = QString("B: %1 (默认)").arg(manifest.displayName);

    view.leftLabel->setText(leftLabel);
    view.rightLabel->setText(rightLabel);

    // Suppress default tab wiring for compare instances
    skipDefaultWiring_ = true;

    auto *instA = manager_->startTool(manifest, leftArgs);
    if (instA) wireInstanceToView(instA, view.leftOutput);

    auto *instB = manager_->startTool(manifest, rightArgs);
    if (instB) wireInstanceToView(instB, view.rightOutput);

    skipDefaultWiring_ = false;

    statusLabel_->setText(QString("对比运行中: %1").arg(manifest.displayName));
}

// ============================================================
//   Instance wiring
// ============================================================

void MainWindow::wireInstanceToTab(ToolInstance *inst, const QString &toolName)
{
    connect(inst, &ToolInstance::outputReady, outputPanel_,
            [this, toolName](const QString &text) {
        outputPanel_->appendOutput(toolName, text);
    });
    outputPanel_->addToolTab(toolName);
}

void MainWindow::wireInstanceToView(ToolInstance *inst, QPlainTextEdit *target)
{
    connect(inst, &ToolInstance::outputReady, this, [target](const QString &text) {
        OutputPanel::appendToView(target, text);
    });
}

// ============================================================
//   Other slots
// ============================================================

void MainWindow::onToolActivated(const QString &toolName)
{
    const auto *manifest = registry_->findByName(toolName);
    if (manifest) {
        statusLabel_->setText(
            QString("工具: %1 | %2 | v%3")
                .arg(manifest->displayName, manifest->description, manifest->version));
        auto *inst = manager_->instance(toolName);
        if (inst && inst->state() == ToolState::Running)
            mainStack_->setCurrentWidget(outputPanel_);
        else
            mainStack_->setCurrentWidget(dashboard_);
    }
}

void MainWindow::onToolStopRequested(const QString &toolName)
{
    manager_->stopTool(toolName);
}

void MainWindow::onToolsChanged()
{
    refreshToolList();
    statusLabel_->setText(QString("已加载 %1 个工具").arg(registry_->count()));
}

void MainWindow::refreshToolList()
{
    sidebar_->clear();
    auto grouped = registry_->toolsByCategory();
    for (auto it = grouped.begin(); it != grouped.end(); ++it)
        for (const auto &tool : it.value())
            sidebar_->addTool(tool);
}
