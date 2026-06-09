#include "output_panel.h"
#include "core/diff_engine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QSplitter>
#include <QFont>
#include <QActionGroup>

OutputPanel::OutputPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void OutputPanel::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ---- Stack: content page vs empty hint ----
    stack_ = new QStackedWidget(this);

    // Page 0 — content area with both tab widgets
    auto *contentWidget = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Normal mode tab widget
    tabWidget_ = new QTabWidget(contentWidget);
    tabWidget_->setTabsClosable(true);
    tabWidget_->setMovable(true);
    connect(tabWidget_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget *w = tabWidget_->widget(index);
        if (w) {
            QString tn;
            for (auto it = outputs_.begin(); it != outputs_.end(); ++it) {
                if (it.value() == w) { tn = it.key(); break; }
            }
            tabWidget_->removeTab(index);
            if (!tn.isEmpty()) outputs_.remove(tn);
        }
        updateContentVisibility();
    });
    contentLayout->addWidget(tabWidget_);

    // Compare mode tab widget (hidden by default)
    compareTabWidget_ = new QTabWidget(contentWidget);
    compareTabWidget_->setTabsClosable(true);
    compareTabWidget_->setMovable(true);
    compareTabWidget_->setVisible(false);
    connect(compareTabWidget_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        compareTabWidget_->removeTab(index);
        compareViews_.removeAt(index);
        updateContentVisibility();
    });
    contentLayout->addWidget(compareTabWidget_);

    stack_->addWidget(contentWidget);  // index 0

    // Page 1 — centered empty hint
    emptyLabel_ = new QLabel(
        "<div style='text-align: center; color: #888;'>"
        "<p>暂无工具输出。<br/>运行一个工具即可在此查看输出。</p>"
        "</div>"
    );
    emptyLabel_->setAlignment(Qt::AlignCenter);
    stack_->addWidget(emptyLabel_);     // index 1

    stack_->setCurrentIndex(1);
    layout->addWidget(stack_);
}

QPlainTextEdit *OutputPanel::createOutputWidget(const QString &placeholder)
{
    auto *edit = new QPlainTextEdit(this);
    edit->setReadOnly(true);
    edit->setFont(QFont("monospace", 10));
    edit->setStyleSheet(
        "QPlainTextEdit { background-color: #1e1e1e; color: #d4d4d4; border: 1px solid #333; }");
    edit->setPlaceholderText(placeholder);
    return edit;
}

QPlainTextEdit *OutputPanel::findOrCreateOutput(const QString &toolName)
{
    auto it = outputs_.find(toolName);
    if (it != outputs_.end()) return it.value();
    auto *edit = createOutputWidget(QString("等待 '%1' 输出...").arg(toolName));
    outputs_[toolName] = edit;
    tabWidget_->addTab(edit, toolName);
    tabWidget_->setCurrentWidget(edit);
    updateContentVisibility();
    return edit;
}

// ============================================================
//   Content visibility
// ============================================================

void OutputPanel::updateContentVisibility()
{
    bool hasContent = tabWidget_->count() > 0 || compareTabWidget_->count() > 0;
    stack_->setCurrentIndex(hasContent ? 0 : 1);
}

// ============================================================
//   Tab mode API
// ============================================================

void OutputPanel::addToolTab(const QString &toolName)
{
    auto *output = findOrCreateOutput(toolName);
    for (int i = 0; i < tabWidget_->count(); ++i) {
        if (tabWidget_->widget(i) == output) { tabWidget_->setCurrentIndex(i); return; }
    }
}

void OutputPanel::appendOutput(const QString &toolName, const QString &text)
{
    auto *output = findOrCreateOutput(toolName);
    appendToView(output, text);
}

void OutputPanel::removeToolTab(const QString &toolName)
{
    auto it = outputs_.find(toolName);
    if (it != outputs_.end()) {
        int idx = tabWidget_->indexOf(it.value());
        if (idx >= 0) tabWidget_->removeTab(idx);
        outputs_.erase(it);
        updateContentVisibility();
    }
}

