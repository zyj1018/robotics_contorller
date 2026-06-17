#!/bin/bash
# ============================================================
# changelog.sh — 变更日志生成器
# 用法:
#   ./changelog.sh generate              # 生成完整 CHANGELOG.md
#   ./changelog.sh since v1.0.0          # 从指定标签生成
#   ./changelog.sh unreleased            # 生成未发布变更
#   ./changelog.sh preview               # 预览下次发布内容
#   ./changelog.sh between v1.0.0 v2.0.0 # 两版本间变更
# ============================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel 2>/dev/null || echo "$SCRIPT_DIR/../..")"
CHANGELOG_FILE="$REPO_ROOT/CHANGELOG.md"

# 颜色
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'

# -----------------------------------------------------------
# 约定式提交分类 (Conventional Commits)
# -----------------------------------------------------------
declare -A COMMIT_CATEGORIES=(
    ["feat"]="🚀 新功能"
    ["fix"]="🐛 修复"
    ["perf"]="⚡ 性能优化"
    ["refactor"]="🔧 重构"
    ["docs"]="📝 文档"
    ["test"]="✅ 测试"
    ["chore"]="🏗️  构建/工具"
    ["ci"]="🔄 CI/CD"
    ["style"]="💄 代码风格"
    ["revert"]="⏪ 回滚"
    ["security"]="🔒 安全"
    ["arch"]="🏛️  架构"
)

# -----------------------------------------------------------
# 获取两版本之间的提交
# -----------------------------------------------------------
get_commits_between() {
    local from="$1"
    local to="${2:-HEAD}"

    if [ "$from" = "unreleased" ]; then
        # 从最新 tag 到 HEAD
        local last_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
        if [ -z "$last_tag" ]; then
            git log --oneline --no-merges
        else
            git log --oneline --no-merges "${last_tag}..HEAD"
        fi
    elif [ "$from" = "all" ]; then
        git log --oneline --no-merges
    else
        git log --oneline --no-merges "${from}..${to}" 2>/dev/null || echo ""
    fi
}

# -----------------------------------------------------------
# 按类型分类提交
# -----------------------------------------------------------
categorize_commits() {
    local commits="$1"
    local output=""

    for category in "${!COMMIT_CATEGORIES[@]}"; do
        local matched=$(echo "$commits" | grep -iE "^[a-f0-9]+ ${category}[\(:]" 2>/dev/null || true)
        if [ -n "$matched" ]; then
            output+="\n### ${COMMIT_CATEGORIES[$category]}\n\n"
            while IFS= read -r line; do
                # 提取 hash 和 message
                local hash=$(echo "$line" | awk '{print $1}')
                local msg=$(echo "$line" | cut -d' ' -f2-)
                # 美化：去掉 type 前缀，首字母大写
                local clean_msg=$(echo "$msg" | sed -E 's/^[a-z]+(\([^)]*\))?:\s*//')
                output+="- ${hash} — ${clean_msg}\n"
            done <<< "$matched"
        fi
    done

    # 未分类的提交
    local uncategorized=$(echo "$commits" | grep -iEv "^[a-f0-9]+ (feat|fix|perf|refactor|docs|test|chore|ci|style|revert|security|arch)[\(:]" 2>/dev/null || true)
    if [ -n "$uncategorized" ]; then
        output+="\n### 🔹 其他\n\n"
        while IFS= read -r line; do
            local hash=$(echo "$line" | awk '{print $1}')
            local msg=$(echo "$line" | cut -d' ' -f2-)
            output+="- ${hash} — ${msg}\n"
        done <<< "$uncategorized"
    fi

    echo -e "$output"
}

# -----------------------------------------------------------
# 列出所有标签
# -----------------------------------------------------------
list_tags() {
    git tag --sort=-v:refname 2>/dev/null | head -20
}

