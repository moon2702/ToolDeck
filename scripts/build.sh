#!/bin/bash
# ============================================================================
# ToolDeck 快捷编译脚本
#
# 用法:
#   ./scripts/build.sh Linux      # 编译 Linux 版本
#   ./scripts/build.sh Windows    # 编译 Windows 版本 (MSYS2 MINGW64)
#
# 产出:
#   build-linux/tooldeck          (Linux)
#   build-windows/tooldeck.exe    (Windows)
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

PLATFORM="${1:-}"

if [ -z "$PLATFORM" ]; then
    echo "用法: $0 <Linux|Windows>"
    echo ""
    echo "示例:"
    echo "  $0 Linux      # 编译 Linux 版本"
    echo "  $0 Windows    # 编译 Windows 版本 (需在 MSYS2 MINGW64 中运行)"
    exit 1
fi

PLATFORM=$(echo "$PLATFORM" | tr '[:upper:]' '[:lower:]')

case "$PLATFORM" in
    linux)
        BUILD_DIR="$PROJECT_DIR/build-linux"
        GENERATOR="Unix Makefiles"
        echo "╔══════════════════════════════════════════╗"
        echo "║  ToolDeck Linux 编译                      ║"
        echo "╚══════════════════════════════════════════╝"
        ;;

    windows)
        BUILD_DIR="$PROJECT_DIR/build-windows"
        GENERATOR="MinGW Makefiles"
        echo "╔══════════════════════════════════════════╗"
        echo "║  ToolDeck Windows 编译                    ║"
        echo "╚══════════════════════════════════════════╝"

        # 检查是否在 MINGW/msys 环境
        if ! uname -a | grep -iq "mingw\|msys"; then
            echo ""
            echo "  注意: 当前不在 MSYS2/MINGW64 环境"
            echo "  Windows 编译建议在 MSYS2 MINGW64 终端中进行"
            echo "  详见 README.md 中的 Windows 编译步骤"
            echo ""
        fi
        ;;

    *)
        echo "错误: 未知平台 '$1'，请指定 Linux 或 Windows"
        exit 1
        ;;
esac

echo ""
echo "[1/2] CMake 配置..."

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -G "$GENERATOR" \
    2>&1 | tail -1

echo "  ✓ 配置完成"

JOBS=$(nproc 2>/dev/null || echo 4)
echo "[2/2] 编译 (并行 $JOBS)..."

cmake --build "$BUILD_DIR" --parallel "$JOBS" 2>&1 | tail -1

echo "  ✓ 编译完成"
echo ""

# 确认产物
case "$PLATFORM" in
    linux)
        if [ -f "$BUILD_DIR/tooldeck" ]; then
            echo "✅ 产出: $BUILD_DIR/tooldeck"
        else
            echo "⚠️  未找到 tooldeck，请检查错误信息"
            exit 1
        fi
        ;;
    windows)
        if [ -f "$BUILD_DIR/tooldeck.exe" ]; then
            echo "✅ 产出: $BUILD_DIR/tooldeck.exe"
        else
            echo "⚠️  未找到 tooldeck.exe，请检查错误信息"
            exit 1
        fi
        ;;
esac
