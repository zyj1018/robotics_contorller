#!/usr/bin/env python3
"""Claude Code Hook: 构造发布前检查结果的 JSON 输出"""
import json, sys

check_result = sys.argv[1]
check_exit = int(sys.argv[2])
version = sys.argv[3]

if check_exit == 0:
    context = (
        f"## 发布前检查结果 ✅\n\n"
        f"当前版本: **{version}**\n\n"
        f"```\n{check_result}\n```\n\n"
        f"**检查通过！** 可以继续发布流程。常用命令:\n"
        f"- 开始发布: `./tools/git/release.sh start <version>`\n"
        f"- 小版本自增: `./tools/git/version.sh bump minor`\n"
        f"- 完成发布: `./tools/git/release.sh finish`"
    )
else:
    context = (
        f"## 发布前检查结果 ❌\n\n"
        f"当前版本: **{version}**\n\n"
        f"```\n{check_result}\n```\n\n"
        f"**检查未通过。** 请先解决以上问题再发布。"
    )

output = {
    "continue": True,
    "hookSpecificOutput": {
        "hookEventName": "UserPromptSubmit",
        "additionalContext": context
    }
}
print(json.dumps(output, ensure_ascii=False))
