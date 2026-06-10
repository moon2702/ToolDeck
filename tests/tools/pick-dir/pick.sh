#!/bin/bash
# pick-dir — 目录内容统计 (覆盖 InputType::Dir + required)
set -e

DIR="$1"

if [ ! -d "$DIR" ]; then
    echo "错误: 目录不存在 — $DIR"
    exit 1
fi

echo "══════ 目录统计 ══════"
echo "路径: $DIR"
echo ""

files=$(find "$DIR" -maxdepth 1 -type f 2>/dev/null | wc -l)
dirs=$(find "$DIR" -maxdepth 1 -type d 2>/dev/null | wc -l)
size=$(du -sh "$DIR" 2>/dev/null | cut -f1)

echo "文件数: $files"
echo "子目录: $((dirs - 1))"  # -1 排除自身
echo "总大小: $size"
echo ""
echo "内容列表:"
ls -lh "$DIR"
