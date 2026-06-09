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

/// A single compare view: left/right split panels with labels.
/// Exposed so callers can wire tool output directly to the correct panel.
struct CompareView {
    QWidget *container = nullptr;
    QPlainTextEdit *leftOutput = nullptr;
    QPlainTextEdit *rightOutput = nullptr;
    QLabel *leftLabel = nullptr;
    QLabel *rightLabel = nullptr;
};

/// Output panel with two modes controlled externally (via MainWindow mode selector):
///   1. Tab mode — one tab per tool (normal usage)
///   2. Compare mode — tabbed compare views, each with left/right split panels (对比模式)
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

    // -- Compare mode (tabbed split views) --

    /// Enter compare mode: show compare tabs, hide normal tabs
    void enterCompareMode();

    /// Exit compare mode: hide compare tabs, show normal tabs
    void exitCompareMode();

    /// Create a new compare tab with the given label.
    /// Returns a reference to the CompareView so callers can wire output directly.
    CompareView &addCompareTab(const QString &label);

    /// Append text to a specific output widget (handles truncation).
    static void appendToView(QPlainTextEdit *target, const QString &text);

    /// Generate a unique tab label by appending a suffix if needed
    QString uniqueCompareTabLabel(const QString &base) const;

    /// Clear the current active compare tab
    void clearCompare();

    /// Clear all content (tabs + compare views)
    void clearAll();

private:
    void setupUi();
    QPlainTextEdit *findOrCreateOutput(const QString &toolName);
    QPlainTextEdit *createOutputWidget(const QString &placeholder);

    void updateContentVisibility();

    // Tab mode
    QTabWidget *tabWidget_;
    QHash<QString, QPlainTextEdit *> outputs_;

    // Compare mode — tabbed, each tab = one CompareView
    QTabWidget *compareTabWidget_;
    QVector<CompareView> compareViews_;

    // Shared
    QStackedWidget *stack_;
    QLabel *emptyLabel_;
};

#endif // OUTPUT_PANEL_H
