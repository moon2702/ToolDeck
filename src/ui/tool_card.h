#ifndef TOOL_CARD_H
#define TOOL_CARD_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include "core/tool_instance.h"

/// A card widget displaying a single tool's status.
/// Shows icon, name, status indicator, elapsed time, and controls.
class ToolCard : public QFrame
{
    Q_OBJECT

public:
    explicit ToolCard(const QString &toolName, ToolInstance *instance, QWidget *parent = nullptr);

    /// Update the displayed state based on the instance
    void refresh();

    QString toolName() const { return toolName_; }

signals:
    void runClicked(const QString &toolName);
    void stopClicked(const QString &toolName);

private:
    void setupUi();
    QString statusColor() const;

    QString toolName_;
    ToolInstance *instance_;

    QLabel *iconLabel_;
    QLabel *nameLabel_;
    QLabel *statusDot_;
    QLabel *statusLabel_;
    QLabel *timeLabel_;
    QPushButton *runBtn_;
    QPushButton *stopBtn_;
};

#endif // TOOL_CARD_H
