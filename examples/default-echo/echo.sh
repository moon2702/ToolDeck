#!/bin/bash
# ------------------------------------------------------------------
# 系统信息工具 — 输出系统概览
# 参数: 简要 | 标准 | 详细  (默认: 标准)
# ------------------------------------------------------------------
set -euo pipefail

LEVEL="${1:-标准}"

echo "=== ToolDeck 系统信息 ==="
echo "日期: $(date '+%Y-%m-%d %H:%M:%S')"
echo "用户: $(whoami)"
echo "主机: $(hostname)"
echo "内核: $(uname -r)"
echo "Shell: $SHELL"

if [ "$LEVEL" = "标准" ] || [ "$LEVEL" = "详细" ]; then
    echo ""
    echo "--- 运行时间 ---"
    uptime
fi

if [ "$LEVEL" = "详细" ]; then
    echo ""
    echo "--- 内存使用 ---"
    free -h | head -2
    echo ""
    echo "--- 磁盘使用 ---"
    df -h / | tail -1 | awk '{print "  / : " $3 "/" $2 " (" $5 ")"}'
fi

echo ""
echo "✅ 完成"
