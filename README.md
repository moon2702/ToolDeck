# ToolDeck

> 日常工具集成平台 — 一个清单文件，即成一个工具

ToolDeck 是一个基于 Qt6 的桌面应用，为日常小工具提供统一的**加载、运行、输出、对比**环境。每个工具只需一个 `manifest.json` + 可执行脚本，无需编写 UI。

---

## 界面

```
┌──────────────────────────────────────────────────────────┐
│ 文件  视图  模式  工具  帮助                                │
├────────────┬─────────────────────────────────────────────┤
│  侧边栏    │  输出面板                                     │
│            │                                              │
│  ┌──────┐  │  ┌──────────────────┬──────────────────┐    │
│  │ 系统  │  │  │ A: file-hash     │ B: file-hash     │    │
│  │  系统信息│  │  │ a.iso 0xabc...  │ b.iso 0xdef...   │    │
│  │  文件哈希│  │  └──────────────────┴──────────────────┘    │
│  │ 网络  │  │                                              │
│  │  端口检测│  │                                              │
│  │ 开发  │  │                                              │
│  │  你好世界│  │                                              │
│  └──────┘  │                                              │
│            │                                              │
│  [▶ 运行]  │                                              │
│  [■ 停止]  │                                              │
├────────────┴─────────────────────────────────────────────┤
│  就绪                                                    │
└──────────────────────────────────────────────────────────┘
```

### 核心功能

- **零 UI 开发** — 工具只需 manifest.json + 脚本，平台自动生成界面
- **智能表单** — 根据 `inputs` 定义自动生成参数输入对话框（文本框/文件选择/下拉/复选框/数字微调）
- **对比模式** — 双栏表单 + 左右分屏输出，便于对比不同输入的运行结果
- **实时输出** — 终端风格暗色输出面板，多工具 Tab 页管理
- **守护进程** — 支持 daemon 模式，持续运行并实时监控输出

---

## 快速开始

### 编译

#### Linux

```bash
git clone <repo-url>
cd ToolDeck
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/tooldeck
```

**依赖**: Qt 6.5+ (Core, Gui, Widgets), CMake 3.20+, C++17 编译器

```bash
# Arch
sudo pacman -S qt6-base cmake gcc

# Ubuntu/Debian
sudo apt install qt6-base-dev cmake g++

# Fedora
sudo dnf install qt6-qtbase-devel cmake gcc-c++
```

#### Windows (MSYS2)

1. **安装 MSYS2**

   从 [msys2.org](https://www.msys2.org/) 下载安装包，按指引完成安装。

2. **打开 MINGW64 终端**

   ⚠️ 务必使用「MSYS2 MINGW64」终端（开始菜单搜索 MINGW64），不要使用 MSYS2 终端。区别：

   | 终端 | 编译器目标 | 适用场景 |
   |------|-----------|---------|
   | **MINGW64** | 原生 Windows (x86_64) | ✅ 编译 ToolDeck |
   | MSYS2 | POSIX 模拟层 | ❌ 不适用 |
   | UCRT64 | 原生 Windows (UCRT) | 可选，需调整 |
   | CLANG64 | 原生 Windows (Clang) | 可选，需调整 |

3. **安装依赖**

   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-{qt6-base,cmake,gcc,toolchain} zip
   ```

   | 包名 | 用途 |
   |------|------|
   | `mingw-w64-x86_64-qt6-base` | Qt6 框架 (Core, Gui, Widgets) |
   | `mingw-w64-x86_64-cmake` | 构建系统 |
   | `mingw-w64-x86_64-gcc` | C/C++ 编译器 |
   | `mingw-w64-x86_64-toolchain` | 工具链 (strip, windeployqt 等) |
   | `zip` | 打包分发（仅构建脚本需要） |

4. **编译**

   ```bash
   git clone <repo-url>
   cd ToolDeck
   cmake -B build -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
   cmake --build build --parallel $(nproc)
   ./build/tooldeck.exe
   ```

   > **注意**：必须指定 `-G "MinGW Makefiles"`，否则 CMake 可能误用 MSVC 生成器。

### 添加第一个工具

```bash
mkdir -p ~/.config/tooldeck/tools/my-tool

cat > ~/.config/tooldeck/tools/my-tool/manifest.json << 'EOF'
{
  "name": "my-tool",
  "displayName": "我的工具",
  "command": "bash",
  "args": ["run.sh"]
}
EOF

cat > ~/.config/tooldeck/tools/my-tool/run.sh << 'EOF'
#!/bin/bash
echo "Hello ToolDeck!"
date
EOF
chmod +x ~/.config/tooldeck/tools/my-tool/run.sh
```

在 ToolDeck 中点击 **文件 → 刷新工具列表**，双击运行。

---

## 项目结构

```
ToolDeck/
├── CMakeLists.txt
├── README.md
├── docs/
│   └── developer-guide.md      ← manifest.json 配置手册
├── examples/                   ← 内置示例工具
│   ├── default-echo/           ← 系统信息 (可选参数)
│   ├── file-hash/              ← 文件哈希 (必填路径)
│   ├── hello-world/            ← 最小示例 (可选参数)
│   ├── port-check/             ← 端口检测 (必填主机+端口)
│   └── win-sysinfo/            ← Windows 系统诊断 (PowerShell)
├── scripts/
│   ├── build-appimage.sh       ← Linux AppImage 构建
│   ├── build-windows.sh        ← Windows MSYS2 构建
│   └── toolchain-mingw.cmake   ← mingw 交叉编译工具链
└── src/
    ├── main.cpp
    ├── core/
    │   ├── tool_manifest.h/cpp     ← manifest.json 解析
    │   ├── tool_registry.h/cpp     ← 工具发现与索引
    │   ├── tool_instance.h/cpp     ← QProcess 封装
    │   └── tool_manager.h/cpp      ← 实例生命周期管理
    └── ui/
        ├── main_window.h/cpp       ← 主窗口 + 模式切换
        ├── sidebar_widget.h/cpp    ← 侧边栏工具列表
        ├── dashboard_widget.h/cpp  ← 仪表盘卡片
        ├── output_panel.h/cpp      ← 输出面板 (Tab/对比)
        ├── tool_card.h/cpp         ← 工具状态卡片
        ├── input_dialog.h/cpp      ← 参数表单 (单栏)
        └── compare_input_dialog.h/cpp ← 参数表单 (双栏对比)
```

---

## 发布构建

### Linux AppImage

```bash
./scripts/build-appimage.sh
# 产出: build-appimage/ToolDeck-x86_64.AppImage
```

### Windows

**前置条件**：MSYS2 MINGW64 环境（详见上方 [Windows 编译步骤](#windows-msys2)）。

```bash
# 安装依赖（含打包工具）
pacman -S mingw-w64-x86_64-{qt6-base,cmake,gcc,toolchain} zip

# 一键构建 + 打包
./scripts/build-windows.sh

# 产出: build-windows/tooldeck-v0.1.0-windows-x86_64.zip
# 解压后双击 bin/tooldeck.exe 即可运行，无需安装 Qt
```

构建脚本自动完成：编译 → 收集 Qt DLL → 拷贝示例工具 → 打包 zip。

---

## 文档

| 文档 | 说明 |
|------|------|
| [developer-guide.md](docs/developer-guide.md) | manifest.json 配置手册，涵盖全部字段、inputs/argTemplate 语法、完整示例 |

---

## License

MIT