# -----------------------------------------------------------
# 生成完整 CHANGELOG
# -----------------------------------------------------------
generate_full() {
    echo "# CHANGELOG" > "$CHANGELOG_FILE"
    echo "" >> "$CHANGELOG_FILE"
    echo "> 本文件由 \`tools/git/changelog.sh\` 自动生成，基于 [约定式提交](https://www.conventionalcommits.org/zh-hans/)。" >> "$CHANGELOG_FILE"
    echo "" >> "$CHANGELOG_FILE"

    # 读取项目版本
    local version_file="$REPO_ROOT/VERSION"
    if [ -f "$version_file" ]; then
        echo "**当前版本**: $(cat "$version_file")" >> "$CHANGELOG_FILE"
    fi
    echo "" >> "$CHANGELOG_FILE"
    echo "---" >> "$CHANGELOG_FILE"
    echo "" >> "$CHANGELOG_FILE"

    local tags=$(git tag --sort=-v:refname 2>/dev/null)
    local prev_tag=""

    # 先输出未发布变更
    local unreleased_commits=$(get_commits_between "unreleased")
    if [ -n "$unreleased_commits" ]; then
        echo "## [未发布] — $(date +%Y-%m-%d)" >> "$CHANGELOG_FILE"
        echo "" >> "$CHANGELOG_FILE"
        categorize_commits "$unreleased_commits" >> "$CHANGELOG_FILE"
        echo "" >> "$CHANGELOG_FILE"
        echo "---" >> "$CHANGELOG_FILE"
        echo "" >> "$CHANGELOG_FILE"
    fi

    # 输出每个 tag 的变更
    for tag in $tags; do
        local tag_date=$(git log -1 --format=%ai "$tag" 2>/dev/null | cut -d' ' -f1)
        echo "## [$tag] — $tag_date" >> "$CHANGELOG_FILE"
        echo "" >> "$CHANGELOG_FILE"

        if [ -n "$prev_tag" ]; then
            local commits=$(get_commits_between "$prev_tag" "$tag")
        else
            local commits=$(get_commits_between "$tag")
            # 首个 tag 之前的所有提交
        fi

        if [ -n "$commits" ]; then
            categorize_commits "$commits" >> "$CHANGELOG_FILE"
        else
            echo "_此版本无变更记录_" >> "$CHANGELOG_FILE"
        fi

        echo "" >> "$CHANGELOG_FILE"
        echo "---" >> "$CHANGELOG_FILE"
        echo "" >> "$CHANGELOG_FILE"
        prev_tag="$tag"
    done

    echo -e "${GREEN}CHANGELOG.md 已生成${NC}"
    echo "文件: $CHANGELOG_FILE"
}

# -----------------------------------------------------------
# 预览未发布变更
# -----------------------------------------------------------
preview_unreleased() {
    local last_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
    local commits=$(get_commits_between "unreleased")

    echo ""
    echo "========================================"
    if [ -n "$last_tag" ]; then
        echo "  自 $last_tag 以来的未发布变更"
    else
        echo "  未发布变更 (无历史标签)"
    fi
    echo "========================================"
    echo ""

    if [ -z "$commits" ]; then
        echo -e "${YELLOW}  无未发布变更${NC}"
    else
        echo -e "$commits" | while read line; do
            local hash=$(echo "$line" | awk '{print $1}')
            local msg=$(echo "$line" | cut -d' ' -f2-)
            echo -e "  ${CYAN}$hash${NC} $msg"
        done
    fi

    echo ""
    echo "  提交总数: $(echo "$commits" | grep -c . || echo 0)"
    echo "========================================"
}

# -----------------------------------------------------------
# 两版本间对比
# -----------------------------------------------------------
between_versions() {
    local from="$1" to="$2"
    local commits=$(get_commits_between "$from" "$to")

    echo ""
    echo "========================================"
    echo "  $from → $to 变更"
    echo "========================================"
    echo ""

    if [ -z "$commits" ]; then
        echo -e "${YELLOW}  无变更${NC}"
    else
        categorize_commits "$commits"
    fi

    echo ""
    echo "  提交总数: $(echo "$commits" | grep -c . || echo 0)"
    echo "  变更文件: $(git diff --stat "$from" "$to" 2>/dev/null | tail -1 || echo 'N/A')"
    echo "========================================"
}

# -----------------------------------------------------------
# 主入口
# -----------------------------------------------------------
main() {
    cd "$REPO_ROOT"

    case "${1:-}" in
        generate|gen)
            generate_full
            ;;
        preview|unreleased)
            preview_unreleased
            ;;
        since)
            if [ $# -lt 2 ]; then
                echo -e "${RED}用法: $0 since <tag>${NC}"
                echo "可用标签:"
                list_tags
                exit 1
            fi
            local commits=$(get_commits_between "$2" "HEAD")
            echo ""
            echo "========================================"
            echo "  自 $2 以来的变更"
            echo "========================================"
            categorize_commits "$commits"
            ;;
        between)
            if [ $# -lt 3 ]; then
                echo -e "${RED}用法: $0 between <from_tag> <to_tag>${NC}"
                exit 1
            fi
            between_versions "$2" "$3"
            ;;
        tags)
            echo "最近 20 个标签:"
            list_tags
            ;;
        *)
            echo "用法: $0 {generate|preview|since|between|tags}"
            echo ""
            echo "  generate          生成完整 CHANGELOG.md"
            echo "  preview           预览未发布变更"
            echo "  since <tag>       查看自某标签以来的变更"
            echo "  between <t1> <t2> 查看两版本间变更"
            echo "  tags              列出最近标签"
            exit 1
            ;;
    esac
}

main "$@"
