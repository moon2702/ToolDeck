#!/bin/bash
# fmt-json — 输出格式化 JSON (覆盖 outputMode=result)
# ToolDeck 在进程结束后一次性显示全部输出，模拟 result 模式
TEXT="$1"
INDENT="$2"

echo "══════ JSON 格式化 ══════"
echo "输入文本: $TEXT"

if [ "$INDENT" = "缩进" ]; then
    python3 -c "
import json, sys
data = {'text': '$TEXT', 'length': len('$TEXT'), 'indent': 2}
print(json.dumps(data, indent=2, ensure_ascii=False))
"
else
    python3 -c "
import json, sys
data = {'text': '$TEXT', 'length': len('$TEXT'), 'compact': True}
print(json.dumps(data, ensure_ascii=False))
"
fi

echo ""
echo "(此工具设 outputMode=result，ToolDeck 会在进程结束后一次性展示输出)"
