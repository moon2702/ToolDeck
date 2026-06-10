#!/bin/bash
# all-needed — 全部字段必填 (覆盖全 required 输入组合)
set -e

NAME="$1"
SOURCE="$2"
OUTDIR="$3"
FORMAT="$4"

echo "══════ 参数汇总 ══════"
echo "名称:   $NAME"
echo "源文件: $SOURCE"
echo "输出目录: $OUTDIR"
echo "格式:   $FORMAT"
echo ""

if [ ! -f "$SOURCE" ]; then
    echo "错误: 源文件不存在 — $SOURCE"
    exit 1
fi

mkdir -p "$OUTDIR"

case "$FORMAT" in
    JSON)  echo '{ "name": "'"$NAME"'", "size": '"$(stat -c%s "$SOURCE" 2>/dev/null || stat -f%z "$SOURCE")"' }' ;;
    YAML)  echo "name: $NAME"; echo "size: $(stat -c%s "$SOURCE" 2>/dev/null || stat -f%z "$SOURCE")" ;;
    TEXT)  echo "name=$NAME";  echo "size=$(stat -c%s "$SOURCE" 2>/dev/null || stat -f%z "$SOURCE")" ;;
    *)     echo "未知格式: $FORMAT"; exit 1 ;;
esac
