#!/bin/bash
# ------------------------------------------------------------------
# 端口检测工具 — 检测目标主机的指定 TCP 端口是否开放
# 用法: check.sh <主机> <端口> [超时秒数]
# 返回: 0=端口开放  1=端口关闭  2=参数错误
# ------------------------------------------------------------------
set -euo pipefail

HOST="${1:-}"
PORT="${2:-}"
TIMEOUT="${3:-3}"

if [ -z "$HOST" ] || [ -z "$PORT" ]; then
    echo "用法: check.sh <主机> <端口> [超时秒数]"
    exit 2
fi

# 验证端口范围
if [ "$PORT" -lt 1 ] || [ "$PORT" -gt 65535 ]; then
    echo "错误: 端口号必须在 1–65535 范围内"
    exit 2
fi

START_TIME=$(date +%s.%N)

echo "╔══════════════════════════════════════╗"
echo "║        TCP 端口检测工具               ║"
echo "╚══════════════════════════════════════╝"
echo ""
echo "  目标主机 : $HOST"
echo "  检测端口 : $PORT"
echo "  超时设置 : ${TIMEOUT}s"
echo ""

# 尝试连接
echo "  正在连接 $HOST:$PORT ..."

# 使用 timeout + bash /dev/tcp (不依赖外部工具)
if command -v timeout &>/dev/null; then
    if timeout "$TIMEOUT" bash -c "exec 3<>/dev/tcp/$HOST/$PORT && exec 3>&-" 2>/dev/null; then
        STATUS="开放 ✅"
        RET=0
    else
        STATUS="关闭 ❌"
        RET=1
    fi
else
    # 降级：使用 nc
    if command -v nc &>/dev/null; then
        if nc -z -w "$TIMEOUT" "$HOST" "$PORT" 2>/dev/null; then
            STATUS="开放 ✅"
            RET=0
        else
            STATUS="关闭 ❌"
            RET=1
        fi
    else
        # 最后降级：纯 bash /dev/tcp
        if bash -c "exec 3<>/dev/tcp/$HOST/$PORT 2>/dev/null && exec 3>&-" 2>/dev/null; then
            STATUS="开放 ✅"
            RET=0
        else
            STATUS="关闭 ❌"
            RET=1
        fi
    fi
fi

END_TIME=$(date +%s.%N)
ELAPSED=$(echo "$END_TIME - $START_TIME" | bc 2>/dev/null || echo "?")

echo ""
echo "  ═══ 检测结果 ═══"
echo "  端口状态 : $STATUS"
echo "  耗时     : ${ELAPSED}s"
echo ""

if [ "$RET" -eq 0 ]; then
    echo "  ✅ 端口 $PORT 已开放，服务可达"
else
    echo "  ❌ 端口 $PORT 不可达（防火墙拦截或服务未启动）"
fi

echo ""
exit $RET
