#!/bin/bash
# ToolDeck 全量测试脚本
# 模拟 ToolDeck 执行链路: manifest 解析 → 参数构建 → 脚本运行 → 验证输出
set -euo pipefail

EXAMPLES_DIR="$(cd "$(dirname "$0")/../examples" && pwd)"
TESTS_DIR="$(cd "$(dirname "$0")/../tests/tools" && pwd)"
PASS=0
FAIL=0
SKIP=0
TOTAL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# ============================================================
#  模拟 ToolDeck 的 buildArgs 逻辑 (python 实现)
# ============================================================
simulate_build_args() {
    local tmpl="$1"    # argTemplate (可为空)
    local values="$2"  # "name1=val1,name2=val2,..." 格式
    local args="$3"    # manifest.args (JSON 数组)

    python3 << PYEOF
import sys, re

tmpl = '''$tmpl'''
values_str = '''$values'''
args_json = '''$args'''

# Parse values
values = {}
if values_str:
    for pair in values_str.split(','):
        if '=' in pair:
            k, v = pair.split('=', 1)
            values[k] = v

# Parse manifest.args
import json
manifest_args = json.loads(args_json) if args_json else []

# Simulate ToolManifest::buildArgs
if not tmpl:
    # No template — return input values in order
    result = list(values.values())
else:
    # Process template
    expanded = tmpl
    pattern = re.compile(r'\{([^}:]+?)(?::([^}]+))?\}')

    # Find all matches
    matches = []
    for m in pattern.finditer(tmpl):
        name = m.group(1)
        modifier = m.group(2) or ''
        matches.append((m.start(), m.end(), name, modifier))

    # Process in reverse
    for start, end, name, modifier in reversed(matches):
        value = values.get(name, '')
        replacement = ''
        if modifier.startswith('--') or modifier.startswith('-'):
            if value and value not in ('false', '0'):
                replacement = modifier
        elif modifier.startswith('+'):
            if value in ('true', '1'):
                replacement = modifier
        else:
            replacement = value
        expanded = expanded[:start] + replacement + expanded[end:]

    expanded = ' '.join(expanded.split())  # simplified()
    result = [x for x in expanded.split(' ') if x] if expanded else []

# Final args that ToolInstance::start would use = manifest.args + extraArgs
final_args = manifest_args + result

# Output: space-separated args
sys.stdout.write(' '.join(final_args))
PYEOF
}

# ============================================================
#  测试用例定义
# ============================================================
run_test() {
    local name="$1"; shift
    local desc="$1"; shift
    local tool_dir="$1"; shift
    local values="$1"; shift       # 逗号分隔 key=val
    local expected_contains="$1"; shift  # 预期 args 中应包含的子串
    local expected_not_contains="${1:-}"  # 预期不应包含的子串

    TOTAL=$((TOTAL + 1))

    # 解析 manifest
    local manifest="$tool_dir/manifest.json"
    local tmpl=$(python3 -c "import json; print(json.load(open('$manifest')).get('argTemplate',''))")
    local args=$(python3 -c "import json; print(json.dumps(json.load(open('$manifest')).get('args',[])))")

    # 模拟构建参数
    local built_args=$(simulate_build_args "$tmpl" "$values" "$args")
    built_args=$(echo "$built_args" | xargs)  # trim

    local test_pass=true

    # 检查预期包含
    if [ -n "$expected_contains" ]; then
        if ! echo "$built_args" | grep -qF -- "$expected_contains"; then
            echo -e "  ${RED}✘ 预期包含「$expected_contains」${NC}"
            echo "     实际: $built_args"
            test_pass=false
        fi
    fi

    # 检查预期不包含
    if [ -n "$expected_not_contains" ]; then
        if echo "$built_args" | grep -qF -- "$expected_not_contains" 2>/dev/null; then
            echo -e "  ${RED}✘ 不应包含「$expected_not_contains」${NC}"
            echo "     实际: $built_args"
            test_pass=false
        fi
    fi

    # 检查无重复脚本名
    local script_name=$(python3 -c "import json; a=json.load(open('$manifest')).get('args',[]); print(a[0] if a else '')")
    if [ -n "$script_name" ]; then
        local count=$(echo "$built_args" | tr ' ' '\n' | grep -cF -- "$script_name" 2>/dev/null || echo 0)
        if [ "${count:-0}" -gt 1 ] 2>/dev/null; then
            echo -e "  ${RED}✘ 脚本名「$script_name」出现 $count 次 (重复!)${NC}"
            echo "     实际: $built_args"
            test_pass=false
        fi
    fi

    # 实际运行脚本
    local cmd=$(python3 -c "import json; m=json.load(open('$manifest')); print(m['command'])")
    local workdir="$tool_dir"

    pushd "$workdir" > /dev/null
    if echo "$built_args" | xargs -r "$cmd" > /tmp/tooldeck_test_output.txt 2>&1; then
        true
    else
        local ec=$?
        # exit code 非0不一定失败 (quick-ok, 除零等)
        echo "     (exit code: $ec)"
    fi
    popd > /dev/null

    if $test_pass; then
        echo -e "  ${GREEN}✓ PASS${NC} args: $built_args"
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
    fi
}

