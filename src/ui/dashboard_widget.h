#ifndef DASHBOARD_WIDGET_H
#define DASHBOARD_WIDGET_H

#include <QWidget>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>

#include "core/tool_instance.h"

class ToolManager;
class ToolCard;

/// Dashboard showing tool status cards in a grid layout.
/// Each running or recently-run tool gets a card.
class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(ToolManager *manager, QWidget *parent = nullptr);

    /// Add a tool card for a running instance
    void addToolCard(const QString &name, ToolInstance *instance);

    /// Update the state display for a tool
    void updateToolState(const QString &name, ToolState state);

    /// Remove a tool card
    void removeToolCard(const QString &name);

signals:
    void runRequested(const QString &toolName);
    void stopRequested(const QString &toolName);

private:
    void setupUi();
    void relayoutCards();

    ToolManager *manager_;
    QWidget *cardContainer_;
    QGridLayout *cardLayout_;
    QLabel *emptyLabel_;
    QHash<QString, ToolCard *> cards_;
};

#endif // DASHBOARD_WIDGET_H
