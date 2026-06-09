#include "diff_engine.h"
#include <QStringList>

// ============================================================
//   Public API
// ============================================================

QVector<DiffHunk> DiffEngine::compute(const QString &left, const QString &right,
                                       int contextLines)
{
    const QStringList leftLines  = left.split('\n');
    const QStringList rightLines = right.split('\n');

    auto dp = buildLcsTable(leftLines, rightLines);
    auto raw = backtrack(dp, leftLines, rightLines);

    if (raw.isEmpty())
        return {};

    // Phase 1: identify change blocks with context
    // A "change" line is Added or Removed (not Same, not Header).
    int n = raw.size();

    // Mark lines to keep
    QVector<bool> keep(n, false);
    for (int i = 0; i < n; ++i) {
        if (raw[i].type == DiffHunk::Added || raw[i].type == DiffHunk::Removed) {
            // The change itself
            keep[i] = true;
            // Context before
            for (int k = 1; k <= contextLines && i - k >= 0; ++k) {
                if (raw[i - k].type == DiffHunk::Same)
                    keep[i - k] = true;
            }
            // Context after
            for (int k = 1; k <= contextLines && i + k < n; ++k) {
                if (raw[i + k].type == DiffHunk::Same)
                    keep[i + k] = true;
            }
        }
    }

    // If no changes at all, return empty — nothing to diff
    bool hasChanges = false;
    for (bool k : keep) { if (k) { hasChanges = true; break; } }
    if (!hasChanges) return {};

    // Phase 2: emit hunks in blocks with headers
    QVector<DiffHunk> result;
    bool inBlock = false;
    int  blockLeftStart = 0, blockRightStart = 0;
    int  blockLeftCount = 0, blockRightCount = 0;
    int  emittedSameCount = 0;

    for (int i = 0; i < n; ++i) {
        if (!keep[i]) {
            // End current block at a non-kept line (gap)
            if (inBlock) {
                // Emit buffered Same context lines first, then header, then the block content
                // We already emitted Same lines — they're already in result.
                // Just mark end of block.
                inBlock = false;
            }
            continue;
        }

        const auto &h = raw[i];

        if (!inBlock) {
            // Starting a new block
            inBlock = true;

            // Walk back to count preceding context Same lines for header ranges
            int leftMin  = h.leftLine  > 0 ? h.leftLine  : 999999;
            int rightMin = h.rightLine > 0 ? h.rightLine : 999999;
            int leftMax  = h.leftLine  > 0 ? h.leftLine  : 0;
            int rightMax = h.rightLine > 0 ? h.rightLine : 0;

            // Count kept lines backwards for start range
            for (int k = i - 1; k >= 0 && keep[k] && raw[k].type == DiffHunk::Same; --k) {
                if (raw[k].leftLine  > 0 && raw[k].leftLine  < leftMin)  leftMin  = raw[k].leftLine;
                if (raw[k].rightLine > 0 && raw[k].rightLine < rightMin) rightMin = raw[k].rightLine;
            }

            // Count forward (same and change lines) for the count
            int leftCount  = 0;
            int rightCount = 0;
            for (int k = i; k < n; ++k) {
                if (!keep[k]) break;
                const auto &hk = raw[k];

                // Stop at next pure-context boundary (Same line followed by gap of Same lines → end of block)
                // Actually just count all kept lines until we hit a gap
                if (hk.type == DiffHunk::Added) {
                    if (hk.rightLine > 0) ++rightCount;
                } else if (hk.type == DiffHunk::Removed) {
                    if (hk.leftLine > 0) ++leftCount;
                } else {
                    if (hk.leftLine > 0) ++leftCount;
                    if (hk.rightLine > 0) ++rightCount;
                }

                // Check if next line breaks the block
                if (k + 1 < n && !keep[k + 1]) break;
                if (k + 1 < n && keep[k + 1] && raw[k].type == DiffHunk::Same
                    && raw[k + 1].type == DiffHunk::Same) {
                    // Two consecutive Same lines after changes → check if there's more changes ahead
                    bool moreChanges = false;
                    for (int j = k + 1; j < n; ++j) {
                        if (!keep[j]) break;
                        if (raw[j].type != DiffHunk::Same) { moreChanges = true; break; }
                    }
                    if (!moreChanges) break;
                }
            }

            blockLeftStart  = leftMin;
            blockRightStart = rightMin;
            blockLeftCount  = leftCount;
            blockRightCount = rightCount;

            // Emit header
            DiffHunk header;
            header.type = DiffHunk::Header;
            header.text = QString("@@ -%1,%2 +%3,%4 @@")
                              .arg(blockLeftStart).arg(blockLeftCount)
                              .arg(blockRightStart).arg(blockRightCount);
            result.append(header);
        }

        result.append(h);
    }

    // If all we got was headers but no changes, return empty
    bool anyChange = false;
    for (const auto &h : result) {
        if (h.type == DiffHunk::Added || h.type == DiffHunk::Removed) {
            anyChange = true; break;
        }
    }
    if (!anyChange) return {};

    return result;
}

