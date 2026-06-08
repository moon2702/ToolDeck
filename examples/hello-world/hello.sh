#!/bin/bash
# ------------------------------------------------------------------
# 你好世界 — ToolDeck 最小示例工具
# 参数: [问候对象] [输出格式: 简洁|标准|完整]
# ------------------------------------------------------------------
set -euo pipefail

NAME="${1:-$(whoami)}"
FORMAT="${2:-标准}"

if [ "$FORMAT" = "简洁" ]; then
    echo "👋 你好，${NAME}！"
    echo "今天是 $(date '+%Y-%m-%d %H:%M')"
    exit 0
fi

echo "👋 你好，${NAME}！"
echo "欢迎使用 ToolDeck — 日常工具集成平台。"
echo ""

if [ "$FORMAT" = "完整" ]; then
    echo "这是一个运行在 ToolDeck 平台中的示例工具。"
    echo "你可以参考本工具的结构来开发自己的工具。"
    echo ""
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  系统信息"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  日期    : $(date '+%Y-%m-%d %H:%M:%S')"
echo "  用户    : $(whoami)"
echo "  主机    : $(hostname)"
echo "  内核    : $(uname -r)"
echo "  架构    : $(uname -m)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "📁 工具目录: ~/.config/tooldeck/tools/"
echo "📄 配置文件: manifest.json"
echo ""
echo "✅ 工具执行成功！"
