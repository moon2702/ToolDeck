#!/bin/bash
# zip-inspect — 浏览 ZIP 包内容清单, 按需提取指定文件
# 用法: zip-inspect.sh <zipfile> <mode> [pattern] [outdir]

set -euo pipefail

ZIPFILE="$1"
MODE="$2"
PATTERN="${3:-}"
OUTDIR="${4:-.}"

if [ ! -f "$ZIPFILE" ]; then
    echo "错误: 文件不存在 — $ZIPFILE"
    exit 1
fi

if ! file "$ZIPFILE" | grep -qi 'zip'; then
    echo "警告: 该文件可能不是 ZIP 格式"
    file "$ZIPFILE"
fi

case "$MODE" in
    列出清单)
        echo "══════ ZIP 文件信息 ══════"
        echo "文件: $ZIPFILE"
        echo "大小: $(du -h "$ZIPFILE" | cut -f1)"
        echo "条目数: $(unzip -l "$ZIPFILE" 2>/dev/null | tail -2 | head -1 | awk '{print $2}')"
        echo ""
        unzip -l "$ZIPFILE"
        ;;

    提取文件)
        if [ -z "$PATTERN" ]; then
            echo "错误: 提取模式需要指定文件路径(在 ZIP 内的路径)"
            echo "提示: 先用「列出清单」查看 ZIP 内文件列表"
            exit 1
        fi

        mkdir -p "$OUTDIR"

        echo "ZIP 文件: $ZIPFILE"
        echo "提取目标: $PATTERN"
        echo "输出目录: $OUTDIR"
        echo ""

        # -j: 不保留 ZIP 内目录结构, 文件扁平提取到输出目录
        # -o: 覆盖已有文件不提示
        if unzip -jo "$ZIPFILE" "$PATTERN" -d "$OUTDIR" 2>&1; then
            echo ""
            echo "══════ 提取完成 ══════"
            ls -lh "$OUTDIR"
        else
            echo ""
            echo "错误: 未找到匹配条目「$PATTERN」"
            echo "可用通配符如 '*.txt' 或 'subdir/*'"
            exit 1
        fi
        ;;

    *)
        echo "错误: 未知模式 '$MODE'"
        exit 1
        ;;
esac
