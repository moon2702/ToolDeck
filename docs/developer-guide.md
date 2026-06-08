# ToolDeck 开发指南

> 如何开发、添加各种类型的工具到 ToolDeck 平台

---

## 目录

1. [快速开始](#1-快速开始)
2. [工具结构](#2-工具结构)
3. [manifest.json 完整参考](#3-manifestjson-完整参考)
4. [运行模式详解](#4-运行模式详解)
5. [输出模式详解](#5-输出模式详解)
6. [实战示例](#6-实战示例)
   - [示例一：一次性命令行工具 (Bash)](#示例一一次性命令行工具-bash)
   - [示例二：带参数的 Python 脚本](#示例二带参数的-python-脚本)
   - [示例三：长时间运行的后台监控 (守护进程)](#示例三长时间运行的后台监控-守护进程)
   - [示例四：C/C++ 编译型工具](#示例四cc-编译型工具)
   - [示例五：定时周期任务](#示例五定时周期任务)
   - [示例六：混合语言工具 (Bash 调用 Python)](#示例六混合语言工具-bash-调用-python)
7. [分类与图标](#7-分类与图标)
8. [调试技巧](#8-调试技巧)
9. [最佳实践](#9-最佳实践)

---

## 1. 快速开始

### 三步添加一个工具

```bash
# 第一步：创建工具目录
mkdir -p ~/.config/tooldeck/tools/my-tool

# 第二步：编写 manifest.json
cat > ~/.config/tooldeck/tools/my-tool/manifest.json << 'EOF'
{
  "name": "my-tool",
  "displayName": "我的工具",
  "description": "这是一个示例工具",
  "category": "自定义",
  "command": "bash",
  "args": ["run.sh"]
}
EOF

# 第三步：编写可执行脚本
cat > ~/.config/tooldeck/tools/my-tool/run.sh << 'EOF'
#!/bin/bash
echo "Hello, ToolDeck!"
date
EOF
chmod +x ~/.config/tooldeck/tools/my-tool/run.sh
```

在 ToolDeck 中点击 **文件 → 刷新工具列表**，即可在侧边栏看到新工具。双击即可运行。

---

## 2. 工具结构

### 目录约定

```
~/.config/tooldeck/tools/
├── <工具名>/                 # 每个工具一个子目录
│   ├── manifest.json        # 工具描述文件（必需）
│   └── ...                  # 可执行脚本/二进制（路径在 manifest 中指定）
```

### 工具发现机制

- ToolDeck 启动时自动扫描 `~/.config/tooldeck/tools/` 下所有子目录
- 找到 `manifest.json` 后解析并加载
- 支持热重载：**文件 → 刷新工具列表** 无需重启
- 项目 `examples/` 目录也会被扫描（开发调试用）

---

## 3. manifest.json 完整参考

### 所有字段一览

```json
{
  "name":        "tool-id",          // [必需] 唯一标识，建议用英文小写+连字符
  "displayName": "显示名称",          // [必需] 界面上显示的名称
  "description": "工具功能描述",      // [可选] 鼠标悬停提示
  "version":     "1.0.0",           // [可选] 版本号，默认 "1.0.0"
  "category":    "分类名",           // [可选] 分组，默认 "custom"
  "icon":        "图标名",           // [可选] Freedesktop 图标名
  "command":     "bash",            // [必需] 可执行程序或解释器
  "args":        ["script.sh"],     // [可选] 命令行参数数组
  "workingDir":  ".",               // [可选] 工作目录，默认 "."
  "runMode":     "oneshot",         // [可选] oneshot | daemon | scheduled
  "outputMode":  "stream",          // [可选] stream | result | status
  "inputSchema": {}                 // [可选] 输入参数 JSON Schema
}
```

### 字段详解

#### `name`（必需）

工具的唯一标识符。建议规则：

| ✅ 推荐 | ❌ 避免 |
|---------|---------|
| `network-monitor` | `网络监控` |
| `disk-cleaner` | `Disk Cleaner` |
| `my-tool-v2` | `tool1` |

> `name` 也是工具目录名，确保目录名与 manifest 中的 name 一致。

#### `displayName`（必需）

界面中显示的名称。支持中文。

#### `command`（必需）

执行入口。支持三种写法：

```json
// 1. 解释器 + args（推荐，跨平台兼容）
{ "command": "bash",     "args": ["script.sh"] }
{ "command": "python3",  "args": ["main.py"] }
{ "command": "node",     "args": ["index.js"] }

// 2. 绝对路径
{ "command": "/usr/local/bin/my-binary" }

// 3. 相对路径（相对于 manifest.json 所在目录）
{ "command": "./bin/compiled-tool" }
```

#### `args`

传递给 `command` 的参数数组。支持 **占位符**（规划中）：

```json
// 未来支持（目前直接传参）：
{ "args": ["--verbose", "--output", "$OUTPUT_DIR"] }
```

#### `runMode`

| 值 | 含义 | 适用场景 |
|----|------|----------|
| `"oneshot"` | 一次性运行，结束后自动回收 | 数据查询、文件处理、编译任务 |
| `"daemon"` | 长时间运行的后台进程 | 系统监控、网络服务、文件监听 |
| `"scheduled"` | 周期性定时执行（规划中） | 定时备份、定期检查 |

默认值：`"oneshot"`

#### `outputMode`

| 值 | 含义 | 界面行为 |
|----|------|----------|
| `"stream"` | 实时流式输出 | 在输出面板中逐行显示 stdout |
| `"result"` | 仅显示最终结果 | 运行结束后一次性显示全部输出 |
| `"status"` | 仅显示状态 | 不显示输出内容，只显示成功/失败状态 |

默认值：`"stream"`

#### `category`

侧边栏分组。内置建议分类：

| 分类 | 中文名 | 适用工具 |
|------|--------|----------|
| `system` | 系统 | 系统信息、进程管理、服务控制 |
| `network` | 网络 | 网络监控、流量分析、连接测试 |
| `dev` | 开发 | 编译、测试、代码生成、Git 工具 |
| `media` | 媒体 | 图片处理、音视频转换、截图 |
| `custom` | 自定义 | 未分类的杂项工具 |

也可以自定义任意分类名。

#### `icon`

使用 [Freedesktop Icon Naming Spec](https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html) 中的图标名：

```json
// 常用图标
"utilities-terminal"    // 终端/命令行工具
"utilities-system-monitor"  // 系统监控
"network-wired"         // 网络工具
"folder"                // 文件管理
"applications-graphics" // 图形/媒体
"text-x-script"         // 脚本
"application-x-executable" // 可执行程序（默认）
```

> ToolDeck 使用 `QIcon::fromTheme()` 从系统图标主题加载。若图标不存在，优雅降级为默认图标。

---

## 4. 运行模式详解

### 4.1 Oneshot 模式（一次性任务）

```
用户点击运行 → QProcess 启动 → 实时输出 → 进程结束 → 显示退出码
```

**生命周期：**
```
[空闲] → [启动中] → [运行中] → [已完成(0)] 或 [失败(!=0)]
```

**适用工具：**
- 数据查询脚本（查 IP、查天气）
- 文件批处理（批量重命名、格式转换）
- 编译构建（make、cmake）
- 一次性检查（磁盘空间、端口占用）

### 4.2 Daemon 模式（守护进程）

```
用户点击运行 → QProcess 启动 → 持续运行 → 实时输出流 → 用户手动停止
```

**生命周期：**
```
[空闲] → [启动中] → [运行中] → [已停止]（用户操作）
                              → [失败]（异常退出）
```

**适用工具：**
- 系统资源监控（CPU、内存、网络流量）
- 文件变更监听（inotify）
- 日志实时查看（tail -f）
- 本地服务（HTTP server、WebSocket）

**停止方式：**
- 仪表盘卡片点击「停止」
- 菜单 **工具 → 停止全部工具**
- 关闭 ToolDeck 窗口（自动终止所有 daemon）

### 4.3 Scheduled 模式（定时任务）

> ⚠️ 规划中，尚未实现。

```
用户配置 cron → 平台定时触发 → 每次作为 oneshot 执行 → 记录历史
```

---

## 5. 输出模式详解

### 5.1 Stream（流式输出）

```
stdout: "Loading...\n"  → 实时追加到输出面板
stdout: "Done.\n"       → 继续追加
stderr: "Warning: ..."  → 也追加到输出面板
```

每个工具在输出面板中独占一个 Tab 页，支持：
- 实时流式显示
- 终端风格配色（深色背景）
- 多工具并发输出（各自独立 Tab）
- 超过 10000 行自动截断

### 5.2 Result（最终结果）

运行期间不显示输出，完成后一次性展示全部内容。适合输出简短的工具。

### 5.3 Status（状态指示）

完全不显示输出内容。仅在仪表盘卡片上通过颜色和状态文字反馈结果：
- 🟢 已完成 (0)
- 🔴 失败 (!=0)

适合静默运行的检查类工具。

---

## 6. 实战示例

### 示例一：一次性命令行工具 (Bash)

**场景**：查看 Git 仓库状态汇总

**目录结构：**
```
~/.config/tooldeck/tools/git-summary/
├── manifest.json
└── summary.sh
```

**manifest.json：**
```json
{
  "name": "git-summary",
  "displayName": "Git 仓库概览",
  "description": "扫描当前目录下的所有 Git 仓库，显示分支和状态",
  "version": "1.0.0",
  "category": "开发",
  "icon": "folder-git",
  "command": "bash",
  "args": ["summary.sh"],
  "runMode": "oneshot",
  "outputMode": "stream"
}
```

**summary.sh：**
```bash
#!/bin/bash
# Git 仓库概览 — 扫描子目录中的所有 Git 仓库
echo "=== Git 仓库概览 ==="
echo "扫描目录: $(pwd)"
echo ""

found=0
for dir in */; do
    if [ -d "$dir/.git" ]; then
        found=$((found + 1))
        cd "$dir" || continue
        branch=$(git branch --show-current 2>/dev/null || echo "??")
        status=$(git status --short 2>/dev/null | wc -l)
        echo "📂 $dir"
        echo "   分支: $branch"
        echo "   变更: $status 个文件"
        echo ""
        cd ..
    fi
done

if [ "$found" -eq 0 ]; then
    echo "未发现 Git 仓库"
else
    echo "共发现 $found 个 Git 仓库"
fi
```

```bash
chmod +x ~/.config/tooldeck/tools/git-summary/summary.sh
```

---

### 示例二：带参数的 Python 脚本

**场景**：HTTP 服务健康检查

**目录结构：**
```
~/.config/tooldeck/tools/health-check/
├── manifest.json
└── check.py
```

**manifest.json：**
```json
{
  "name": "health-check",
  "displayName": "服务健康检查",
  "description": "对指定的 URL 列表进行 HTTP 健康检查",
  "version": "1.0.0",
  "category": "网络",
  "icon": "network-server",
  "command": "python3",
  "args": ["check.py", "https://github.com", "https://google.com"],
  "runMode": "oneshot",
  "outputMode": "stream"
}
```

**check.py：**
```python
#!/usr/bin/env python3
"""HTTP 健康检查工具"""
import sys
import urllib.request
import urllib.error
import time

def check_url(url: str) -> dict:
    """检查单个 URL 的可用性"""
    start = time.time()
    try:
        req = urllib.request.Request(url, method='HEAD')
        resp = urllib.request.urlopen(req, timeout=10)
        elapsed = (time.time() - start) * 1000
        return {
            "url": url,
            "status": resp.status,
            "latency_ms": f"{elapsed:.0f}",
            "ok": resp.status < 400
        }
    except urllib.error.HTTPError as e:
        return {"url": url, "status": e.code, "latency_ms": "-", "ok": False}
    except Exception as e:
        return {"url": url, "status": "error", "latency_ms": "-", "ok": False, "error": str(e)}

def main():
    urls = sys.argv[1:] if len(sys.argv) > 1 else ["https://github.com"]
    
    print("=" * 60)
    print("  HTTP 服务健康检查")
    print("=" * 60)
    print()
    
    for url in urls:
        result = check_url(url)
        icon = "✅" if result["ok"] else "❌"
        print(f"{icon} {result['url']}")
        print(f"   状态: {result['status']}  |  延迟: {result['latency_ms']}ms")
        if "error" in result:
            print(f"   错误: {result['error']}")
        print()
    
    print("检查完成")

if __name__ == "__main__":
    main()
```

```bash
chmod +x ~/.config/tooldeck/tools/health-check/check.py
```

---

### 示例三：长时间运行的后台监控 (守护进程)

**场景**：实时网络流量监控

**目录结构：**
```
~/.config/tooldeck/tools/net-monitor/
├── manifest.json
└── monitor.sh
```

**manifest.json：**
```json
{
  "name": "net-monitor",
  "displayName": "网络流量监控",
  "description": "每秒输出网络接口的实时流量统计",
  "version": "1.0.0",
  "category": "网络",
  "icon": "network-wired",
  "command": "bash",
  "args": ["monitor.sh"],
  "runMode": "daemon",
  "outputMode": "stream"
}
```

**monitor.sh：**
```bash
#!/bin/bash
# 网络流量监控 — daemon 模式，持续运行直到被停止

INTERFACE="${1:-wlan0}"
INTERVAL=2

echo "🌐 网络流量监控 — $INTERFACE"
echo "刷新间隔: ${INTERVAL}s"
echo "按 Stop 按钮停止监控"
echo "---"

# 获取初始值
rx1=$(cat /sys/class/net/$INTERFACE/statistics/rx_bytes 2>/dev/null)
tx1=$(cat /sys/class/net/$INTERFACE/statistics/tx_bytes 2>/dev/null)

if [ -z "$rx1" ]; then
    echo "错误: 接口 $INTERFACE 不存在"
    exit 1
fi

# 持续循环，直到被 kill
while true; do
    sleep "$INTERVAL"

    rx2=$(cat /sys/class/net/$INTERFACE/statistics/rx_bytes)
    tx2=$(cat /sys/class/net/$INTERFACE/statistics/tx_bytes)

    rx_rate=$(( (rx2 - rx1) / INTERVAL ))
    tx_rate=$(( (tx2 - tx1) / INTERVAL ))

    # 格式化速率
    rx_display=$(numfmt --to=iec --suffix=B/s $rx_rate 2>/dev/null || echo "${rx_rate}B/s")
    tx_display=$(numfmt --to=iec --suffix=B/s $tx_rate 2>/dev/null || echo "${tx_rate}B/s")

    printf "[%s] ⬇ %-12s  ⬆ %-12s\n" "$(date +%H:%M:%S)" "$rx_display" "$tx_display"

    rx1=$rx2
    tx1=$tx2
done
```

```bash
chmod +x ~/.config/tooldeck/tools/net-monitor/monitor.sh
```

> **关键点**：`runMode: "daemon"` 让平台知道这是一个持久进程，仪表盘会显示运行时长，"停止"按钮可用。

---

### 示例四：C/C++ 编译型工具

**场景**：高性能的文件哈希计算工具

**目录结构：**
```
~/.config/tooldeck/tools/file-hash/
├── manifest.json
├── build.sh              # 编译脚本
└── bin/
    └── filehash          # 编译产物
```

**manifest.json：**
```json
{
  "name": "file-hash",
  "displayName": "文件哈希计算",
  "description": "使用 xxHash 算法快速计算文件哈希值",
  "version": "1.0.0",
  "category": "系统",
  "icon": "accessories-calculator",
  "command": "./bin/filehash",
  "args": [],
  "runMode": "oneshot",
  "outputMode": "stream"
}
```

**build.sh**（编译脚本，可选）：
```bash
#!/bin/bash
gcc -O3 -o bin/filehash filehash.c -lxxhash
echo "Build complete"
```

**filehash.c**（核心逻辑）：
```c
#include <stdio.h>
#include <stdlib.h>
#include <xxhash.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("用法: filehash <文件路径>\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        printf("无法打开文件: %s\n", argv[1]);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);

    XXH64_hash_t hash = XXH64(buf, size, 0);
    printf("文件: %s\n", argv[1]);
    printf("大小: %ld 字节\n", size);
    printf("哈希: 0x%llx\n", (unsigned long long)hash);

    free(buf);
    return 0;
}
```

> **关键点**：`command` 直接指向编译后的二进制文件，`runMode: "oneshot"` 执行完即退出。

---

### 示例五：定时周期任务

> ⚠️ Scheduled 模式规划中。当前可以用 daemon + sleep 循环模拟：

```bash
#!/bin/bash
# 磁盘空间定时检查 — 每 5 分钟报告一次
INTERVAL=300
while true; do
    echo "[$(date '+%H:%M:%S')] 磁盘空间检查"
    df -h / /home | tail -n +2
    echo "---"
    sleep "$INTERVAL"
done
```

配置 manifest：`"runMode": "daemon"`

---

### 示例六：混合语言工具 (Bash 调用 Python)

**场景**：Bash 收集环境信息 → Python 格式化输出

**manifest.json：**
```json
{
  "name": "sys-report",
  "displayName": "系统诊断报告",
  "description": "收集系统信息并生成格式化报告",
  "version": "1.0.0",
  "category": "系统",
  "icon": "utilities-system-monitor",
  "command": "bash",
  "args": ["collect.sh"],
  "runMode": "oneshot",
  "outputMode": "stream"
}
```

**collect.sh**（收集原始数据，传给 Python）：
```bash
#!/bin/bash
echo "---SYSINFO---"
echo "hostname=$(hostname)"
echo "kernel=$(uname -r)"
echo "uptime=$(uptime -p)"
echo "memory=$(free -h | grep Mem | awk '{print $3"/"$2}')"
echo "disk=$(df -h / | tail -1 | awk '{print $3"/"$2 " ("$5")"}')"
echo "---END---"
```

**format.py**（读取数据，格式化输出）：
```python
#!/usr/bin/env python3
import subprocess, sys

# 运行 collect.sh 捕获输出
result = subprocess.run(["bash", "collect.sh"], capture_output=True, text=True)

print("╔══════════════════════════════╗")
print("║     系统诊断报告             ║")
print("╚══════════════════════════════╝")
print()

for line in result.stdout.strip().split("\n"):
    if "=" in line:
        key, val = line.split("=", 1)
        print(f"  {key:12s} : {val}")

print()
print("诊断完成 ✅")
```

---

## 7. 分类与图标

### 内置分类推荐

| 分类 ID | 显示名称 | 推荐图标 |
|---------|----------|----------|
| `system` | 系统 | `utilities-system-monitor` |
| `network` | 网络 | `network-wired` |
| `dev` | 开发 | `applications-development` |
| `media` | 媒体 | `applications-graphics` |
| `data` | 数据处理 | `x-office-spreadsheet` |
| `security` | 安全 | `changes-prevent` |
| `custom` | 自定义 | `application-x-executable` |

### 常用 Freedesktop 图标名

```
# 操作类
media-playback-start     文档编辑          folder
document-open             system-run        application-exit

# 设备类
drive-harddisk            media-flash       network-wired
printer                   camera-photo      input-keyboard

# 状态类
dialog-information        dialog-warning    dialog-error
emblem-ok                 emblem-important  face-smile

# 开发类
applications-development  folder-git        text-x-script
utilities-terminal        accessories-text-editor
```

> 完整列表：`/usr/share/icons/` 或运行 `qt6-image-formats` 查看可用图标。

---

## 8. 调试技巧

### 8.1 查看 manifest 是否被加载

```bash
# 确认目录结构无误
ls -la ~/.config/tooldeck/tools/<工具名>/

# 验证 manifest JSON 语法
python3 -m json.tool ~/.config/tooldeck/tools/<工具名>/manifest.json
```

在 ToolDeck 中点击 **文件 → 刷新工具列表**，侧边栏应出现你的工具。

### 8.2 模拟平台执行

```bash
# 进入工具目录，手动执行命令
cd ~/.config/tooldeck/tools/<工具名>/
bash script.sh        # 对 bash 工具
python3 main.py       # 对 python 工具
./bin/my-tool         # 对二进制工具
```

如果手动运行正常但平台中异常，检查：
- 脚本是否有执行权限（`chmod +x`）
- manifest 中 `command` 和 `args` 是否正确
- `workingDir` 是否正确解析

### 8.3 常见问题

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| 侧边栏看不到工具 | manifest 无效或位置不对 | 检查目录是否在 `~/.config/tooldeck/tools/` 下，JSON 是否合规 |
| 点击运行无反应 | `command` 路径错误 | 用绝对路径测试，或确保相对路径在 manifest 所在目录下 |
| 输出面板无输出 | `outputMode` 设为 status | 改为 `"stream"` 查看输出 |
| daemon 工具异常退出 | 脚本未进入死循环 | daemon 需要持续运行的逻辑（如 `while true`） |
| 中文乱码 | 文件编码问题 | 确保脚本文件为 UTF-8 编码 |

### 8.4 查看运行时日志

ToolDeck 内部日志使用 `qDebug()`，启动时可设置环境变量查看：

```bash
QT_LOGGING_RULES="*.debug=true" ./build/tooldeck 2>&1 | grep -i tool
```

---

## 9. 最佳实践

### 9.1 脚本规范

```bash
#!/bin/bash
# ------------------------------------------------------------------
# 工具名: 简短描述
# 用法:   说明参数
# 返回:   0=成功  非0=失败
# ------------------------------------------------------------------
set -euo pipefail    # 严格模式：遇错即停

# ... 工具逻辑 ...

exit 0               # 明确的退出码
```

### 9.2 输出规范

- 使用清晰的标题分隔（`===`）
- 关键信息前置（状态、数值）
- 错误信息输出到 stderr（`>&2 echo "错误: ..."`）
- 避免大量无用输出（daemon 模式尤其注意频率）

### 9.3 命名规范

| 规范 | 说明 |
|------|------|
| `name` | 英文小写 + 连字符：`disk-cleaner`、`net-monitor` |
| `displayName` | 简洁中文：`磁盘清理`、`网络监控` |
| `category` | 使用建议分类名或自定义中文分类 |
| 目录名 | 与 `name` 保持一致 |

### 9.4 安全性

- **敏感操作**（删除、格式化、系统配置修改）加确认逻辑
- **网络工具**注意超时设置，避免无限等待
- **daemon 工具**控制输出频率，避免日志爆炸
- 对外部输入做校验（文件路径、URL 等）

### 9.5 版本管理

建议在 manifest 中维护版本号，方便追踪变更：

```json
{
  "version": "1.0.0",
  ...
}
```

工具目录自身可以用 Git 管理：

```bash
cd ~/.config/tooldeck/tools/
git init
git add -A
git commit -m "添加网络监控工具"
```

---

## 附录：最小 manifest 模板

```json
{
  "name": "my-tool",
  "displayName": "我的工具",
  "command": "bash",
  "args": ["run.sh"]
}
```

只有 `name`、`displayName`、`command` 是必需的。其余字段都有合理的默认值。

---

> 📚 相关文档：架构设计见 `docs/architecture.md`（待生成）
>
> 🤖 Generated with [Claude Code](https://claude.com/claude-code)