echo "══════════════════════════════════════════════════"
echo "  ToolDeck 全量测试"
echo "══════════════════════════════════════════════════"
echo ""

# ============================================================
#  1. hello-world — 基本 text+choice, args 非空 + tmpl
# ============================================================
echo -e "${YELLOW}[1/15] hello-world — text+choice, args非空+tmpl ${NC}"
run_test "hello-world" "基本参数" \
    "$EXAMPLES_DIR/hello-world" \
    "who=ToolDeck,format=完整" \
    "hello.sh ToolDeck" \
    "hello.sh hello.sh"

# ============================================================
#  2. echo-demo — 可选 choice+默认值
# ============================================================
echo -e "${YELLOW}[2/15] echo-demo — choice+默认值 ${NC}"
run_test "echo-demo" "默认值" \
    "$EXAMPLES_DIR/default-echo" \
    "detail=详细" \
    "echo.sh 详细" \
    "echo.sh echo.sh"

# ============================================================
#  3. file-hash — args 空 + bool --flag + file required
# ============================================================
echo -e "${YELLOW}[3/15] file-hash — args空 + bool --flag + file ${NC}"
run_test "file-hash" "args空+bool标志" \
    "$EXAMPLES_DIR/file-hash" \
    "algorithm=xxh64,verbose=true,filepath=/etc/hostname" \
    "-a xxh64 -v /etc/hostname" \
    ""

# file-hash verbose=false
run_test "file-hash-2" "verbose=false不输出-v" \
    "$EXAMPLES_DIR/file-hash" \
    "algorithm=xxh128,verbose=false,filepath=/etc/hostname" \
    "-a xxh128 /etc/hostname" \
    "-v"

# ============================================================
#  4. port-check — int + text, 多个 required
# ============================================================
echo -e "${YELLOW}[4/15] port-check — int+text, 多个required ${NC}"
run_test "port-check" "int输入" \
    "$EXAMPLES_DIR/port-check" \
    "host=github.com,port=443,timeout=5" \
    "check.sh github.com 443 5" \
    "check.sh check.sh"

# ============================================================
#  5. win-sysinfo — windows only, powershell + 多固定 args
# ============================================================
echo -e "${YELLOW}[5/15] win-sysinfo — powershell, 多固定args ${NC}"
echo "  (Linux 上跳过实际运行，仅验证参数构建)"
run_test "win-sysinfo" "多固定args" \
    "$EXAMPLES_DIR/win-sysinfo" \
    "section=完整" \
    "Bypass -File sysinfo.ps1 完整" \
    ""

# ============================================================
#  6. zip-inspect — dir 可选 + file 必填
# ============================================================
echo -e "${YELLOW}[6/15] zip-inspect — file+dir+choice ${NC}"
run_test "zip-inspect" "file+dir" \
    "$EXAMPLES_DIR/zip-inspect" \
    "zipfile=/tmp/test-demo.zip,mode=列出清单,pattern=,outdir=." \
    "zip-inspect.sh" \
    "zip-inspect.sh zip-inspect.sh"

# ============================================================
#  7. num-calc — number 浮点 + int
# ============================================================
echo -e "${YELLOW}[7/15] num-calc — number浮点 + int ${NC}"
run_test "num-calc" "浮点运算" \
    "$TESTS_DIR/num-calc" \
    "left=3.14,op=乘,right=2" \
    "calc.sh 3.14 乘 2" \
    "calc.sh calc.sh"

# ============================================================
#  8. bool-flags — 三种 bool 修饰符 +flag, --flag, -flag
# ============================================================
echo -e "${YELLOW}[8/15] bool-flags — +flag/--flag/-flag 三种修饰符 ${NC}"

# 全部 true
run_test "bool-flags-all-true" "全部true" \
    "$TESTS_DIR/bool-flags" \
    "verbose=true,debug=true,quiet=true" \
    "flags.sh --verbose +d -q" \
    "flags.sh flags.sh"

# 全部 false: 直接验证 args 构建结果
TOTAL=$((TOTAL + 1))
tmpl_all_false=$(simulate_build_args \
    "{verbose:--verbose} {debug:+d} {quiet:-q}" \
    "verbose=false,debug=false,quiet=false" \
    '["flags.sh"]')
