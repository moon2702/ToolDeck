#!/bin/bash
# anywhere — 全平台通用工具 (覆盖 os 为空/省略)
echo "══════ 系统信息 ══════"
echo "平台: $(uname -s 2>/dev/null || echo 'Windows')"
echo "架构: $(uname -m 2>/dev/null || echo 'x86_64')"
echo "主机名: $(hostname 2>/dev/null || echo 'N/A')"
echo "用户: $(whoami 2>/dev/null || echo "$USERNAME")"
echo ""
echo "(该工具未设置 os 字段，在所有平台上可见)"
