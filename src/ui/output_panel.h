#ifndef OUTPUT_PANEL_H
#define OUTPUT_PANEL_H

#include <QWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QHash>

class QStackedWidget;

/// Output panel with two modes controlled externally (via MainWindow mode selector):
///   1. Tab mode — one tab per tool (normal usage)
///   2. Compare mode — left/right split panels (对比模式)
class OutputPanel : public QWidget
{
    Q_OBJECT

public:
    explicit OutputPanel(QWidget *parent = nullptr);

    // -- Tab mode (normal) --

    void addToolTab(const QString &toolName);
    void appendOutput(const QString &toolName, const QString &text);
    void removeToolTab(const QString &toolName);
    void clearOutput(const QString &toolName);

    // -- Compare mode (split) --

    /// Enter compare mode: show splitter, hide tabs
    void enterCompareMode();

    /// Exit compare mode: hide splitter, show tabs
    void exitCompareMode();

    /// Route output to the left panel
    void appendOutputLeft(const QString &text);

    /// Route output to the right panel
    void appendOutputRight(const QString &text);

    /// Set labels above left/right panels
    void setLeftLabel(const QString &label);
    void setRightLabel(const QString &label);

    /// Clear both compare panels
    void clearCompare();

    /// Clear all content (tabs + compare panels)
    void clearAll();

private:
    void setupUi();
    QPlainTextEdit *findOrCreateOutput(const QString &toolName);
    QPlainTextEdit *createOutputWidget(const QString &placeholder);

    // Tab mode
    QTabWidget *tabWidget_;
    QHash<QString, QPlainTextEdit *> outputs_;

    // Compare mode
    QWidget *compareContainer_;
    QSplitter *splitter_;
    QPlainTextEdit *leftOutput_;
    QPlainTextEdit *rightOutput_;
    QLabel *leftLabel_;
    QLabel *rightLabel_;

    // Shared
    QLabel *emptyLabel_;
};

#endif // OUTPUT_PANEL_H
