#include "output_panel.h"

#include <QVBoxLayout>
#include <QLabel>
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

    auto *header = new QLabel("<h2>📋 输出</h2>", this);
    layout->addWidget(header);

    tabWidget_ = new QTabWidget(this);
    tabWidget_->setTabsClosable(true);
    tabWidget_->setMovable(true);

    connect(tabWidget_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget *w = tabWidget_->widget(index);
        if (w) {
            // Find the tool name for this widget
            QString toolName;
            for (auto it = outputs_.begin(); it != outputs_.end(); ++it) {
                if (it.value() == w) {
                    toolName = it.key();
                    break;
                }
            }
            tabWidget_->removeTab(index);
            if (!toolName.isEmpty()) {
                outputs_.remove(toolName);
            }
        }
    });

    layout->addWidget(tabWidget_);

    // Empty state placeholder
    auto *emptyLabel = new QLabel(
        "<div style='text-align: center; margin-top: 60px; color: #888;'>"
        "<p>暂无工具输出。<br/>运行一个工具即可在此查看输出。</p>"
        "</div>"
    );
    emptyLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(emptyLabel);

    connect(tabWidget_, &QTabWidget::currentChanged, this, [emptyLabel](int index) {
        emptyLabel->setVisible(index < 0);
    });
}

void OutputPanel::addToolTab(const QString &toolName)
{
    auto *output = findOrCreateOutput(toolName);

    // Switch to the tab if it already exists
    for (int i = 0; i < tabWidget_->count(); ++i) {
        if (tabWidget_->widget(i) == output) {
            tabWidget_->setCurrentIndex(i);
            return;
        }
    }
}

QPlainTextEdit *OutputPanel::findOrCreateOutput(const QString &toolName)
{
    auto it = outputs_.find(toolName);
    if (it != outputs_.end()) {
        return it.value();
    }

    auto *textEdit = new QPlainTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("monospace", 10));
    textEdit->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #1e1e1e;"
        "  color: #d4d4d4;"
        "  border: none;"
        "}"
    );
    textEdit->setPlaceholderText(
        QString("等待 '%1' 输出...").arg(toolName)
    );

    outputs_[toolName] = textEdit;
    tabWidget_->addTab(textEdit, toolName);
    tabWidget_->setCurrentWidget(textEdit);

    return textEdit;
}

void OutputPanel::appendOutput(const QString &toolName, const QString &text)
{
    auto *output = findOrCreateOutput(toolName);

    // Prevent excessive memory usage
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
        if (idx >= 0) {
            tabWidget_->removeTab(idx);
        }
        outputs_.erase(it);
    }
}

void OutputPanel::clearOutput(const QString &toolName)
{
    auto *output = outputs_.value(toolName, nullptr);
    if (output) {
        output->clear();
    }
}
