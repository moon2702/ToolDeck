#include "output_panel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QFont>

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

    stack_->setCurrentIndex(1);  // start with empty hint
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

    auto *splitter = new QSplitter(Qt::Horizontal, view.container);
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
    vl->addWidget(splitter);

    compareViews_.append(view);
    int idx = compareTabWidget_->addTab(view.container, label);
    compareTabWidget_->setCurrentIndex(idx);

    updateContentVisibility();
    return compareViews_.last();
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
    }
}

void OutputPanel::clearAll()
{
    // Tab mode
    for (auto *output : outputs_) output->clear();
    // Compare mode
    for (auto &v : compareViews_) {
        v.leftOutput->clear();
        v.rightOutput->clear();
    }
}
