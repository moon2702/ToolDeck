#include "diff_engine.h"
#include <QStringList>
#include <algorithm>

// ============================================================
//   Public API — line-level diff
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

    int n = raw.size();

    // Mark lines to keep (changes + context)
    QVector<bool> keep(n, false);
    for (int i = 0; i < n; ++i) {
        if (raw[i].type == DiffHunk::Added || raw[i].type == DiffHunk::Removed) {
            keep[i] = true;
            for (int k = 1; k <= contextLines && i - k >= 0; ++k) {
                if (raw[i - k].type == DiffHunk::Same) keep[i - k] = true;
            }
            for (int k = 1; k <= contextLines && i + k < n; ++k) {
                if (raw[i + k].type == DiffHunk::Same) keep[i + k] = true;
            }
        }
    }

    bool hasChanges = false;
    for (bool k : keep) { if (k) { hasChanges = true; break; } }
    if (!hasChanges) return {};

    // Emit hunks in blocks with headers
    QVector<DiffHunk> result;
    bool inBlock = false;
    int  blockLeftStart = 0, blockRightStart = 0;
    int  blockLeftCount = 0, blockRightCount = 0;

    for (int i = 0; i < n; ++i) {
        if (!keep[i]) {
            if (inBlock) inBlock = false;
            continue;
        }

        const auto &h = raw[i];

        if (!inBlock) {
            inBlock = true;

            int leftMin  = h.leftLine  > 0 ? h.leftLine  : 999999;
            int rightMin = h.rightLine > 0 ? h.rightLine : 999999;

            for (int k = i - 1; k >= 0 && keep[k] && raw[k].type == DiffHunk::Same; --k) {
                if (raw[k].leftLine  > 0 && raw[k].leftLine  < leftMin)  leftMin  = raw[k].leftLine;
                if (raw[k].rightLine > 0 && raw[k].rightLine < rightMin) rightMin = raw[k].rightLine;
            }

            int leftCount  = 0;
            int rightCount = 0;
            for (int k = i; k < n; ++k) {
                if (!keep[k]) break;
                const auto &hk = raw[k];
                if (hk.type == DiffHunk::Added)      { if (hk.rightLine > 0) ++rightCount; }
                else if (hk.type == DiffHunk::Removed) { if (hk.leftLine > 0)  ++leftCount;  }
                else { if (hk.leftLine > 0) ++leftCount; if (hk.rightLine > 0) ++rightCount; }

                if (k + 1 < n && !keep[k + 1]) break;
                if (k + 1 < n && keep[k + 1] && raw[k].type == DiffHunk::Same
                    && raw[k + 1].type == DiffHunk::Same) {
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

            DiffHunk header;
            header.type = DiffHunk::Header;
            header.text = QString("@@ -%1,%2 +%3,%4 @@")
                              .arg(blockLeftStart).arg(blockLeftCount)
                              .arg(blockRightStart).arg(blockRightCount);
            result.append(header);
        }

        result.append(h);
    }

    bool anyChange = false;
    for (const auto &h : result) {
        if (h.type == DiffHunk::Added || h.type == DiffHunk::Removed) {
            anyChange = true; break;
        }
    }
    if (!anyChange) return {};

    return result;
}

// ============================================================
//   Character-level inline diff
// ============================================================

void DiffEngine::enrichInline(QVector<DiffHunk> &hunks)
{
    int n = hunks.size();

    for (int i = 0; i < n; ++i) {
        if (hunks[i].type != DiffHunk::Removed) continue;

        // Collect consecutive Removed lines
        int remEnd = i;
        while (remEnd < n && hunks[remEnd].type == DiffHunk::Removed) ++remEnd;
        int remCount = remEnd - i;

        // Collect consecutive Added lines (skip Headers)
        int addStart = remEnd;
        while (addStart < n && hunks[addStart].type == DiffHunk::Header) ++addStart;
        int addEnd = addStart;
        while (addEnd < n && hunks[addEnd].type == DiffHunk::Added) ++addEnd;
        int addCount = addEnd - addStart;

        if (addCount == 0) { i = remEnd - 1; continue; }

        // Greedy similarity-based pairing
        // Build similarity matrix
        struct Pair { int ri; int ai; double sim; };
        QVector<Pair> candidates;
        candidates.reserve(remCount * addCount);
        for (int r = 0; r < remCount; ++r) {
            for (int a = 0; a < addCount; ++a) {
                double s = similarity(hunks[i + r].text, hunks[addStart + a].text);
                candidates.append({r, a, s});
            }
        }

        // Sort by similarity descending
        std::sort(candidates.begin(), candidates.end(),
                  [](const Pair &x, const Pair &y) { return x.sim > y.sim; });

        QVector<bool> remUsed(remCount, false);
        QVector<bool> addUsed(addCount, false);

        for (const auto &c : candidates) {
            if (remUsed[c.ri] || addUsed[c.ai]) continue;
            if (c.sim < 0.35) break; // threshold: lines too different to align

            remUsed[c.ri] = true;
            addUsed[c.ai] = true;
            pairInline(hunks[i + c.ri], hunks[addStart + c.ai]);
        }

        i = addEnd - 1;
        if (i < remEnd - 1) i = remEnd - 1;
    }
}

double DiffEngine::similarity(const QString &a, const QString &b)
{
    if (a.isEmpty() && b.isEmpty()) return 1.0;
    auto dp = buildCharLcsTable(a, b);
    int lcsLen = dp[a.size()][b.size()];
    int maxLen = qMax(a.size(), b.size());
    return maxLen > 0 ? double(lcsLen) / maxLen : 0.0;
}

void DiffEngine::pairInline(DiffHunk &removed, DiffHunk &added)
{
    auto dp = buildCharLcsTable(removed.text, added.text);
    auto chunks = backtrackChars(dp, removed.text, added.text);

    QVector<InlineChunk> remChunks, addChunks;
    for (const auto &ch : chunks) {
        if (ch.type == InlineChunk::Same) {
            remChunks.append(ch);
            addChunks.append(ch);
        } else if (ch.type == InlineChunk::Removed) {
            remChunks.append(ch);
        } else {
            addChunks.append(ch);
        }
    }
    removed.inlineChunks = remChunks;
    added.inlineChunks = addChunks;
}

QVector<QVector<int>> DiffEngine::buildCharLcsTable(const QString &left,
                                                      const QString &right)
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

QVector<InlineChunk> DiffEngine::backtrackChars(const QVector<QVector<int>> &dp,
                                                  const QString &left,
                                                  const QString &right)
{
    int i = left.size();
    int j = right.size();

    QVector<InlineChunk> reversed;

    while (i > 0 || j > 0) {
        InlineChunk ch;
        if (i > 0 && j > 0 && left[i - 1] == right[j - 1]) {
            ch.type = InlineChunk::Same;
            ch.text = QString(left[i - 1]);
            --i; --j;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            ch.type = InlineChunk::Added;
            ch.text = QString(right[j - 1]);
            --j;
        } else {
            ch.type = InlineChunk::Removed;
            ch.text = QString(left[i - 1]);
            --i;
        }
        reversed.append(ch);
    }

    // Reverse and merge adjacent Same chars
    QVector<InlineChunk> result;
    for (int k = reversed.size() - 1; k >= 0; --k) {
        if (!result.isEmpty()
            && result.last().type == InlineChunk::Same
            && reversed[k].type == InlineChunk::Same) {
            result.last().text += reversed[k].text;
        } else {
            result.append(reversed[k]);
        }
    }

    return result;
}

// ============================================================
//   Render a line with inline diff highlighting
// ============================================================

QString DiffEngine::renderInlineLine(const DiffHunk &hunk, const QString &prefix,
                                      const QString &baseBg, const QString &baseFg)
{
    if (hunk.inlineChunks.isEmpty()) {
        // No inline data — plain line
        return "<div style='background:" + baseBg + "; color:" + baseFg
               + "; padding:0 8px;'>" + prefix + hunk.text.toHtmlEscaped() + "</div>";
    }

    // Build inline HTML with character-level highlights
    QString inner;
    for (const auto &ch : hunk.inlineChunks) {
        QString esc = ch.text.toHtmlEscaped();
        // Replace leading/trailing spaces with &nbsp; to preserve whitespace
        // (QTextEdit with white-space:pre should handle this, but we normalize anyway)
        switch (ch.type) {
        case InlineChunk::Same:
            inner += "<span style='color:" + baseFg + ";'>" + esc + "</span>";
            break;
        case InlineChunk::Removed:
            inner += "<span style='background:#6b2020; color:#ffaaaa; border-radius:2px;'>"
                   + esc + "</span>";
            break;
        case InlineChunk::Added:
            inner += "<span style='background:#206b20; color:#aaffaa; border-radius:2px;'>"
                   + esc + "</span>";
            break;
        }
    }

    return "<div style='background:" + baseBg + "; color:" + baseFg
           + "; padding:0 8px;'>" + prefix + inner + "</div>";
}

// ============================================================
//   HTML output
// ============================================================

QString DiffEngine::toHtmlUnified(const QVector<DiffHunk> &hunks)
{
    if (hunks.isEmpty()) return {};

    int totalAdded = 0, totalRemoved = 0;
    for (const auto &h : hunks) {
        if (h.type == DiffHunk::Added)   ++totalAdded;
        if (h.type == DiffHunk::Removed) ++totalRemoved;
    }

    QString html;
    html += "<html><body style='background:#1e1e1e; color:#d4d4d4; font-family:monospace; "
            "font-size:12px; margin:0; padding:6px; white-space:pre;'>";

    for (const auto &h : hunks) {
        switch (h.type) {
        case DiffHunk::Header:
            html += "<div style='background:#2a2a4a; color:#8ab4d8; padding:2px 8px; "
                    "margin:4px 0 0 0;'>"
                  + h.text.toHtmlEscaped() + "</div>";
            break;
        case DiffHunk::Added:
            html += renderInlineLine(h, "+ ", "#1b3a1b", "#b5d6b5");
            break;
        case DiffHunk::Removed:
            html += renderInlineLine(h, "- ", "#3a1b1b", "#d6b5b5");
            break;
        case DiffHunk::Same:
            html += "<div style='color:#666; padding:0 8px;'>  "
                  + h.text.toHtmlEscaped() + "</div>";
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

    auto startDoc = []() -> QString {
        return "<html><body style='background:#1e1e1e; color:#d4d4d4; font-family:monospace; "
               "font-size:12px; margin:0; padding:0; white-space:pre;'>";
    };
    auto endDoc = []() -> QString { return "</body></html>"; };
    QString emptyRow = "<div style='height:18px;'>&nbsp;</div>";

    QString leftHtml  = startDoc();
    QString rightHtml = startDoc();

    int n = hunks.size();
    int idx = 0;

    while (idx < n) {
        const auto &h = hunks[idx];

        if (h.type == DiffHunk::Header) {
            QString line = h.text.toHtmlEscaped();
            leftHtml  += "<div style='background:#2a2a4a; color:#8ab4d8; padding:2px 8px; margin:4px 0 0 0;'>" + line + "</div>";
            rightHtml += "<div style='background:#2a2a4a; color:#8ab4d8; padding:2px 8px; margin:4px 0 0 0;'>" + line + "</div>";
            ++idx;
            continue;
        }

        if (h.type == DiffHunk::Same) {
            QString line = h.text.toHtmlEscaped();
            leftHtml  += "<div style='color:#666; padding:0 8px;'>  " + line + "</div>";
            rightHtml += "<div style='color:#666; padding:0 8px;'>  " + line + "</div>";
            ++idx;
            continue;
        }

        // --- Change block: collect consecutive Removed + Added lines ---
        int remStart = idx;
        while (remStart < n && hunks[remStart].type != DiffHunk::Removed) ++remStart;
        int remEnd = remStart;
        while (remEnd < n && hunks[remEnd].type == DiffHunk::Removed) ++remEnd;

        int addStart = remEnd;
        while (addStart < n && hunks[addStart].type == DiffHunk::Header) ++addStart;
        int addEnd = addStart;
        while (addEnd < n && hunks[addEnd].type == DiffHunk::Added) ++addEnd;

        int remCount = remEnd - remStart;
        int addCount = addEnd - addStart;

        if (remCount == 0 && addCount == 0) {
            // No change lines — emit anything between remStart and addEnd as-is
            for (int k = idx; k < addEnd; ++k) {
                if (hunks[k].type == DiffHunk::Same) {
                    QString line = hunks[k].text.toHtmlEscaped();
                    leftHtml  += "<div style='color:#666; padding:0 8px;'>  " + line + "</div>";
                    rightHtml += "<div style='color:#666; padding:0 8px;'>  " + line + "</div>";
                }
            }
            idx = addEnd;
            continue;
        }

        // Compute similarity matrix and greedy match
        struct MatchPair { int ri; int ai; double sim; };
        QVector<MatchPair> pairs;
        pairs.reserve(remCount * addCount);
        for (int r = 0; r < remCount; ++r) {
            for (int a = 0; a < addCount; ++a) {
                double s = similarity(hunks[remStart + r].text, hunks[addStart + a].text);
                if (s >= 0.35)
                    pairs.append({r, a, s});
            }
        }
        std::sort(pairs.begin(), pairs.end(),
                  [](const MatchPair &x, const MatchPair &y) { return x.sim > y.sim; });

        QVector<bool> remUsed(remCount, false);
        QVector<bool> addUsed(addCount, false);
        QVector<MatchPair> matched;
        for (const auto &p : pairs) {
            if (!remUsed[p.ri] && !addUsed[p.ai]) {
                remUsed[p.ri] = true;
                addUsed[p.ai] = true;
                matched.append(p);
            }
        }

        // Compute inline diff for matched pairs on the fly
        for (auto &m : matched) {
            DiffHunk remCopy = hunks[remStart + m.ri];
            DiffHunk addCopy = hunks[addStart + m.ai];
            pairInline(remCopy, addCopy);

            leftHtml  += renderInlineLine(remCopy, "- ", "#3a1b1b", "#d6b5b5");
            rightHtml += renderInlineLine(addCopy, "+ ", "#1b3a1b", "#b5d6b5");
        }

        // Emit unmatched Removed lines (left only)
        for (int r = 0; r < remCount; ++r) {
            if (!remUsed[r]) {
                leftHtml += renderInlineLine(hunks[remStart + r], "- ", "#3a1b1b", "#d6b5b5");
                rightHtml += emptyRow;
            }
        }

        // Emit unmatched Added lines (right only)
        for (int a = 0; a < addCount; ++a) {
            if (!addUsed[a]) {
                leftHtml += emptyRow;
                rightHtml += renderInlineLine(hunks[addStart + a], "+ ", "#1b3a1b", "#b5d6b5");
            }
        }

        idx = addEnd;
    }

    leftHtml  += endDoc();
    rightHtml += endDoc();
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
//   Line-level LCS
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

QVector<DiffHunk> DiffEngine::backtrack(const QVector<QVector<int>> &dp,
                                          const QStringList &left,
                                          const QStringList &right)
{
    int i = left.size();
    int j = right.size();

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

    QVector<DiffHunk> result;
    result.reserve(reversed.size());
    for (int k = reversed.size() - 1; k >= 0; --k)
        result.append(reversed[k]);

    return result;
}
