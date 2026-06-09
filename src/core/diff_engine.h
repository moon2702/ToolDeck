#ifndef DIFF_ENGINE_H
#define DIFF_ENGINE_H

#include <QString>
#include <QPair>
#include <QVector>

/// A single line in a diff result.
struct DiffHunk {
    enum Type { Same, Added, Removed, Header };
    Type type = Same;
    QString text;
    int leftLine = -1;   // 1-based line number in left input, -1 if not present
    int rightLine = -1;  // 1-based line number in right input, -1 if not present
};

/// Line-based diff engine using LCS (Longest Common Subsequence) algorithm.
/// Produces structured diff results that can be rendered as HTML.
class DiffEngine
{
public:
    /// Compute diff between two text blocks.
    /// Returns hunks with headers (@@ -a,b +c,d @@) and context lines.
    /// @param contextLines  number of unchanged lines to keep around changes
    static QVector<DiffHunk> compute(const QString &left, const QString &right,
                                      int contextLines = 3);

    /// Convert diff hunks to unified diff HTML (single column, +/- prefixes).
    static QString toHtmlUnified(const QVector<DiffHunk> &hunks);

    /// Convert diff hunks to side-by-side HTML.
    /// Returns a pair of {leftHtml, rightHtml} for display in two aligned panes.
    static QPair<QString, QString> toHtmlSideBySide(const QVector<DiffHunk> &hunks);

    /// Return a summary string like "+12 −3".
    static QString summary(const QVector<DiffHunk> &hunks);

private:
    /// Compute LCS length table for two string lists.
    static QVector<QVector<int>> buildLcsTable(const QStringList &left,
                                                const QStringList &right);

    /// Backtrack the LCS table to produce a raw diff (Same/Added/Removed per line).
    static QVector<DiffHunk> backtrack(const QVector<QVector<int>> &dp,
                                        const QStringList &left,
                                        const QStringList &right);
};

#endif // DIFF_ENGINE_H