void OutputPanel::clearOutput(const QString &toolName)
{
    auto *output = outputs_.value(toolName, nullptr);
    if (output) output->clear();
}

// ============================================================
//   Compare mode API — tabbed views
// ============================================================

void OutputPanel::enterCompareMode()
{
    tabWidget_->setVisible(false);
    compareTabWidget_->setVisible(true);
    updateContentVisibility();
}

void OutputPanel::exitCompareMode()
{
    compareTabWidget_->setVisible(false);
    tabWidget_->setVisible(true);
    updateContentVisibility();
}

CompareView &OutputPanel::addCompareTab(const QString &label)
{
    CompareView view;
    view.container = new QWidget(this);
    auto *vl = new QVBoxLayout(view.container);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    // ---- Toolbar ----
    view.toolbar = new QToolBar(view.container);
    view.toolbar->setMovable(false);
    view.toolbar->setStyleSheet(
        "QToolBar { background: #2d2d2d; border: none; border-bottom: 1px solid #3a3a3a;"
        "  spacing: 2px; padding: 2px 4px; }"
        "QToolBar QToolButton { color: #999; padding: 2px 12px; border-radius: 3px;"
        "  font-size: 11px; border: 1px solid transparent; }"
        "QToolBar QToolButton:checked { background: #444; color: #eee;"
        "  border: 1px solid #555; }"
        "QToolBar QToolButton:hover { background: #383838; }"
    );

    auto *actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);

    view.rawAction = view.toolbar->addAction("▦ 原始");
    view.rawAction->setCheckable(true);
    view.rawAction->setChecked(true);
    view.rawAction->setActionGroup(actionGroup);

    view.unifiedAction = view.toolbar->addAction("≔ 统一差异");
    view.unifiedAction->setCheckable(true);
    view.unifiedAction->setActionGroup(actionGroup);

    view.sideAction = view.toolbar->addAction("◫ 并排差异");
    view.sideAction->setCheckable(true);
    view.sideAction->setActionGroup(actionGroup);

    // Diff summary label (right-aligned)
    auto *spacer = new QWidget(view.container);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    view.toolbar->addWidget(spacer);

    view.diffSummary = new QLabel(view.container);
    view.diffSummary->setStyleSheet("color: #888; font-size: 11px; padding: 0 6px;");
    view.toolbar->addWidget(view.diffSummary);

    vl->addWidget(view.toolbar);

    // ---- Stacked: raw splitter / unified diff / side-by-side diff ----
    view.viewStack = new QStackedWidget(view.container);

    // Page 0 — raw splitter
    view.splitterWrapper = new QWidget(view.container);
    auto *swl = new QHBoxLayout(view.splitterWrapper);
    swl->setContentsMargins(0, 0, 0, 0);
    swl->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, view.splitterWrapper);
    splitter->setHandleWidth(3);

    // Left panel
    auto *lc = new QWidget(this);
    auto *ll = new QVBoxLayout(lc);
    ll->setContentsMargins(0, 0, 0, 0);
    ll->setSpacing(0);
    view.leftLabel = new QLabel(label + " — A", this);
    view.leftLabel->setStyleSheet(
        "background: #333; color: #aaa; font-size: 11px;"
        "padding: 1px 6px; border-bottom: 1px solid #444;");
    view.leftLabel->setMaximumHeight(20);
    ll->addWidget(view.leftLabel);
    view.leftOutput = createOutputWidget("参数组 A 的输出将显示在此");
    ll->addWidget(view.leftOutput, 1);
    splitter->addWidget(lc);

    // Right panel
    auto *rc = new QWidget(this);
    auto *rl = new QVBoxLayout(rc);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(0);
    view.rightLabel = new QLabel(label + " — B", this);
    view.rightLabel->setStyleSheet(
        "background: #333; color: #aaa; font-size: 11px;"
        "padding: 1px 6px; border-bottom: 1px solid #444;");
    view.rightLabel->setMaximumHeight(20);
    rl->addWidget(view.rightLabel);
    view.rightOutput = createOutputWidget("参数组 B 的输出将显示在此");
    rl->addWidget(view.rightOutput, 1);
    splitter->addWidget(rc);

    splitter->setSizes({380, 380});
    swl->addWidget(splitter);
    view.viewStack->addWidget(view.splitterWrapper);  // page 0

    // Page 1 — unified diff view
    auto *unifiedPage = new QWidget(view.container);
    auto *ufl = new QVBoxLayout(unifiedPage);
    ufl->setContentsMargins(0, 0, 0, 0);
    ufl->setSpacing(0);

    view.diffView = new QTextEdit(unifiedPage);
    view.diffView->setReadOnly(true);
    view.diffView->setFont(QFont("monospace", 11));
    view.diffView->setStyleSheet(
        "QTextEdit { background-color: #1e1e1e; border: none; }");
    view.diffView->setPlaceholderText("点击「统一差异」以查看 A/B 输出差异");
    ufl->addWidget(view.diffView);
    view.viewStack->addWidget(unifiedPage);  // page 1

    // Page 2 — side-by-side diff view
    auto *sidePage = new QWidget(view.container);
    auto *ssl = new QHBoxLayout(sidePage);
    ssl->setContentsMargins(0, 0, 0, 0);
    ssl->setSpacing(1);

    view.sideLeftView = new QTextEdit(sidePage);
    view.sideLeftView->setReadOnly(true);
    view.sideLeftView->setFont(QFont("monospace", 11));
    view.sideLeftView->setStyleSheet(
        "QTextEdit { background-color: #1e1e1e; border: none; }");
    view.sideLeftView->setPlaceholderText("左侧输出");
    ssl->addWidget(view.sideLeftView);

    view.sideRightView = new QTextEdit(sidePage);
    view.sideRightView->setReadOnly(true);
    view.sideRightView->setFont(QFont("monospace", 11));
    view.sideRightView->setStyleSheet(
        "QTextEdit { background-color: #1e1e1e; border: none; }");
    view.sideRightView->setPlaceholderText("右侧输出");
    ssl->addWidget(view.sideRightView);
    view.viewStack->addWidget(sidePage);  // page 2

    view.viewStack->setCurrentIndex(0);
    vl->addWidget(view.viewStack, 1);

    // ---- Toolbar actions ----
    int viewIdx = compareViews_.size();

    connect(view.rawAction, &QAction::triggered, this, [this, viewIdx]() {
        if (viewIdx < compareViews_.size()) {
            auto &v = compareViews_[viewIdx];
            v.viewStack->setCurrentIndex(0);
            v.diffSummary->clear();
        }
    });

    connect(view.unifiedAction, &QAction::triggered, this, [this, viewIdx]() {
        if (viewIdx < compareViews_.size())
            refreshUnifiedDiff(compareViews_[viewIdx]);
    });

    connect(view.sideAction, &QAction::triggered, this, [this, viewIdx]() {
        if (viewIdx < compareViews_.size())
            refreshSideDiff(compareViews_[viewIdx]);
    });

    compareViews_.append(view);
    int idx = compareTabWidget_->addTab(view.container, label);
    compareTabWidget_->setCurrentIndex(idx);

    updateContentVisibility();
    return compareViews_.last();
}

