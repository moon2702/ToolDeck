<#
.SYNOPSIS
    Windows 系统诊断工具 — ToolDeck 示例工具
.DESCRIPTION
    收集 Windows 系统信息：OS 版本、CPU、内存、磁盘、网络配置。
.PARAMETER Section
    信息类别: 完整 | 系统概览 | CPU | 内存 | 磁盘 | 网络
#>
param(
    [ValidateSet("完整", "系统概览", "CPU", "内存", "磁盘", "网络")]
    [string]$Section = "完整"
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [Text.Encoding]::UTF8

# ── 工具函数 ──────────────────────────────────────────
function Write-Banner {
    Write-Host "╔══════════════════════════════════════════╗"
    Write-Host "║     Windows 系统诊断工具                  ║"
    Write-Host "╚══════════════════════════════════════════╝"
    Write-Host ""
    Write-Host "  时间 : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
    Write-Host "  主机 : $env:COMPUTERNAME"
    Write-Host "  用户 : $env:USERNAME"
    Write-Host ""
}

function Write-SectionHeader {
    param([string]$Title)
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    Write-Host "  $Title"
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
}

# ── 各模块 ────────────────────────────────────────────
function Show-SystemOverview {
    Write-SectionHeader "系统概览"

    $os = Get-CimInstance -ClassName Win32_OperatingSystem
    $cs = Get-CimInstance -ClassName Win32_ComputerSystem

    $installDate = $os.InstallDate
    if ($installDate) {
        $uptime = (Get-Date) - $os.LastBootUpTime
        $uptimeStr = "$($uptime.Days)天 $($uptime.Hours)小时 $($uptime.Minutes)分钟"
    } else {
        $uptimeStr = "N/A"
    }

    Write-Host "  OS 名称   : $($os.Caption)"
    Write-Host "  版本       : $($os.Version)"
    Write-Host "  构建号     : $($os.BuildNumber)"
    Write-Host "  架构       : $($os.OSArchitecture)"
    Write-Host "  安装日期   : $(if($installDate){$installDate.ToString('yyyy-MM-dd')}else{'N/A'})"
    Write-Host "  运行时间   : $uptimeStr"
    Write-Host "  制造商     : $($cs.Manufacturer)"
    Write-Host "  型号       : $($cs.Model)"
    Write-Host "  物理内存   : $([math]::Round($cs.TotalPhysicalMemory/1GB, 1)) GB"
    Write-Host ""
}

function Show-CPU {
    Write-SectionHeader "CPU 信息"

    $cpus = Get-CimInstance -ClassName Win32_Processor

    foreach ($cpu in $cpus) {
        Write-Host "  名称       : $($cpu.Name.Trim())"
        Write-Host "  核心数     : $($cpu.NumberOfCores)"
        Write-Host "  逻辑线程   : $($cpu.NumberOfLogicalProcessors)"
        Write-Host "  最大频率   : $($cpu.MaxClockSpeed) MHz"
        Write-Host "  插槽       : $($cpu.SocketDesignation)"
        Write-Host "  L2 缓存    : $([math]::Round($cpu.L2CacheSize/1024, 1)) MB"
        Write-Host "  L3 缓存    : $([math]::Round($cpu.L3CacheSize/1024, 1)) MB"
        Write-Host ""
    }

    # 简单负载
    $load = Get-CimInstance -ClassName Win32_Processor |
        Measure-Object -Property LoadPercentage -Average
    Write-Host "  当前负载   : $([math]::Round($load.Average, 1))%"
    Write-Host ""
}

function Show-Memory {
    Write-SectionHeader "内存信息"

    $os = Get-CimInstance -ClassName Win32_OperatingSystem
    $totalGB = [math]::Round($os.TotalVisibleMemorySize / 1MB, 1)
    $freeGB  = [math]::Round($os.FreePhysicalMemory / 1MB, 1)
    $usedGB  = [math]::Round($totalGB - $freeGB, 1)
    $pct     = [math]::Round(($usedGB / $totalGB) * 100, 1)

    Write-Host "  总内存     : ${totalGB} GB"
    Write-Host "  已用       : ${usedGB} GB"
    Write-Host "  可用       : ${freeGB} GB"
    Write-Host "  使用率     : ${pct}%"
    Write-Host ""

    # 虚拟内存
    $totalVM = [math]::Round($os.TotalVirtualMemorySize / 1MB, 1)
    $freeVM  = [math]::Round($os.FreeVirtualMemory / 1MB, 1)
    Write-Host "  虚拟内存总量 : ${totalVM} GB"
    Write-Host "  虚拟内存可用 : ${freeVM} GB"
    Write-Host ""

    # 耗时最长的前 5 个进程 (按工作集)
    Write-Host "  内存占用 Top 5 进程:"
    Get-Process | Sort-Object WorkingSet64 -Descending |
        Select-Object -First 5 |
        ForEach-Object {
            $ws = [math]::Round($_.WorkingSet64 / 1MB, 1)
            Write-Host "    $($_.ProcessName.PadRight(25)) ${ws} MB"
        }
    Write-Host ""
}

function Show-Disk {
    Write-SectionHeader "磁盘信息"

    Get-CimInstance -ClassName Win32_LogicalDisk -Filter "DriveType=3" |
        ForEach-Object {
            $total = [math]::Round($_.Size / 1GB, 1)
            $free  = [math]::Round($_.FreeSpace / 1GB, 1)
            $used  = [math]::Round($total - $free, 1)
            $pct   = if ($total -gt 0) { [math]::Round(($used / $total) * 100, 1) } else { 0 }

            Write-Host "  盘符       : $($_.DeviceID)"
            Write-Host "  文件系统   : $($_.FileSystem)"
            Write-Host "  总空间     : ${total} GB"
            Write-Host "  已用       : ${used} GB"
            Write-Host "  可用       : ${free} GB"
            Write-Host "  使用率     : ${pct}%"
            Write-Host "  ─────────────────────────────"
        }

    Write-Host ""
}

function Show-Network {
    Write-SectionHeader "网络配置"

    $adapters = Get-NetAdapter -Physical |
        Where-Object { $_.Status -eq 'Up' }

    foreach ($adapter in $adapters) {
        Write-Host "  适配器     : $($adapter.Name)"
        Write-Host "  MAC        : $($adapter.MacAddress)"
        Write-Host "  速率       : $($adapter.LinkSpeed)"
        Write-Host "  状态       : $($adapter.Status)"

        # IP 地址
        $ipInfo = Get-NetIPAddress -InterfaceIndex $adapter.InterfaceIndex -AddressFamily IPv4 |
            Select-Object -First 1
        if ($ipInfo) {
            Write-Host "  IPv4       : $($ipInfo.IPAddress)"
            $prefix = $ipInfo.PrefixLength

            # 网关
            $gw = Get-NetRoute -InterfaceIndex $adapter.InterfaceIndex -DestinationPrefix "0.0.0.0/0" |
                Select-Object -First 1
            if ($gw) {
                Write-Host "  网关       : $($gw.NextHop)"
            }

            # DNS
            $dns = Get-DnsClientServerAddress -InterfaceIndex $adapter.InterfaceIndex -AddressFamily IPv4 |
                Select-Object -First 1
            if ($dns -and $dns.ServerAddresses) {
                Write-Host "  DNS        : $($dns.ServerAddresses -join ', ')"
            }
        }

        Write-Host "  ─────────────────────────────"
    }

    Write-Host ""
}

# ── 主流程 ────────────────────────────────────────────
Write-Banner

switch ($Section) {
    "完整"     { Show-SystemOverview; Show-CPU; Show-Memory; Show-Disk; Show-Network }
    "系统概览" { Show-SystemOverview }
    "CPU"      { Show-CPU }
    "内存"     { Show-Memory }
    "磁盘"     { Show-Disk }
    "网络"     { Show-Network }
}

Write-Host "✅ Windows 系统诊断完成"
