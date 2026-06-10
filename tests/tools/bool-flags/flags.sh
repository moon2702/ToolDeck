#!/bin/bash
# bool-flags — 展示三种 Bool 修饰符的效果 (覆盖 +flag, --flag, -flag)
echo "══════ 收到的参数 ══════"
echo "参数个数: $#"
echo ""
for i in "$@"; do
    echo "  arg: [$i]"
done
echo ""
echo "预期: verbose=true→'--verbose', debug=true→'+d', quiet=true→'-q'"
echo "      verbose=false→(空), debug=false→(空), quiet=false→(空)"
