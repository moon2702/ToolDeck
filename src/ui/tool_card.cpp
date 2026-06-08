#include "tool_card.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

ToolCard::ToolCard(const QString &toolName, ToolInstance *instance, QWidget *parent)
    : QFrame(parent)
    , toolName_(toolName)
    , instance_(instance)
{
    setupUi();

    // Auto-refresh every second for elapsed time
    auto *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &ToolCard::refresh);
    refreshTimer->start(1000);

    refresh();
}

void ToolCard::setupUi()
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    setMinimumSize(180, 120);
    setMaximumSize(280, 160);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(6);

    // Top row: icon + name
    auto *topRow = new QHBoxLayout();

    iconLabel_ = new QLabel(this);
    QIcon icon = QIcon::fromTheme(instance_->manifest().icon);
    if (icon.isNull()) {
        icon = QIcon::fromTheme("application-x-executable");
    }
    iconLabel_->setPixmap(icon.pixmap(24, 24));
    topRow->addWidget(iconLabel_);

    nameLabel_ = new QLabel(instance_->manifest().displayName, this);
    QFont nameFont = nameLabel_->font();
    nameFont.setPointSize(nameFont.pointSize() + 1);
    nameFont.setBold(true);
    nameLabel_->setFont(nameFont);
    topRow->addWidget(nameLabel_);
    topRow->addStretch();

    layout->addLayout(topRow);

    // Status row: dot + status text
    auto *statusRow = new QHBoxLayout();

    statusDot_ = new QLabel("●", this);
    statusDot_->setStyleSheet("color: gray; font-size: 14px;");
    statusRow->addWidget(statusDot_);

    statusLabel_ = new QLabel("空闲", this);
    statusRow->addWidget(statusLabel_);

    timeLabel_ = new QLabel("", this);
    timeLabel_->setStyleSheet("color: #888;");
    statusRow->addWidget(timeLabel_);
    statusRow->addStretch();

    layout->addLayout(statusRow);

    // Bottom row: action buttons
    auto *btnRow = new QHBoxLayout();

    runBtn_ = new QPushButton("▶  运行", this);
    runBtn_->setFixedHeight(28);
    connect(runBtn_, &QPushButton::clicked, this, [this]() {
        emit runClicked(toolName_);
    });
    btnRow->addWidget(runBtn_);

    stopBtn_ = new QPushButton("■  停止", this);
    stopBtn_->setFixedHeight(28);
    connect(stopBtn_, &QPushButton::clicked, this, [this]() {
        emit stopClicked(toolName_);
    });
    btnRow->addWidget(stopBtn_);

    layout->addLayout(btnRow);
}

void ToolCard::refresh()
{
    QString color = statusColor();
    statusDot_->setStyleSheet(QString("color: %1; font-size: 14px;").arg(color));

    statusLabel_->setText(instance_->stateString());

    // Show elapsed time for running tools
    if (instance_->state() == ToolState::Running) {
        qint64 secs = instance_->elapsedMs() / 1000;
        if (secs < 60) {
            timeLabel_->setText(QString("%1s").arg(secs));
        } else if (secs < 3600) {
            timeLabel_->setText(QString("%1m %2s").arg(secs / 60).arg(secs % 60));
        } else {
            timeLabel_->setText(QString("%1h %2m").arg(secs / 3600).arg((secs % 3600) / 60));
        }
    } else if (instance_->state() == ToolState::Idle) {
        timeLabel_->setText("");
    }

    // Toggle button states
    bool isRunning = (instance_->state() == ToolState::Running ||
                      instance_->state() == ToolState::Starting);
    runBtn_->setEnabled(!isRunning && instance_->state() != ToolState::Starting);
    stopBtn_->setEnabled(isRunning);
}

QString ToolCard::statusColor() const
{
    switch (instance_->state()) {
    case ToolState::Running:  return "#4CAF50"; // Green
    case ToolState::Starting: return "#FFC107"; // Amber
    case ToolState::Finished: return "#2196F3"; // Blue
    case ToolState::Failed:   return "#F44336"; // Red
    case ToolState::Stopped:  return "#FF9800"; // Orange
    case ToolState::Idle:     return "#9E9E9E"; // Grey
    }
    return "#9E9E9E";
}