void OutputPanel::refreshUnifiedDiff(CompareView &view)
{
    QString leftText  = view.leftOutput->toPlainText();
    QString rightText = view.rightOutput->toPlainText();

    // Compute diff and cache for side-by-side switching
    view.cachedHunks = DiffEngine::compute(leftText, rightText);
    DiffEngine::enrichInline(view.cachedHunks);
    const auto &hunks = view.cachedHunks;

    if (hunks.isEmpty() && !leftText.isEmpty() && !rightText.isEmpty()) {
        view.diffView->setHtml(
            "<html><body style='background:#1e1e1e; color:#888; font-family:monospace; "
            "font-size:12px; margin:0; padding:12px;'>"
            "<p>(A 与 B 输出完全一致，无差异)</p></body></html>");
        view.diffSummary->setText("无差异");
    } else if (hunks.isEmpty()) {
        view.diffView->setHtml(
            "<html><body style='background:#1e1e1e; color:#888; font-family:monospace; "
            "font-size:12px; margin:0; padding:12px;'>"
            "<p>左侧或右侧无输出，无法对比。</p></body></html>");
        view.diffSummary->setText("—");
    } else {
        view.diffView->setHtml(DiffEngine::toHtmlUnified(hunks));
        view.diffSummary->setText(DiffEngine::summary(hunks));
    }

    view.viewStack->setCurrentIndex(1);
}

