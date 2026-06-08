#!/bin/bash
# ------------------------------------------------------------------
# file-hash 编译脚本
# 将 filehash.c 编译为 bin/filehash 二进制文件
# ------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$SCRIPT_DIR/filehash.c"
OUT="$SCRIPT_DIR/bin/filehash"

echo "=== 编译文件哈希工具 ==="

# 检查依赖
if ! pkg-config --exists libxxhash 2>/dev/null; then
    echo "错误: 需要 libxxhash 开发库"
    echo "  Arch:  sudo pacman -S xxhash"
    echo "  Debian: sudo apt install libxxhash-dev"
    exit 1
fi

CFLAGS=$(pkg-config --cflags libxxhash 2>/dev/null || echo "")
LDFLAGS=$(pkg-config --libs libxxhash 2>/dev/null || echo "-lxxhash")

mkdir -p "$(dirname "$OUT")"

echo "源码: $SRC"
echo "输出: $OUT"
echo ""

gcc -O3 -Wall -Wextra \
    $CFLAGS \
    -o "$OUT" \
    "$SRC" \
    $LDFLAGS

echo "编译成功: $OUT"
echo "大小: $(du -h "$OUT" | cut -f1)"
echo ""
echo "运行测试:"
echo "  $OUT -h"
"$OUT" -h