QString DiffEngine::toHtmlUnified(const QVector<DiffHunk> &hunks)
{
    if (hunks.isEmpty()) return {};

    // Count added/removed lines for summary
    int totalAdded = 0, totalRemoved = 0;
    for (const auto &h : hunks) {
        if (h.type == DiffHunk::Added)   ++totalAdded;
        if (h.type == DiffHunk::Removed) ++totalRemoved;
    }

    QString html;
    html += "<html><body style='background:#1e1e1e; color:#d4d4d4; font-family:monospace; "
            "font-size:12px; margin:0; padding:6px; white-space:pre;'>";

    for (const auto &h : hunks) {
        QString line = h.text.toHtmlEscaped();

        switch (h.type) {
        case DiffHunk::Header:
            html += "<div style='background:#2a2a4a; color:#8ab4d8; padding:2px 8px; "
                    "margin:4px 0 0 0;'>"
                  + line + "</div>";
            break;
        case DiffHunk::Added:
            html += "<div style='background:#1b3a1b; color:#b5d6b5; padding:0 8px;'>+ "
                  + line + "</div>";
            break;
        case DiffHunk::Removed:
            html += "<div style='background:#3a1b1b; color:#d6b5b5; padding:0 8px;'>- "
                  + line + "</div>";
            break;
        case DiffHunk::Same:
            html += "<div style='color:#666; padding:0 8px;'>  "
                  + line + "</div>";
            break;
        }
    }

    if (totalAdded == 0 && totalRemoved == 0)
        html += "<div style='color:#888; padding:8px;'>(无差异)</div>";

    html += "</body></html>";
    return html;
}

QPair<QString, QString> DiffEngine::toHtmlSideBySide(const QVector<DiffHunk> &hunks)
{
    if (hunks.isEmpty()) return {};

    // Build left and right HTML in parallel, using a table for alignment.
    // We generate a single HTML string with a two-column table,
    // then extract left and right portions. Actually, simpler to emit
    // separate left/right divs, each row synced with same-height rows.

    QString leftHtml;
    QString rightHtml;

    auto start = []() -> QString {
        return "<html><body style='background:#1e1e1e; color:#d4d4d4; font-family:monospace; "
               "font-size:12px; margin:0; padding:0; white-space:pre;'>";
    };
    auto end = []() -> QString {
        return "</body></html>";
    };

    leftHtml  = start();
    rightHtml = start();

    // Track how many lines we've emitted in each column for alignment
    // We'll walk through hunks and build paired <div> rows

    for (const auto &h : hunks) {
        QString line = h.text.toHtmlEscaped();
        QString emptyRow = "<div style='height:18px;'>&nbsp;</div>";

        switch (h.type) {
        case DiffHunk::Header:
            // Full-width header in both panels
            leftHtml += "<div style='background:#2a2a4a; color:#8ab4d8; padding:2px 8px; "
                        "margin:4px 0 0 0;'>" + line + "</div>";
            rightHtml += "<div style='background:#2a2a4a; color:#8ab4d8; padding:2px 8px; "
                         "margin:4px 0 0 0;'>" + line + "</div>";
            break;

        case DiffHunk::Same:
            leftHtml  += "<div style='color:#666; padding:0 8px;'>  " + line + "</div>";
            rightHtml += "<div style='color:#666; padding:0 8px;'>  " + line + "</div>";
            break;

        case DiffHunk::Removed:
            // Only in left — highlight red
            leftHtml  += "<div style='background:#3a1b1b; color:#d6b5b5; padding:0 8px;'>- "
                        + line + "</div>";
            rightHtml += emptyRow;
            break;

        case DiffHunk::Added:
            // Only in right — highlight green
            leftHtml  += emptyRow;
            rightHtml += "<div style='background:#1b3a1b; color:#b5d6b5; padding:0 8px;'>+ "
                        + line + "</div>";
            break;
        }
    }

    leftHtml  += end();
    rightHtml += end();

    return {leftHtml, rightHtml};
}

QString DiffEngine::summary(const QVector<DiffHunk> &hunks)
{
    int added = 0, removed = 0;
    for (const auto &h : hunks) {
        if (h.type == DiffHunk::Added)   ++added;
        if (h.type == DiffHunk::Removed) ++removed;
    }
    if (added == 0 && removed == 0)
        return QStringLiteral("无差异");
    return QString("+%1 −%2").arg(added).arg(removed);
}

// ============================================================
//   LCS table
// ============================================================

QVector<QVector<int>> DiffEngine::buildLcsTable(const QStringList &left,
                                                  const QStringList &right)
{
    int m = left.size();
    int n = right.size();

    QVector<QVector<int>> dp(m + 1, QVector<int>(n + 1, 0));

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (left[i - 1] == right[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = qMax(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    return dp;
}

// ============================================================
//   Backtrack to raw diff
// ============================================================

QVector<DiffHunk> DiffEngine::backtrack(const QVector<QVector<int>> &dp,
                                          const QStringList &left,
                                          const QStringList &right)
{
    int i = left.size();
    int j = right.size();

    // Collect in reverse order
    QVector<DiffHunk> reversed;

    while (i > 0 || j > 0) {
        DiffHunk h;
        if (i > 0 && j > 0 && left[i - 1] == right[j - 1]) {
            h.type = DiffHunk::Same;
            h.text = left[i - 1];
            h.leftLine = i;
            h.rightLine = j;
            --i; --j;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            h.type = DiffHunk::Added;
            h.text = right[j - 1];
            h.leftLine = -1;
            h.rightLine = j;
            --j;
        } else {
            h.type = DiffHunk::Removed;
            h.text = left[i - 1];
            h.leftLine = i;
            h.rightLine = -1;
            --i;
        }
        reversed.append(h);
    }

    // Reverse to forward order
    QVector<DiffHunk> result;
    result.reserve(reversed.size());
    for (int k = reversed.size() - 1; k >= 0; --k)
        result.append(reversed[k]);

    return result;
}
