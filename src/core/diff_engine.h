#ifndef DIFF_ENGINE_H
#define DIFF_ENGINE_H

#include <QString>
#include <QPair>
#include <QVector>

/// A single character-level chunk for inline diff highlighting.
struct InlineChunk {
    enum Type { Same, Added, Removed };
    Type type = Same;
    QString text;  // HTML-escaped substring
};

/// A single line in a diff result.
struct DiffHunk {
    enum Type { Same, Added, Removed, Header };
    Type type = Same;
    QString text;
    int leftLine = -1;   // 1-based line number in left input, -1 if not present
    int rightLine = -1;  // 1-based line number in right input, -1 if not present

    /// Inline character-level diff chunks (populated by enrichInline).
    /// Non-empty only for Added/Removed lines that were paired with a counterpart.
    QVector<InlineChunk> inlineChunks;
};

/// Line-based diff engine using LCS (Longest Common Subsequence) algorithm.
/// Supports both line-level and character-level inline diffs.
class DiffEngine
{
public:
    /// Compute diff between two text blocks.
    /// Returns hunks with headers (@@ -a,b +c,d @@) and context lines.
    /// @param contextLines  number of unchanged lines to keep around changes
    static QVector<DiffHunk> compute(const QString &left, const QString &right,
                                      int contextLines = 3);

    /// Enrich diff hunks with character-level inline diffs.
    /// Pairs adjacent Removed/Added lines and highlights changed characters within them.
    static void enrichInline(QVector<DiffHunk> &hunks);

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

    /// Character-level LCS table for two strings.
    static QVector<QVector<int>> buildCharLcsTable(const QString &left,
                                                    const QString &right);

    /// Backtrack character-level LCS → inline chunks.
    static QVector<InlineChunk> backtrackChars(const QVector<QVector<int>> &dp,
                                                const QString &left,
                                                const QString &right);

    /// Render a line with inline chunks as HTML.
    static QString renderInlineLine(const DiffHunk &hunk, const QString &prefix,
                                     const QString &baseBg, const QString &baseFg);

    /// Character-level similarity ratio: LCS length / max length. Range [0, 1].
    static double similarity(const QString &a, const QString &b);

    /// Compute inline diff between two lines and fill inlineChunks on both.
    static void pairInline(DiffHunk &removed, DiffHunk &added);
};

#endif // DIFF_ENGINE_H
