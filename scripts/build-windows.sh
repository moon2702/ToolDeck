#!/bin/bash
# ============================================================================
# ToolDeck Windows 构建脚本 (交叉编译 或 本地 MSYS2)
#
# 产出: tooldeck-windows.zip
#        ├── tooldeck.exe
#        ├── *.dll              (Qt6 + 运行时)
#        └── examples/
#
# 用法:
#   方式一 (推荐): 在 Windows MSYS2 中运行
#     pacman -S mingw-w64-x86_64-{qt6-base,cmake,gcc}
#     ./scripts/build-windows.sh
#
#   方式二 (实验性): 在 Linux 上用 mingw-w64 交叉编译
#     需要安装: mingw-w64-qt6-base (部分发行版有)
#     ./scripts/build-windows.sh --cross
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build-windows"
DIST_DIR="$BUILD_DIR/tooldeck-windows"
VERSION="0.1.0"

echo "╔══════════════════════════════════════════╗"
echo "║  ToolDeck Windows 构建                   ║"
echo "╚══════════════════════════════════════════╝"
echo ""

MODE="${1:-native}"

# ============================================================
#   方式一: Windows MSYS2 本地构建
# ============================================================
if [ "$MODE" != "--cross" ]; then

    # 检测是否在 MSYS2 环境
    if ! uname -a | grep -iq "mingw\|msys"; then
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "  当前不在 Windows/MSYS2 环境"
        echo ""
        echo "  推荐方式 (Windows 本地):"
        echo "  ─────────────────────────────────────"
        echo "  1. 安装 MSYS2: https://www.msys2.org/"
        echo "  2. 打开 'MSYS2 MINGW64' 终端, 执行:"
        echo ""
        echo "     pacman -Syu"
        echo "     pacman -S mingw-w64-x86_64-{qt6-base,cmake,gcc,toolchain}"
        echo ""
        echo "  3. 进入 ToolDeck 项目目录, 运行:"
        echo "     ./scripts/build-windows.sh"
        echo "  ─────────────────────────────────────"
        echo ""
        echo "  实验性 Linux 交叉编译:"
        echo "    ./scripts/build-windows.sh --cross"
        echo "    (需要 mingw-w64-qt6, 部分发行版不可用)"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        exit 0
    fi

    echo "[1/4] 检查 MSYS2 依赖..."
    MISSING=""
    for cmd in cmake g++ strip; do
        if ! command -v "$cmd" &>/dev/null; then
            MISSING="$MISSING $cmd"
        fi
    done
    # windeployqt may be named windeployqt6 or windeployqt
    if command -v windeployqt6 &>/dev/null; then
        WINDEPLOYQT="windeployqt6"
    elif command -v windeployqt &>/dev/null; then
        WINDEPLOYQT="windeployqt"
    else
        MISSING="$MISSING windeployqt"
    fi
    if [ -n "$MISSING" ]; then
        echo "  ✗ 缺少: $MISSING"
        echo "  请安装: pacman -S mingw-w64-x86_64-{qt6-base,cmake,gcc,toolchain}"
        exit 1
    fi
    echo "  ✓ MSYS2 依赖就绪"

    echo "[2/4] 编译 Release 版本..."
    cmake -S "$PROJECT_DIR" -B "$BUILD_DIR/build" \
        -DCMAKE_BUILD_TYPE=Release \
        -G "MinGW Makefiles" \
        2>&1 | tail -1

    cmake --build "$BUILD_DIR/build" --parallel $(nproc 2>/dev/null || echo 4) 2>&1 | tail -1

    echo "  ✓ 编译完成"

    echo "[3/4] 收集 Qt DLL..."
    rm -rf "$DIST_DIR"
    mkdir -p "$DIST_DIR/bin"
    mkdir -p "$DIST_DIR/examples"

    cp "$BUILD_DIR/build/tooldeck.exe" "$DIST_DIR/bin/"

    # windeployqt 自动收集所需 Qt DLL
    $WINDEPLOYQT "$DIST_DIR/bin/tooldeck.exe" --no-translations --no-compiler-runtime 2>&1 | tail -3

    # 自动递归收集 MinGW/GCC 运行时 DLL
    echo "  补充运行时 DLL..."
    MINGW_BIN="/mingw64/bin"

    # Windows 系统 DLL（无需打包，所有 Windows 自带）
    is_system_dll() {
        case "${1,,}" in
            kernel32.dll|msvcrt.dll|ntdll.dll|user32.dll|gdi32.dll|advapi32.dll|\
            shell32.dll|ole32.dll|oleaut32.dll|ws2_32.dll|version.dll|winmm.dll|\
            netapi32.dll|iphlpapi.dll|dnsapi.dll|crypt32.dll|bcrypt.dll|ncrypt.dll|\
            userenv.dll|mpr.dll|secur32.dll|setupapi.dll|wintrust.dll|cfgmgr32.dll|\
            imm32.dll|comdlg32.dll|rpcrt4.dll|shlwapi.dll|uxtheme.dll|dwmapi.dll|\
            opengl32.dll|winspool.dll|wtsapi32.dll|powrprof.dll|wldap32.dll|\
            netutils.dll|logoncli.dll|samcli.dll|winsta.dll|winhttp.dll|dxgi.dll|\
            d3d11.dll|d3d12.dll|dwrite.dll|cryptsp.dll|usp10.dll|authz.dll)
                return 0 ;;
            api-ms-win-*) return 0 ;;
            ext-ms-win-*) return 0 ;;
            *)            return 1 ;;
        esac
    }

    # 递归扫描：找出 bin/ 中所有 exe/dll 缺失的非系统 DLL，从 MINGW_BIN 拷贝
    collect_deps() {
        local scanned=()
        while true; do
            local missing=()
            for f in "$DIST_DIR/bin"/*.exe "$DIST_DIR/bin"/*.dll; do
                [ -f "$f" ] || continue
                while read -r dll; do
                    dll=$(echo "$dll" | tr -d '\r')
                    if is_system_dll "$dll"; then continue; fi
                    local target="$DIST_DIR/bin/$dll"
                    if [ ! -f "$target" ] && [ ! -f "$MINGW_BIN/$dll" ]; then
                        # DLL 可能来自 Qt 插件子目录（如 platforms/qwindows.dll 从 windeployqt 已部署）
                        # 忽略扫描过程中出现在子目录的依赖
                        true
                    elif [ ! -f "$target" ] && [ -f "$MINGW_BIN/$dll" ]; then
                        missing+=("$dll")
                    fi
                done < <(objdump -p "$f" 2>/dev/null | grep "DLL Name" | sed 's/.*DLL Name: //')
            done

            if [ ${#missing[@]} -eq 0 ]; then
                break
            fi

            for dll in "${missing[@]}"; do
                if ! echo "${scanned[@]}" | grep -qw "$dll"; then
                    cp "$MINGW_BIN/$dll" "$DIST_DIR/bin/"
                    scanned+=("$dll")
                fi
            done
        done
        echo "    已收集 ${#scanned[@]} 个运行时 DLL"
    }

    collect_deps

    # ---- 4. 打包 ----
    echo "[4/4] 打包..."
    cp -r "$PROJECT_DIR/examples/"* "$DIST_DIR/examples/"
    cp "$PROJECT_DIR/docs/developer-guide.md" "$DIST_DIR/" 2>/dev/null || true

    # 创建简单启动说明
    cat > "$DIST_DIR/使用说明.txt" << 'EOF'
ToolDeck - 日常工具集成平台
============================

启动: 双击 bin/tooldeck.exe

添加工具: 将工具目录放入:
  %APPDATA%/tooldeck/tools/

示例工具在 examples/ 目录中, 启动后自动加载。

详细配置方法见 developer-guide.md
EOF

    cd "$BUILD_DIR"
    ZIPNAME="tooldeck-v${VERSION}-windows-x86_64.zip"
    rm -f "$ZIPNAME"
    zip -r "$ZIPNAME" "tooldeck-windows" -x "*.c" "*.o" 2>&1 | tail -1

    echo ""
    echo "╔══════════════════════════════════════════╗"
    echo "║  ✅ Windows 构建成功!                     ║"
    echo "╠══════════════════════════════════════════╣"
    echo "║  $ZIPNAME"
    echo "║  $(du -h "$ZIPNAME" | cut -f1)"
    echo "╚══════════════════════════════════════════╝"
    echo ""
    echo "解压后即可使用, 无需安装 Qt"
    exit 0
fi

# ============================================================
#   方式二: Linux mingw-w64 交叉编译 (实验性)
# ============================================================
echo "[交叉编译模式] 检查工具链..."

# 检查 mingw 交叉编译器
CROSS_PREFIX="x86_64-w64-mingw32"
if ! command -v "${CROSS_PREFIX}-g++" &>/dev/null; then
    echo "  ✗ mingw-w64 未安装"
    echo ""
    echo "  安装方式 (Arch):"
    echo "    sudo pacman -S mingw-w64-gcc mingw-w64-qt6-base"
    echo ""
    echo "  安装方式 (Ubuntu):"
    echo "    sudo apt install mingw-w64 qt6-base-dev"
    echo "    # 注意: Ubuntu 的 qt6 是 Linux 版本, 无法交叉编译 Windows 版"
    echo "    # 建议在 Windows MSYS2 中构建"
    echo ""
    echo "  安装方式 (Fedora):"
    echo "    sudo dnf install mingw64-gcc-c++ mingw64-qt6-qtbase"
    exit 1
fi

# 检查 mingw Qt6
if ! pkg-config --exists Qt6Core 2>/dev/null; then
    # 尝试 mingw 特定的 pkg-config 路径
    MINGW_PKG_CONFIG="/usr/${CROSS_PREFIX}/sys-root/mingw/lib/pkgconfig"
    if [ -d "$MINGW_PKG_CONFIG" ]; then
        export PKG_CONFIG_PATH="$MINGW_PKG_CONFIG:$PKG_CONFIG_PATH"
    fi
fi

echo "  ✓ mingw-w64 工具链就绪"

echo "[2/5] 编译 Release 版本 (mingw)..."
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/scripts/toolchain-mingw.cmake" \
    -DCMAKE_INSTALL_PREFIX=/usr \
    2>&1 | tail -3 || {
    echo "  ✗ CMake 配置失败"
    echo "  交叉编译 Qt6 需要 mingw 版本的 Qt, 当前环境可能不支持"
    echo "  建议在 Windows MSYS2 中构建"
    exit 1
}

cmake --build "$BUILD_DIR/build" --parallel $(nproc) 2>&1 | tail -1
echo "  ✓ 交叉编译完成"

echo "[3/5] 创建发布目录..."
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR/bin"
mkdir -p "$DIST_DIR/examples"

cp "$BUILD_DIR/build/tooldeck.exe" "$DIST_DIR/bin/" 2>/dev/null || {
    echo "  ✗ 未找到 tooldeck.exe"
    exit 1
}

# 手动收集 DLL
echo "[4/5] 收集依赖 DLL..."
MINGW_SYSROOT="/usr/${CROSS_PREFIX}"
QT_BIN="$MINGW_SYSROOT/bin"

DLLS=(
    "libQt6Core.dll" "libQt6Gui.dll" "libQt6Widgets.dll"
    "libgcc_s_seh-1.dll" "libstdc++-6.dll" "libwinpthread-1.dll"
    "libpng16-16.dll" "libharfbuzz-0.dll" "libfreetype-6.dll"
    "libglib-2.0-0.dll" "libintl-8.dll" "libiconv-2.dll"
    "libpcre2-8-0.dll" "libzstd.dll" "libmd4c.dll"
    "libdouble-conversion.dll" "libb2-1.dll" "libgraphite2.dll"
)

for dll in "${DLLS[@]}"; do
    if [ -f "$QT_BIN/$dll" ]; then
        cp "$QT_BIN/$dll" "$DIST_DIR/bin/"
    fi
done

# 如果 windeployqt 可用 (通过 mingw), 用它自动收集
MINGW_DEPLOY="$MINGW_SYSROOT/bin/windeployqt"
if [ -f "$MINGW_DEPLOY" ]; then
    "$MINGW_DEPLOY" "$DIST_DIR/bin/tooldeck.exe" --no-translations 2>&1 | tail -3 || true
fi

# ---- 5. 打包 ----
echo "[5/5] 打包..."
cp -r "$PROJECT_DIR/examples/"* "$DIST_DIR/examples/" 2>/dev/null || true

cd "$BUILD_DIR"
ZIPNAME="tooldeck-v${VERSION}-windows-x86_64-cross.zip"
rm -f "$ZIPNAME"
zip -r "$ZIPNAME" "tooldeck-windows" -x "*.c" 2>&1 | tail -1

echo ""
echo "╔══════════════════════════════════════════╗"
echo "║  ⚠ 交叉编译版本 — 未在 Windows 上测试     ║"
echo "║  $ZIPNAME                              ║"
echo "║  建议在 Windows 上实际验证后再分发        ║"
echo "╚══════════════════════════════════════════╝"
