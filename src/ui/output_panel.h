#ifndef OUTPUT_PANEL_H
#define OUTPUT_PANEL_H

#include <QWidget>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QHash>

/// Tabbed output panel showing real-time stdout per tool.
/// Each running tool gets its own tab with a read-only text view.
class OutputPanel : public QWidget
{
    Q_OBJECT

public:
    explicit OutputPanel(QWidget *parent = nullptr);

    /// Add a tab for a tool (or switch to existing)
    void addToolTab(const QString &toolName);

    /// Append text to a tool's output
    void appendOutput(const QString &toolName, const QString &text);

    /// Remove a tool's tab
    void removeToolTab(const QString &toolName);

    /// Clear output for a specific tool
    void clearOutput(const QString &toolName);

private:
    void setupUi();
    QPlainTextEdit *findOrCreateOutput(const QString &toolName);

    QTabWidget *tabWidget_;
    QHash<QString, QPlainTextEdit *> outputs_;
};

#endif // OUTPUT_PANEL_H
