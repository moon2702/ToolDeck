#!/bin/bash
# no-tmpl — 无 argTemplate，参数直接追加 (覆盖 argTemplate 为空分支)
echo "══════ 收到的参数 (无 argTemplate 模式) ══════"
echo "参数个数: $#"
echo ""
for i in "$@"; do
    echo "  arg: [$i]"
done
echo ""
echo "ToolDeck 在无 argTemplate 时，按 inputs 定义顺序直接追加参数。"