void OutputPanel::refreshSideDiff(CompareView &view)
{
    QString leftText  = view.leftOutput->toPlainText();
    QString rightText = view.rightOutput->toPlainText();

    view.cachedHunks = DiffEngine::compute(leftText, rightText);
    DiffEngine::enrichInline(view.cachedHunks);
    const auto &hunks = view.cachedHunks;

    if (hunks.isEmpty() && !leftText.isEmpty() && !rightText.isEmpty()) {
        QString sameMsg =
            "<html><body style='background:#1e1e1e; color:#888; font-family:monospace; "
            "font-size:12px; margin:0; padding:12px;'>"
            "<p>(A 与 B 输出完全一致)</p></body></html>";
        view.sideLeftView->setHtml(sameMsg);
        view.sideRightView->setHtml(sameMsg);
        view.diffSummary->setText("无差异");
    } else if (hunks.isEmpty()) {
        QString emptyMsg =
            "<html><body style='background:#1e1e1e; color:#888; font-family:monospace; "
            "font-size:12px; margin:0; padding:12px;'>"
            "<p>无法对比</p></body></html>";
        view.sideLeftView->setHtml(emptyMsg);
        view.sideRightView->setHtml(emptyMsg);
        view.diffSummary->setText("—");
    } else {
        auto [leftHtml, rightHtml] = DiffEngine::toHtmlSideBySide(hunks);
        view.sideLeftView->setHtml(leftHtml);
        view.sideRightView->setHtml(rightHtml);
        view.diffSummary->setText(DiffEngine::summary(hunks));
    }

    view.viewStack->setCurrentIndex(2);
}

QString OutputPanel::uniqueCompareTabLabel(const QString &base) const
{
    QString candidate = base;
    int suffix = 1;
    while (true) {
        bool dup = false;
        for (int i = 0; i < compareTabWidget_->count(); ++i) {
            if (compareTabWidget_->tabText(i) == candidate) {
                dup = true;
                break;
            }
        }
        if (!dup) break;
        candidate = QString("%1 (%2)").arg(base).arg(++suffix);
    }
    return candidate;
}

void OutputPanel::appendToView(QPlainTextEdit *target, const QString &text)
{
    if (target->document()->blockCount() > 10000) {
        target->clear();
        target->appendPlainText("--- 输出已截断 (超过10000行限制) ---");
    }
    target->moveCursor(QTextCursor::End);
    target->insertPlainText(text);
    target->moveCursor(QTextCursor::End);
}

void OutputPanel::clearCompare()
{
    int idx = compareTabWidget_->currentIndex();
    if (idx >= 0 && idx < compareViews_.size()) {
        auto &v = compareViews_[idx];
        v.leftOutput->clear();
        v.rightOutput->clear();
        v.leftLabel->setText("A");
        v.rightLabel->setText("B");
        v.diffView->clear();
        v.sideLeftView->clear();
        v.sideRightView->clear();
        v.diffSummary->clear();
        v.cachedHunks.clear();
    }
}

void OutputPanel::clearAll()
{
    for (auto *output : outputs_) output->clear();
    for (auto &v : compareViews_) {
        v.leftOutput->clear();
        v.rightOutput->clear();
        v.diffView->clear();
        v.sideLeftView->clear();
        v.sideRightView->clear();
        v.diffSummary->clear();
        v.cachedHunks.clear();
    }
}
