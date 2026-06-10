#!/bin/bash
# quick-ok — 快速状态检测 (覆盖 outputMode=status)
# 以指定 exit code 退出，ToolDeck 仪表盘显示成功/失败
CODE="${1:-0}"

if [ "$CODE" -eq 0 ]; then
    echo "OK — 所有检查通过"
else
    echo "FAIL — 检测到问题 (exit=$CODE)"
    echo "提示: 该工具设 outputMode=status，"
    echo "  ToolDeck 不显示输出，仅仪表盘状态灯指示成功/失败"
fi

exit "$CODE"
