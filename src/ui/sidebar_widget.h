#ifndef SIDEBAR_WIDGET_H
#define SIDEBAR_WIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include "core/tool_manifest.h"

/// Left sidebar panel: search bar + categorized tool list.
/// Emits signals for tool selection and run/stop actions.
class SidebarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);

    /// Add a tool to the sidebar (auto-groups by category)
    void addTool(const ToolManifest &manifest);

    /// Clear all tools
    void clear();

    /// Set the search filter text
    void setFilter(const QString &text);

signals:
    /// Emitted when a tool is clicked/selected in the list
    void toolSelected(const QString &toolName);

    /// Emitted when user requests to run a tool
    void runRequested(const QString &toolName);

    /// Emitted when user requests to stop a tool
    void stopRequested(const QString &toolName);

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onSearchTextChanged(const QString &text);
    void onRunClicked();

private:
    void setupUi();
    QTreeWidgetItem *findOrCreateCategory(const QString &category);
    QString toolNameFromItem(QTreeWidgetItem *item) const;

    QLineEdit *searchBox_;
    QTreeWidget *treeWidget_;
    QPushButton *runButton_;
    QPushButton *stopButton_;
    QString currentToolName_;
    QHash<QString, QTreeWidgetItem *> categoryItems_;
};

#endif // SIDEBAR_WIDGET_H
