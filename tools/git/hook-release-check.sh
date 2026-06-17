#!/bin/bash
# ============================================================
# Claude Code Hook: 检测发布意图并自动运行发布前检查
#
# 触发: UserPromptSubmit 事件
# 行为: 当用户输入包含发布关键词时，运行 release.sh check，
#       将检查结果注入到 Claude 的上下文
# ============================================================
set -euo pipefail

INPUT=$(cat)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel 2>/dev/null || echo "$SCRIPT_DIR/../..")"

# 提取用户 prompt 文本
PROMPT=$(python3 -c "
import json, sys
try:
    d = json.loads(sys.argv[1])
    for key in ('prompt', 'user_prompt', 'text', 'message', 'tool_input'):
        val = d.get(key, '')
        if isinstance(val, str) and val:
            print(val)
            break
except: pass
" "$INPUT" 2>/dev/null)

# 无 prompt 内容则放行
if [ -z "$PROMPT" ]; then
    echo '{"continue": true}'
    exit 0
fi

# 检测发布关键词
if ! echo "$PROMPT" | grep -qiE '(发布版本|release version|版本发布|发版|打版本|tag version|release new|发布新版本)'; then
    echo '{"continue": true}'
    exit 0
fi

# 运行发布前检查
VERSION=$(cat "$REPO_ROOT/VERSION" 2>/dev/null || echo "0.0.0")
CHECK_OUTPUT=$(bash "$REPO_ROOT/tools/git/release.sh" check 2>&1) || true
CHECK_EXIT=${PIPESTATUS[0]}

# 构造 JSON 响应
python3 "$SCRIPT_DIR/_hook_release_output.py" "$CHECK_OUTPUT" "$CHECK_EXIT" "$VERSION"
