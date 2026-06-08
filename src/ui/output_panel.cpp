#include "output_panel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
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
    layout->setSpacing(4);

    // ---- Tab mode (default) ----
    tabWidget_ = new QTabWidget(this);
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
    });
    layout->addWidget(tabWidget_);

    // ---- Compare mode (hidden by default) ----
    compareContainer_ = new QWidget(this);
    auto *cmpLayout = new QVBoxLayout(compareContainer_);
    cmpLayout->setContentsMargins(0, 0, 0, 0);
    cmpLayout->setSpacing(0);

    splitter_ = new QSplitter(Qt::Horizontal, compareContainer_);
    splitter_->setHandleWidth(3);

    // Left panel — label inside the text area as a tiny overlay bar
    auto *lc = new QWidget(this);
    auto *ll = new QVBoxLayout(lc);
    ll->setContentsMargins(0, 0, 0, 0);
    ll->setSpacing(0);
    leftLabel_ = new QLabel("A", this);
    leftLabel_->setStyleSheet(
        "background: #333; color: #aaa; font-size: 11px;"
        "padding: 1px 6px; border-bottom: 1px solid #444;");
    leftLabel_->setMaximumHeight(20);
    ll->addWidget(leftLabel_);
    leftOutput_ = createOutputWidget("参数组 A 的输出将显示在此");
    ll->addWidget(leftOutput_, 1);  // stretch = 1 → fills all remaining space
    splitter_->addWidget(lc);

    // Right panel
    auto *rc = new QWidget(this);
    auto *rl = new QVBoxLayout(rc);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(0);
    rightLabel_ = new QLabel("B", this);
    rightLabel_->setStyleSheet(
        "background: #333; color: #aaa; font-size: 11px;"
        "padding: 1px 6px; border-bottom: 1px solid #444;");
    rightLabel_->setMaximumHeight(20);
    rl->addWidget(rightLabel_);
    rightOutput_ = createOutputWidget("参数组 B 的输出将显示在此");
    rl->addWidget(rightOutput_, 1);  // stretch = 1
    splitter_->addWidget(rc);

    cmpLayout->addWidget(splitter_);
    compareContainer_->setVisible(false);
    layout->addWidget(compareContainer_, 1);  // stretch → fills available space

    // ---- Empty state ----
    emptyLabel_ = new QLabel(
        "<div style='text-align: center; margin-top: 60px; color: #888;'>"
        "<p>暂无工具输出。<br/>运行一个工具即可在此查看输出。</p>"
        "</div>"
    );
    emptyLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(emptyLabel_);

    connect(tabWidget_, &QTabWidget::currentChanged, this, [this](int) {
        emptyLabel_->setVisible(tabWidget_->count() == 0);
    });
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
    return edit;
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
    emptyLabel_->setVisible(false);
}

void OutputPanel::appendOutput(const QString &toolName, const QString &text)
{
    auto *output = findOrCreateOutput(toolName);
    if (output->document()->blockCount() > 10000) {
        output->clear();
        output->appendPlainText("--- 输出已截断 (超过10000行限制) ---");
    }
    output->moveCursor(QTextCursor::End);
    output->insertPlainText(text);
    output->moveCursor(QTextCursor::End);
}

void OutputPanel::removeToolTab(const QString &toolName)
{
    auto it = outputs_.find(toolName);
    if (it != outputs_.end()) {
        int idx = tabWidget_->indexOf(it.value());
        if (idx >= 0) tabWidget_->removeTab(idx);
        outputs_.erase(it);
    }
}

void OutputPanel::clearOutput(const QString &toolName)
{
    auto *output = outputs_.value(toolName, nullptr);
    if (output) output->clear();
}

// ============================================================
//   Compare mode API
// ============================================================

void OutputPanel::enterCompareMode()
{
    tabWidget_->setVisible(false);
    compareContainer_->setVisible(true);
    emptyLabel_->setVisible(false);
}

void OutputPanel::exitCompareMode()
{
    compareContainer_->setVisible(false);
    tabWidget_->setVisible(true);
    emptyLabel_->setVisible(tabWidget_->count() == 0);
}

void OutputPanel::appendOutputLeft(const QString &text)
{
    leftOutput_->moveCursor(QTextCursor::End);
    leftOutput_->insertPlainText(text);
    leftOutput_->moveCursor(QTextCursor::End);
}

void OutputPanel::appendOutputRight(const QString &text)
{
    rightOutput_->moveCursor(QTextCursor::End);
    rightOutput_->insertPlainText(text);
    rightOutput_->moveCursor(QTextCursor::End);
}

void OutputPanel::setLeftLabel(const QString &label)
{
    leftLabel_->setText(label);
}

void OutputPanel::setRightLabel(const QString &label)
{
    rightLabel_->setText(label);
}

void OutputPanel::clearCompare()
{
    leftOutput_->clear();
    rightOutput_->clear();
    leftLabel_->setText("A");
    rightLabel_->setText("B");
}

void OutputPanel::clearAll()
{
    // Tab mode
    for (auto *output : outputs_) output->clear();
    // Compare mode
    leftOutput_->clear();
    rightOutput_->clear();
    leftLabel_->setText("A");
    rightLabel_->setText("B");
}
