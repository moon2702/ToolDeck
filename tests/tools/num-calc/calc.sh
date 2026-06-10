#!/bin/bash
# num-calc — 浮点/整数计算器 (覆盖 Number + Int + Choice)
set -e

LEFT="$1"
OP="$2"
RIGHT="$3"

echo "══════ 计算器 ══════"
echo "左操作数: $LEFT"
echo "运算符:   $OP"
echo "右操作数: $RIGHT"

case "$OP" in
    加)  result=$(python3 -c "print($LEFT + $RIGHT)") ;;
    减)  result=$(python3 -c "print($LEFT - $RIGHT)") ;;
    乘)  result=$(python3 -c "print($LEFT * $RIGHT)") ;;
    除)
        if python3 -c "exit(0 if $RIGHT == 0 else 1)" 2>/dev/null; then
            echo "错误: 除数为零"
            exit 1
        fi
        result=$(python3 -c "print(round($LEFT / $RIGHT, 6))")
        ;;
    *) echo "错误: 未知运算符"; exit 1 ;;
esac

echo "结果:     $result"
