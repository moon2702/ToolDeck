#!/bin/bash
# clock-watch — daemon 模式系统监控 (覆盖 runMode=daemon)
echo "══════ 系统监控 (daemon) ══════"
echo "每 2 秒输出一次时间戳和负载"
echo "按侧边栏「停止」按钮或工具→停止全部工具来停止"
echo ""

count=0
while true; do
    count=$((count + 1))
    now=$(date '+%Y-%m-%d %H:%M:%S')
    load=$(uptime 2>/dev/null | awk -F'load average:' '{print $2}' || echo "N/A")
    echo "[#$count] $now | 负载:$load"
    sleep 2
done