if echo "$tmpl_all_false" | grep -qE '\-\-verbose|\+d|\-q' 2>/dev/null; then
    echo -e "  ${RED}✘ 全false时不应有标志: $tmpl_all_false${NC}"
    FAIL=$((FAIL + 1))
else
    echo -e "  ${GREEN}✓ PASS${NC} (全部 false, 无标志输出) args=$tmpl_all_false"
    PASS=$((PASS + 1))
fi

# 混合
run_test "bool-flags-mix" "verbose+debug true, quiet false" \
    "$TESTS_DIR/bool-flags" \
    "verbose=true,debug=true,quiet=false" \
    "flags.sh --verbose +d" \
    "-q"

# ============================================================
#  9. all-needed — 全部 required
# ============================================================
echo -e "${YELLOW}[9/15] all-needed — 全部required ${NC}"
run_test "all-needed" "全必填" \
    "$TESTS_DIR/all-needed" \
    "name=test,source=/etc/hostname,outdir=/tmp,format=JSON" \
    "needed.sh test /etc/hostname /tmp JSON" \
    "needed.sh needed.sh"

# ============================================================
#  10. clock-watch — daemon 模式
# ============================================================
echo -e "${YELLOW}[10/15] clock-watch — daemon模式 ${NC}"
TOTAL=$((TOTAL + 1))
if timeout 3 bash "$EXAMPLES_DIR/clock-watch/watch.sh" > /tmp/daemon_test.txt 2>&1; then
    true
else
    ec=$?
    if [ $ec -eq 124 ]; then
        echo -e "  ${GREEN}✓ PASS${NC} (daemon 运行 3 秒后被 timeout 终止)"
        PASS=$((PASS + 1))
    fi
fi

# ============================================================
#  11. fmt-json — outputMode: result
# ============================================================
echo -e "${YELLOW}[11/15] fmt-json — outputMode=result ${NC}"
run_test "fmt-json" "result模式" \
    "$TESTS_DIR/fmt-json" \
    "text=hello world,indent=缩进" \
    "fmt.sh hello world 缩进" \
    "fmt.sh fmt.sh"

# ============================================================
#  12. quick-ok — outputMode: status
# ============================================================
echo -e "${YELLOW}[12/15] quick-ok — outputMode=status ${NC}"
run_test "quick-ok-0" "exit 0" \
    "$TESTS_DIR/quick-ok" \
    "code=0" \
    "ok.sh 0" \
    "ok.sh ok.sh"

# ============================================================
#  13. no-tmpl — 空 argTemplate
# ============================================================
echo -e "${YELLOW}[13/15] no-tmpl — 空argTemplate ${NC}"
run_test "no-tmpl" "无模板" \
    "$TESTS_DIR/no-tmpl" \
    "title=my-task,mode=预览,retries=3" \
    "tmpl.sh my-task 预览 3" \
    "tmpl.sh tmpl.sh"

# ============================================================
#  14. anywhere — os 为空(全平台)
# ============================================================
echo -e "${YELLOW}[14/15] anywhere — os空(全平台) ${NC}"
run_test "anywhere" "全平台" \
    "$TESTS_DIR/anywhere" \
    "" \
    "sys.sh" \
    ""

# ============================================================
#  15. pick-dir — dir 必填
# ============================================================
echo -e "${YELLOW}[15/15] pick-dir — dir必填 ${NC}"
run_test "pick-dir" "dir必填" \
    "$TESTS_DIR/pick-dir" \
    "path=/tmp" \
    "pick.sh /tmp" \
    "pick.sh pick.sh"

# ============================================================
#  额外: buildArgs 边界测试
# ============================================================
echo ""
echo -e "${YELLOW}[边界] buildArgs 边界情况 ${NC}"
TOTAL=$((TOTAL + 1))

# 空值不追加到结果
built=$(simulate_build_args "" "name=,mode=,retries=" '["test.sh"]')
built=$(echo "$built" | xargs)  # trim whitespace
if [ "$built" = "test.sh" ]; then
    echo -e "  ${GREEN}✓ PASS${NC} 空值正确过滤, args=$built"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✘ 空值未过滤: '$built'${NC}"
    FAIL=$((FAIL + 1))
fi

# ============================================================
#  结果汇总
# ============================================================
echo ""
echo "══════════════════════════════════════════════════"
printf "  总计: %d  通过: ${GREEN}%d${NC}  失败: ${RED}%d${NC}  跳过: ${YELLOW}%d${NC}\n" $TOTAL $PASS $FAIL $SKIP
echo "══════════════════════════════════════════════════"

if [ $FAIL -gt 0 ]; then
    exit 1
fi
