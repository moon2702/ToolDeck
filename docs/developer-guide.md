# ToolDeck 开发指南 — manifest.json 配置手册

> 本文档聚焦 **manifest.json 的编写**。工具脚本的编写规范请参考项目 `examples/` 中的实际工具。

---

## 目录

1. [最小可用的 manifest](#1-最小可用的-manifest)
2. [字段速查表](#2-字段速查表)
3. [必填字段](#3-必填字段)
4. [运行与输出控制](#4-运行与输出控制)
5. [参数输入: inputs + argTemplate](#5-参数输入-inputs--argtemplate)
6. [参数输入: 弹窗行为规则](#6-参数输入-弹窗行为规则)
7. [argTemplate 语法参考](#7-argtemplate-语法参考)
8. [完整配置示例](#8-完整配置示例)
9. [分类与图标速查](#9-分类与图标速查)
10. [工具发现路径](#10-工具发现路径)
11. [故障排查](#11-故障排查)

---

## 1. 最小可用的 manifest

只需三个字段即可让工具出现在侧边栏并运行：

```json
{
  "name": "my-tool",
  "displayName": "我的工具",
  "command": "bash",
  "args": ["run.sh"]
}
```

**目录结构**：
```
~/.config/tooldeck/tools/my-tool/
├── manifest.json     ← 这个文件
└── run.sh            ← 可执行脚本 (chmod +x)
```

双击即可运行，不弹窗、不传参。

---

## 2. 字段速查表

| 字段 | 类型 | 必需 | 默认值 | 说明 |
|------|:--:|:--:|--------|------|
| `name` | string | ✅ | — | 唯一 ID，英文小写+连字符 |
| `displayName` | string | ✅ | — | 界面显示名，支持中文 |
| `command` | string | ✅ | — | 可执行程序或解释器路径 |
| `args` | string[] | — | `[]` | 固定命令行参数 |
| `description` | string | — | `""` | 鼠标悬停提示 |
| `version` | string | — | `"1.0.0"` | 语义版本号 |
| `category` | string | — | `"custom"` | 侧边栏分组名 |
| `icon` | string | — | `"application-x-executable"` | Freedesktop 图标名 |
| `workingDir` | string | — | `"."` | 工作目录（相对=相对于 manifest 目录） |
| `runMode` | string | — | `"oneshot"` | `oneshot` / `daemon` |
| `outputMode` | string | — | `"stream"` | `stream` / `result` / `status` |
| `inputs` | object[] | — | `[]` | 参数输入定义（见第 5 节） |
| `argTemplate` | string | — | `""` | 参数拼接模板（见第 7 节） |
| `os` | string[] | — | `[]` | 兼容操作系统列表，不填=全平台（`windows`/`linux`/`macos`） |

---

## 3. 必填字段

### `name` — 工具唯一标识

```
规则: 英文小写 + 连字符, 与目录名一致
✅ echo-demo  hello-world  file-hash  port-check
❌ 网络监控    HelloWorld   tool 1
```

### `displayName` — 界面显示名

```
侧边栏、仪表盘卡片、输出 Tab 标签均使用此名称。
支持中文、emoji、空格。
```

### `command` — 执行入口

三种写法，按需选择：

```json
// 解释器模式（推荐）—— 脚本跨平台只需换解释器
{ "command": "bash",     "args": ["script.sh"] }
{ "command": "python3",  "args": ["main.py"] }
{ "command": "node",     "args": ["index.js"] }

// 绝对路径 —— 适用于系统安装的工具
{ "command": "/usr/local/bin/my-tool" }

// 相对路径 —— 相对于 manifest.json 所在目录
{ "command": "./bin/compiled-tool" }
```

`args` 中的固定参数**始终拼接**到命令行。运行时通过 `inputs` 传入的参数通过 `argTemplate` 拼接（见第 5 节），最终命令行 = `args + argTemplate展开结果`。

### `os` — 兼容操作系统

```
可选字段。声明工具兼容的操作系统列表。
不填或空数组 = 全平台兼容。
```

```json
// 仅 Windows（如使用 PowerShell 的工具）
{ "os": ["windows"] }

// 仅 Linux（如依赖 /proc 的工具）
{ "os": ["linux"] }

// 多平台
{ "os": ["windows", "linux"] }

// 全平台（默认）
{ "os": [] }
```

| 合法值 | 对应平台 |
|--------|---------|
| `"windows"` | Microsoft Windows |
| `"linux"` | Linux 发行版 |
| `"macos"` | Apple macOS |

ToolDeck 在启动和刷新时通过 `QSysInfo::kernelType()` 获取当前系统标识（`linux`/`winnt`/`darwin`），仅加载 `os` 列表包含当前系统标识（或 `os` 为空）的工具。

---

## 4. 运行与输出控制

### `runMode` — 运行模式

| 值 | 生命周期 | 何时用 |
|----|----------|--------|
| `"oneshot"` | 启动 → 运行 → 结束 → 自动回收 | 数据查询、文件处理、编译构建 |
| `"daemon"` | 启动 → 持续运行 → 用户手动停止 | 系统监控、文件监听、本地服务 |

daemon 工具需脚本内有死循环（`while true`）。停止方式：仪表盘「停止」按钮、**工具 → 停止全部工具**、关闭 ToolDeck。

### `outputMode` — 输出显示

| 值 | 行为 |
|----|------|
| `"stream"` | stdout 实时追加到输出面板（默认） |
| `"result"` | 进程结束才显示全部输出 |
| `"status"` | 不显示输出，仅仪表盘显示成功/失败 |

---

## 5. 参数输入: inputs + argTemplate

这是配置文件的**核心章节**。定义了用户如何给工具传参，以及 ToolDeck 如何生成命令行。

### 5.1 工作流

```
点击「运行」
  → ToolDeck 读取 inputs[]
  → 若有 required 字段 → 弹出表单
  → 用户填写
  → 表单值通过 argTemplate 展开
  → 最终命令行 = args + 展开结果
  → QProcess 执行
```

### 5.2 inputs 字段定义

每个 input 是一个 JSON 对象：

```json
{
  "name":        "filepath",      // 变量名，argTemplate 中用 {filepath} 引用
  "type":        "file",          // 控件类型（见下表）
  "label":       "文件路径",       // 表单中的标签
  "description": "选择要处理的文件", // 提示文字/placeholder
  "required":    true,             // 是否必填
  "default":     ""                // 默认值
}
```

按 type 不同，还有额外字段：

```json
// type = "choice" 需要 choices 数组
{ "type": "choice", "choices": ["xxh64", "xxh32", "xxh128"], "default": "xxh64" }

// type = "int" 可选 min / max
{ "type": "int", "min": 1, "max": 65535, "default": "443" }

// type = "number" 可选 min / max / decimals
{ "type": "number", "min": 0.0, "max": 100.0, "decimals": 2 }
```

### 5.3 type 与 Qt 控件的对应

| `type` | 生成的表单控件 | 示例效果 |
|:------:|---------------|----------|
| `text` | `QLineEdit` | `[________________]` |
| `file` | `QLineEdit` + **「浏览」按钮** | `[/path/to/file] [浏览]` |
| `dir` | `QLineEdit` + **「浏览」按钮** | `[/path/to/dir ] [浏览]` |
| `choice` | `QComboBox`（下拉选择） | `[xxh64      ▾]` |
| `bool` | `QCheckBox`（复选框） | `☑ 详细模式` |
| `int` | `QSpinBox`（整数微调） | `[  443  🔼🔽]` |
| `number` | `QDoubleSpinBox`（浮点微调） | `[ 0.00  🔼🔽]` |

### 5.4 对比模式

在**对比模式**（菜单 模式 → 对比模式）下，ToolDeck 支持同一工具使用两组不同参数分别运行，并在输出面板中提供三种视图对比差异：

#### 5.4.1 双栏参数表单

点击带有 inputs 的工具时，弹出**双栏** `CompareInputDialog`。左右各一组相同的控件（参数组 A / 参数组 B），一次运行生成两组参数、启动两个独立进程。

普通模式下仍然弹出单栏 `InputDialog`。

#### 5.4.2 三种差异视图

每次对比运行创建一个独立标签页，标签页顶部工具栏提供三种视图切换：

| 按钮 | 视图 | 说明 |
|------|------|------|
| **▦ 原始** | 左右分屏原始输出 | A/B 两个进程的实时输出，分栏对照 |
| **≔ 统一差异** | 单列 unified diff | `+`/`-` 前缀标识增删行，行内字符级变更用深红/深绿底高亮 |
| **◫ 并排差异** | 双列 side-by-side | 左右对齐排列，相似行智能对齐（基于 LCS 相似度贪心匹配），字符级差异内嵌高亮 |

#### 5.4.3 Diff 引擎

对比模式的差异计算由内置 `DiffEngine` 提供：

- **行级 diff**：基于 LCS（最长公共子序列）算法，识别新增/删除/相同行
- **字符级 diff**：对配对的相邻删行/增行运行二级字符 LCS，精确标注行内变化位置
- **相似度匹配**（并排视图）：对相邻删行和增行按字符 LCS 相似度贪心配对（阈值 0.35），相似行对齐到同一行，差异字符行内高亮

Diff 引擎的可配置参数（不可通过 manifest 配置）：
- 上下文行数：3（变化块周围保留 3 行相同行）
- 相似度阈值：0.35（字符级 LCS 长度 / max 长度）

---

## 6. 参数输入: 弹窗行为规则

ToolDeck 根据 `inputs` 的 `required` 字段决定是否弹窗：

| 条件 | 双击行为 |
|------|----------|
| `inputs` 为空或不存在 | **直接运行**，不传额外参数 |
| `inputs` 全都有 `default` 值，且无 `required` | **直接运行**，使用默认值 |
| 存在 `"required": true` 的字段 | **弹出表单**，用户必须填写 |

**示例**：

```json
// 不弹窗 — 直接运行
"inputs": []

// 不弹窗 — 全部可选，用默认值运行
"inputs": [
  { "name": "detail", "type": "choice", "choices": ["简要","标准","详细"], "default": "标准" }
]

// 弹窗 — filepath 是必填的
"inputs": [
  { "name": "filepath", "type": "file", "label": "文件路径", "required": true },
  { "name": "verbose",  "type": "bool", "label": "详细模式" }
]
```

---

## 7. argTemplate 语法参考

### 7.1 基本语法

`argTemplate` 是带占位符的字符串，占位符 `{name}` 对应 `inputs` 中的 `name`。

```
输入: argTemplate: "{host} {port} {timeout}"
表单: host=github.com  port=443  timeout=3
结果: github.com 443 3
```

### 7.2 修饰符语法

`{name:修饰符}` — 根据 bool 值决定是否输出修饰符。

| 语法 | true 时输出 | false 时输出 |
|------|------------|-------------|
| `{verbose:-v}` | `-v` | (空) |
| `{verbose:--verbose}` | `--verbose` | (空) |
| `{debug:+d}` | `+d` | (空) |

**规则**：修饰符以 `-` / `--` 开头 = 仅 bool 为 true 时输出修饰符本身。修饰符以 `+` 开头 = 仅 bool 为 true 时输出修饰符。

```json
// 完整示例
"argTemplate": "-a {algorithm} {verbose:-v} {filepath}"

// algorithm=xxh64  verbose=false  filepath=/tmp/a.iso
// → "-a xxh64 /tmp/a.iso"

// algorithm=xxh128  verbose=true  filepath=/tmp/b.iso
// → "-a xxh128 -v /tmp/b.iso"
```

### 7.3 无 argTemplate 时的行为

若 `argTemplate` 为空，ToolDeck 将表单值按 `inputs` 定义顺序直接追加到命令行：

```
manifest.args = ["./bin/filehash"]
inputs = [filepath, algorithm, verbose]
表单: filepath=/tmp/a.iso  algorithm=xxh64  verbose=true

→ 最终命令: ./bin/filehash /tmp/a.iso xxh64 true
```

---

## 8. 完整配置示例

以下示例均来自项目 `examples/` 目录。

### 8.1 无参数工具 (hello-world)

```json
{
  "name": "hello-world",
  "displayName": "你好世界",
  "description": "一个最小示例工具，演示 ToolDeck 平台的基本工具结构",
  "version": "1.1.0",
  "category": "开发",
  "icon": "face-smile",
  "command": "bash",
  "args": ["hello.sh"],
  "runMode": "oneshot",
  "outputMode": "stream",
  "inputs": [
    { "name": "who",    "type": "text",   "label": "问候对象", "default": "" },
    { "name": "format", "type": "choice", "label": "输出格式", "choices": ["简洁","标准","完整"], "default": "标准" }
  ],
  "argTemplate": "{who} {format}"
}
```

双击行为：直接运行（全部可选）。传参：bash hello.sh "" "标准"

### 8.2 必填文件路径 (file-hash)

```json
{
  "name": "file-hash",
  "displayName": "文件哈希计算",
  "description": "使用 xxHash 算法快速计算文件哈希值，支持多种算法切换",
  "version": "1.0.0",
  "category": "系统",
  "icon": "accessories-calculator",
  "command": "./bin/filehash",
  "args": [],
  "runMode": "oneshot",
  "outputMode": "stream",
  "inputs": [
    { "name": "filepath",  "type": "file",   "label": "文件路径", "required": true },
    { "name": "algorithm", "type": "choice", "label": "哈希算法", "choices": ["xxh64","xxh32","xxh128"], "default": "xxh64" },
    { "name": "verbose",   "type": "bool",   "label": "详细模式", "description": "显示性能统计信息" }
  ],
  "argTemplate": "-a {algorithm} {verbose:-v} {filepath}"
}
```

双击行为：弹出文件选择表单（filepath 必填）。

### 8.3 必填主机+端口 (port-check)

```json
{
  "name": "port-check",
  "displayName": "端口检测",
  "description": "检测目标主机的指定端口是否开放",
  "version": "1.0.0",
  "category": "网络",
  "icon": "network-server",
  "command": "bash",
  "args": ["check.sh"],
  "runMode": "oneshot",
  "outputMode": "stream",
  "inputs": [
    { "name": "host",    "type": "text", "label": "目标主机", "required": true },
    { "name": "port",    "type": "int",  "label": "端口号",   "required": true, "min": 1, "max": 65535, "default": "443" },
    { "name": "timeout", "type": "int",  "label": "超时(秒)", "min": 1, "max": 30, "default": "3" }
  ],
  "argTemplate": "{host} {port} {timeout}"
}
```

双击行为：弹出表单（host、port 均必填），host 是文本框，port/timeout 是数字微调控件。

### 8.4 可选参数 (default-echo)

```json
{
  "name": "echo-demo",
  "displayName": "系统信息",
  "description": "输出系统信息概览",
  "version": "1.1.0",
  "category": "系统",
  "icon": "utilities-system-monitor",
  "command": "bash",
  "args": ["echo.sh"],
  "runMode": "oneshot",
  "outputMode": "stream",
  "inputs": [
    { "name": "detail", "type": "choice", "label": "信息详细度", "choices": ["简要","标准","详细"], "default": "标准" }
  ],
  "argTemplate": "{detail}"
}
```

双击行为：直接运行（仅一个可选字段）。传参：bash echo.sh "标准"

### 8.5 Windows 限定工具 (win-sysinfo)

```json
{
  "name": "win-sysinfo",
  "displayName": "Windows 系统诊断",
  "description": "收集 Windows 系统信息：OS 版本、CPU、内存、磁盘、网络配置",
  "version": "1.0.0",
  "category": "系统",
  "icon": "utilities-system-monitor",
  "os": ["windows"],
  "command": "powershell",
  "args": ["-ExecutionPolicy", "Bypass", "-File", "sysinfo.ps1"],
  "runMode": "oneshot",
  "outputMode": "stream",
  "inputs": [
    { "name": "section", "type": "choice", "label": "信息类别",
      "choices": ["完整","系统概览","CPU","内存","磁盘","网络"], "default": "完整" }
  ],
  "argTemplate": "{section}"
}
```

双击行为：直接运行（分为完整/概览/CPU/内存/磁盘/网络可选）。仅在 Windows 上显示。

---

## 9. 分类与图标速查

### 内置分类

| `category` | 侧边栏显示 | 适用 |
|------------|-----------|------|
| `system` | 系统 | 系统信息、进程管理、磁盘工具 |
| `network` | 网络 | 网络检测、流量监控、DNS 查询 |
| `dev` | 开发 | 编译、测试、Git、代码生成 |
| `media` | 媒体 | 图片处理、音视频转换 |
| `data` | 数据处理 | 格式转换、统计分析 |
| `custom` | 自定义 | 未分类（默认值） |

分类名支持中文，写入 `category` 字段即可。

### 常用图标

```
utilities-terminal         命令行工具
utilities-system-monitor   系统监控
network-wired              网络
network-server             服务器/端口
folder-git                 Git 仓库
accessories-calculator     计算/哈希
face-smile                 通用/示例
document-edit              文档编辑
folder                     文件夹
media-flash                存储/闪存
```

> ToolDeck 使用 `QIcon::fromTheme()` 从系统主题加载图标，不存在时降级为默认图标。

---

## 10. 工具发现路径

ToolDeck 按以下优先级扫描工具：

| 路径 | 用途 |
|------|------|
| `~/.config/tooldeck/tools/` | 用户安装的工具（持久存放） |
| `<二进制目录>/../examples/` | 项目内置示例（开发调试） |

**目录约定**：

```
~/.config/tooldeck/tools/
├── my-tool-1/            ← 目录名 = manifest 中的 name
│   ├── manifest.json     ← 必需
│   └── ...               ← 脚本/二进制等
├── my-tool-2/
│   ├── manifest.json
│   └── ...
```

刷新：**文件 → 刷新工具列表**（无需重启）。

---

## 11. 故障排查

| 现象 | 检查 |
|------|------|
| 侧边栏没有我的工具 | `manifest.json` 是否在 `~/.config/tooldeck/tools/<name>/` 下？JSON 语法是否合法？`os` 字段是否包含当前系统？ |
| 点击运行无反应 | `command` 路径是否正确？脚本是否有执行权限？ |
| 弹窗不出现 | 检查 `inputs` 中是否有 `"required": true` 的字段 |
| 参数没传进去 | 检查 `argTemplate` 占位符 `{name}` 是否与 `inputs[].name` 一致 |
| 输出乱码 | 脚本文件编码应为 UTF-8 |
| daemon 立即退出 | 脚本需要死循环（`while true`），否则执行完就退出 |

### 验证 manifest JSON

```bash
python3 -m json.tool ~/.config/tooldeck/tools/<工具名>/manifest.json > /dev/null \
  && echo "JSON 合法" || echo "JSON 错误"
```

### 手动模拟执行

```bash
cd ~/.config/tooldeck/tools/<工具名>/
bash script.sh arg1 arg2           # 模拟 ToolDeck 执行
```

---

> 📚 完整参考实现：`examples/` 目录下的 `hello-world`, `default-echo`, `file-hash`, `port-check`, `win-sysinfo`
