#!/bin/bash
# ============================================================================
# ToolDeck AppImage 构建脚本 (Linux)
#
# 产出: ToolDeck-x86_64.AppImage  (自包含，无需安装 Qt)
#
# 用法:
#   ./scripts/build-appimage.sh
#
# 依赖 (脚本会自动检查):
#   - cmake, g++, Qt6 (编译用)
#   - linuxdeploy-x86_64.AppImage (打包用, 自动下载)
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build-appimage"
APPDIR="$BUILD_DIR/AppDir"
VERSION="0.1.0"
APPNAME="ToolDeck"

echo "╔══════════════════════════════════════════╗"
echo "║  ToolDeck AppImage 构建                  ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# ---- 1. 检查编译依赖 ----
echo "[1/5] 检查依赖..."

MISSING=""
for cmd in cmake g++; do
    if ! command -v "$cmd" &>/dev/null; then
        MISSING="$MISSING $cmd"
    fi
done

if ! pkg-config --exists Qt6Core Qt6Gui Qt6Widgets 2>/dev/null; then
    echo "  ⚠ Qt6 开发库未找到, 请先安装:"
    echo "    Arch:   sudo pacman -S qt6-base"
    echo "    Debian: sudo apt install qt6-base-dev"
    echo "    Fedora: sudo dnf install qt6-qtbase-devel"
    exit 1
fi

if [ -n "$MISSING" ]; then
    echo "  ✗ 缺少: $MISSING"
    exit 1
fi
echo "  ✓ 编译依赖就绪"

# ---- 2. 下载 linuxdeploy ----
echo "[2/5] 准备 linuxdeploy..."

LINUXDEPLOY="$BUILD_DIR/linuxdeploy-x86_64.AppImage"
if [ ! -f "$LINUXDEPLOY" ]; then
    mkdir -p "$BUILD_DIR"
    echo "  下载 linuxdeploy..."
    wget -q -O "$LINUXDEPLOY" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
        2>/dev/null || curl -sSL -o "$LINUXDEPLOY" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY"
    echo "  ✓ linuxdeploy 就绪"
else
    echo "  ✓ linuxdeploy 已存在"
fi

# ---- 3. 编译 Release 版本 ----
echo "[3/5] 编译 Release 版本..."

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    2>&1 | tail -1

cmake --build "$BUILD_DIR/build" --parallel $(nproc) 2>&1 | tail -1

echo "  ✓ 编译完成"

# ---- 4. 打包 AppDir ----
echo "[4/5] 打包 AppDir..."

# 创建 AppDir 结构
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/share/tooldeck/examples"

# 复制二进制
cp "$BUILD_DIR/build/tooldeck" "$APPDIR/usr/bin/"

# 复制示例工具
cp -r "$PROJECT_DIR/examples/"* "$APPDIR/usr/share/tooldeck/examples/"

# 复制文档
mkdir -p "$APPDIR/usr/share/tooldeck/docs"
cp "$PROJECT_DIR/docs/developer-guide.md" "$APPDIR/usr/share/tooldeck/docs/"

# 创建 .desktop 文件 (需同时放在根目录和 usr/share/applications/)
cat > "$APPDIR/tooldeck.desktop" << 'DESKTOP'
[Desktop Entry]
Name=ToolDeck
Comment=日常工具集成平台
Exec=tooldeck
Icon=tooldeck
Terminal=false
Type=Application
Categories=Utility;
DESKTOP
cp "$APPDIR/tooldeck.desktop" "$APPDIR/usr/share/applications/tooldeck.desktop"

# 创建启动脚本 (设置 examples 路径)
cat > "$APPDIR/AppRun" << 'APPRUN'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="$HERE/usr/bin:$PATH"
exec "$HERE/usr/bin/tooldeck" "$@"
APPRUN
chmod +x "$APPDIR/AppRun"

# 生成简单 icon (1x1 透明 PNG 占位, 避免 linuxdeploy 报错)
# 实际图标可用 QIcon::fromTheme 在运行时加载
python3 -c "
import struct, zlib
def create_png(path):
    sig = b'\\x89PNG\\r\\n\\x1a\\n'
    ihdr_data = struct.pack('>IIBBBBB', 1, 1, 8, 2, 0, 0, 0)
    ihdr_crc = zlib.crc32(b'IHDR' + ihdr_data)
    ihdr = struct.pack('>I', 13) + b'IHDR' + ihdr_data + struct.pack('>I', ihdr_crc)
    idat_data = zlib.compress(b'\\x00\\xff\\x00\\x00')
    idat_crc = zlib.crc32(b'IDAT' + idat_data)
    idat = struct.pack('>I', len(idat_data)) + b'IDAT' + idat_data + struct.pack('>I', idat_crc)
    iend = struct.pack('>I', 0) + b'IEND' + struct.pack('>I', zlib.crc32(b'IEND'))
    with open(path, 'wb') as f: f.write(sig + ihdr + idat + iend)
create_png('$APPDIR/usr/share/icons/hicolor/256x256/apps/tooldeck.png')
create_png('$APPDIR/tooldeck.png')
" 2>/dev/null || { touch "$APPDIR/usr/share/icons/hicolor/256x256/apps/tooldeck.png"; touch "$APPDIR/tooldeck.png"; }

# ---- 4a. 使用 linuxdeploy 收集 Qt 依赖到 AppDir ----
echo "  收集 Qt 依赖..."
cd "$BUILD_DIR"
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/tooldeck" \
    --desktop-file "$APPDIR/usr/share/applications/tooldeck.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/tooldeck.png" \
    2>&1 | grep -E "Deploying|ERROR|WARNING" || true

echo "  ✓ Qt 依赖已收集"

# ---- 4b. 下载 appimagetool 打包 AppImage ----
echo "  打包 AppImage..."
APPIMAGETOOL="$BUILD_DIR/appimagetool-x86_64.AppImage"
if [ ! -f "$APPIMAGETOOL" ]; then
    wget -q -O "$APPIMAGETOOL" \
        "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" \
        2>/dev/null || curl -sSL -o "$APPIMAGETOOL" \
        "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod +x "$APPIMAGETOOL"
fi

# 用系统 strip 预处理，避免 appimagetool 内旧版 strip 失败
strip "$APPDIR/usr/bin/tooldeck" 2>/dev/null || true
find "$APPDIR/usr/lib" -name "*.so*" -exec strip --strip-unneeded {} \; 2>/dev/null || true

ARCH=x86_64 "$APPIMAGETOOL" --no-appstream "$APPDIR" \
    "$BUILD_DIR/ToolDeck-${VERSION}-x86_64.AppImage" 2>&1 | grep -E "AppImage|done|Error" || true

APPIMAGE=$(ls "$BUILD_DIR"/ToolDeck-*.AppImage 2>/dev/null | head -1)

if [ -n "$APPIMAGE" ]; then
    echo ""
    echo "╔══════════════════════════════════════════╗"
    echo "║  ✅ AppImage 构建成功!                    ║"
    echo "╠══════════════════════════════════════════╣"
    echo "║  $(basename "$APPIMAGE")"
    echo "║  $(du -h "$APPIMAGE" | cut -f1)"
    echo "╚══════════════════════════════════════════╝"
    echo ""
    echo "使用方法:"
    echo "  chmod +x $(basename "$APPIMAGE")"
    echo "  ./$(basename "$APPIMAGE")"
else
    echo ""
    echo "⚠ AppImage 生成失败，但 AppDir 可用:"
    echo "  $APPDIR"
    echo "  手动运行:  $APPDIR/AppRun"
fi
