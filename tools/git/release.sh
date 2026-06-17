#!/bin/bash
# ============================================================
# release.sh — 发布流程管理
# 用法:
#   ./release.sh start <version>    # 开始发布流程
#   ./release.sh check              # 发布前检查清单
#   ./release.sh finish             # 完成发布 (tag + push)
#   ./release.sh rollback           # 回滚发布
#   ./release.sh hotfix <version>   # 创建 hotfix 分支
# ============================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel 2>/dev/null || echo "$SCRIPT_DIR/../..")"
VERSION_SCRIPT="$SCRIPT_DIR/version.sh"
CHANGELOG_SCRIPT="$SCRIPT_DIR/changelog.sh"

# 颜色
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'

# 分支命名规范
MAIN_BRANCH="main"
DEVELOP_BRANCH="develop"
RELEASE_PREFIX="release"
HOTFIX_PREFIX="hotfix"

# -----------------------------------------------------------
# 发布前检查
# -----------------------------------------------------------
pre_release_check() {
    local errors=0
    local warnings=0

    echo ""
    echo "========================================"
    echo "  发布前检查清单"
    echo "========================================"
    echo ""

    # 1. 检查工作区
    echo -n "  [1/10] 工作区干净 ................ "
    if git diff-index --quiet HEAD -- 2>/dev/null; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗ 有未提交更改${NC}"
        ((errors++))
    fi

    # 2. 检查当前分支
    echo -n "  [2/10] 在正确分支 ................ "
    local branch=$(git branch --show-current 2>/dev/null)
    if [[ "$branch" == "$MAIN_BRANCH" ]] || [[ "$branch" == ${RELEASE_PREFIX}/* ]]; then
        echo -e "${GREEN}✓ ($branch)${NC}"
    else
        echo -e "${YELLOW}⚠ ($branch, 建议在 $MAIN_BRANCH 或 release/*)${NC}"
        ((warnings++))
    fi

    # 3. 检查是否已同步远程
    echo -n "  [3/10] 与远程同步 ................ "
    git fetch origin 2>/dev/null || true
    local behind=$(git rev-list HEAD..origin/$MAIN_BRANCH --count 2>/dev/null || echo "?")
    local ahead=$(git rev-list origin/$MAIN_BRANCH..HEAD --count 2>/dev/null || echo "?")
    if [ "$behind" = "0" ] 2>/dev/null && [ "$ahead" = "0" ]; then
        echo -e "${GREEN}✓${NC}"
    elif [ "$behind" != "0" ] 2>/dev/null; then
        echo -e "${RED}✗ 落后远程 $behind 个提交${NC}"
        ((errors++))
    else
        echo -e "${YELLOW}⚠ 领先远程 $ahead 个提交${NC}"
        ((warnings++))
    fi

    # 4. 检查 VERSION 文件
    echo -n "  [4/10] VERSION 文件存在 ........... "
    if [ -f "$REPO_ROOT/VERSION" ]; then
        echo -e "${GREEN}✓ ($(cat "$REPO_ROOT/VERSION"))${NC}"
    else
        echo -e "${YELLOW}⚠ 不存在${NC}"
        ((warnings++))
    fi

    # 5. 检查 CHANGELOG
    echo -n "  [5/10] CHANGELOG.md 存在 .......... "
    if [ -f "$REPO_ROOT/CHANGELOG.md" ]; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⚠ 不存在${NC}"
        ((warnings++))
    fi

    # 6. 检查未合并的特性分支
    echo -n "  [6/10] 无未合并分支 .............. "
    local unmerged=$(git branch --no-merged HEAD 2>/dev/null | grep -vE "^\*" | wc -l)
    if [ "$unmerged" -eq 0 ]; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⚠ $unmerged 个未合并分支${NC}"
        ((warnings++))
    fi

    # 7. 检查 tag 是否已存在
    echo -n "  [7/10] 标签不冲突 ................ "
    local version=$(cat "$REPO_ROOT/VERSION" 2>/dev/null || echo "0.0.0")
    if git rev-parse "v$version" >/dev/null 2>&1; then
        echo -e "${RED}✗ v$version 已存在${NC}"
        ((errors++))
    else
        echo -e "${GREEN}✓${NC}"
    fi

    # 8. 检查是否有 CI 配置
    echo -n "  [8/10] CI 配置存在 ................ "
    if [ -f "$REPO_ROOT/.github/workflows" ] || [ -f "$REPO_ROOT/.gitlab-ci.yml" ] || [ -f "$REPO_ROOT/ci/Jenkinsfile" ]; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⚠ 未检测到 CI 配置${NC}"
        ((warnings++))
    fi

    # 9. 检查架构文档
    echo -n "  [9/10] 架构文档 .................. "
    if [ -f "$REPO_ROOT/ARCHITECTURE_V2.md" ] || [ -f "$REPO_ROOT/ARCHITECTURE.md" ]; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⚠ 未检测到架构文档${NC}"
        ((warnings++))
    fi

    # 10. 检查最近提交信息格式
    echo -n "  [10/10] 提交信息格式 .............. "
    local bad_commits=$(git log --oneline -10 | grep -vE "(feat|fix|perf|refactor|docs|test|chore|ci|style|revert|security|arch)[\(:]" | grep -v "Merge" | wc -l)
    if [ "$bad_commits" -eq 0 ]; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⚠ $bad_commits 个最近提交可能不符合约定式提交${NC}"
        ((warnings++))
    fi

    echo ""
    echo "========================================"
    echo -e "  错误: ${RED}$errors${NC}  警告: ${YELLOW}$warnings${NC}"
    echo "========================================"

    if [ "$errors" -gt 0 ]; then
        echo -e "${RED}请先解决以上错误再发布${NC}"
        return 1
    else
        echo -e "${GREEN}检查通过，可以发布${NC}"
        return 0
    fi
}

# -----------------------------------------------------------
# 开始发布
# -----------------------------------------------------------
start_release() {
    local version="$1"
    local release_branch="${RELEASE_PREFIX}/v${version}"

    echo "========================================"
    echo "  开始发布 v${version}"
    echo "========================================"

    # 更新版本号
    bash "$VERSION_SCRIPT" set "$version"

    # 创建 release 分支
    git checkout -b "$release_branch" 2>/dev/null || {
        echo -e "${YELLOW}分支 $release_branch 已存在，切换到该分支${NC}"
        git checkout "$release_branch"
    }

    # 生成 CHANGELOG
    if [ -x "$CHANGELOG_SCRIPT" ]; then
        bash "$CHANGELOG_SCRIPT" generate
        git add CHANGELOG.md 2>/dev/null || true
    fi

    # 提交版本变更
    git add VERSION
    git commit -m "chore(release): prepare v${version}

- 更新版本号至 ${version}
- 生成 CHANGELOG

Co-Authored-By: release.sh" || true

    echo ""
    echo "========================================"
    echo -e "${GREEN}发布分支已创建: $release_branch${NC}"
    echo "========================================"
    echo ""
    echo "后续步骤:"
    echo "  1. 在此分支上进行最终测试"
    echo "  2. 修复发现的问题"
    echo "  3. 运行: $0 finish"
}

# -----------------------------------------------------------
# 完成发布
# -----------------------------------------------------------
finish_release() {
    local branch=$(git branch --show-current 2>/dev/null)
    local version=$(cat "$REPO_ROOT/VERSION" 2>/dev/null || echo "0.0.0")

    if [[ ! "$branch" =~ ^${RELEASE_PREFIX}/ ]]; then
        echo -e "${RED}错误: 当前不在 release 分支 ($branch)${NC}"
        exit 1
    fi

    echo "========================================"
    echo "  完成发布 v${version}"
    echo "========================================"

    # 运行发布前检查
    if ! pre_release_check; then
        echo -e "${RED}发布前检查未通过${NC}"
        exit 1
    fi

    # 合并到 main
    echo ""
    echo "合并到 $MAIN_BRANCH ..."
    git checkout "$MAIN_BRANCH"
    git merge --no-ff "$branch" -m "chore(release): merge release v${version}"
    git branch -d "$branch"

    # 打标签
    bash "$VERSION_SCRIPT" tag

    # 推送到远程
    echo ""
    read -p "是否推送到远程? [y/N] " confirm
    if [ "$confirm" = "y" ] || [ "$confirm" = "Y" ]; then
        git push origin "$MAIN_BRANCH"
        git push origin "v${version}"
        echo -e "${GREEN}已推送${NC}"
    fi

    echo ""
    echo "========================================"
    echo -e "${GREEN}发布完成! v${version}${NC}"
    echo "========================================"
}

# -----------------------------------------------------------
# 回滚发布
# -----------------------------------------------------------
rollback_release() {
    local version="${1:-}"
    local branch=$(git branch --show-current 2>/dev/null)

    echo "========================================"
    echo "  回滚发布"
    echo "========================================"

    if [ -n "$version" ]; then
        # 删除 tag
        if git rev-parse "v$version" >/dev/null 2>&1; then
            echo "删除本地标签 v$version ..."
            git tag -d "v$version"
            read -p "是否删除远程标签? [y/N] " confirm
            if [ "$confirm" = "y" ] || [ "$confirm" = "Y" ]; then
                git push origin ":v$version" 2>/dev/null || true
            fi
        fi
    fi

    # 如果还在 release 分支，切换回 main
    if [[ "$branch" =~ ^${RELEASE_PREFIX}/ ]]; then
        echo "切换回 $MAIN_BRANCH ..."
        git checkout "$MAIN_BRANCH"
        read -p "是否删除 release 分支 $branch? [y/N] " confirm
        if [ "$confirm" = "y" ] || [ "$confirm" = "Y" ]; then
            git branch -D "$branch"
        fi
    fi

    echo -e "${GREEN}回滚完成${NC}"
}

# -----------------------------------------------------------
# Hotfix
# -----------------------------------------------------------
create_hotfix() {
    local version="$1"
    local hotfix_branch="${HOTFIX_PREFIX}/v${version}"

    echo "========================================"
    echo "  创建 Hotfix v${version}"
    echo "========================================"

    git checkout "$MAIN_BRANCH"
    git pull origin "$MAIN_BRANCH" 2>/dev/null || true
    git checkout -b "$hotfix_branch"

    bash "$VERSION_SCRIPT" set "$version"

    echo ""
    echo -e "${GREEN}Hotfix 分支已创建: $hotfix_branch${NC}"
    echo ""
    echo "后续步骤:"
    echo "  1. 在此分支上修复问题"
    echo "  2. 提交修复: git commit -m 'fix: ...'"
    echo "  3. 运行: $0 finish"
}

# -----------------------------------------------------------
# 主入口
# -----------------------------------------------------------
main() {
    cd "$REPO_ROOT"

    case "${1:-}" in
        start)
            if [ $# -lt 2 ]; then
                echo -e "${RED}用法: $0 start <version>${NC}"
                exit 1
            fi
            start_release "$2"
            ;;
        check)
            pre_release_check
            ;;
        finish)
            finish_release
            ;;
        rollback)
            rollback_release "${2:-}"
            ;;
        hotfix)
            if [ $# -lt 2 ]; then
                echo -e "${RED}用法: $0 hotfix <version>${NC}"
                exit 1
            fi
            create_hotfix "$2"
            ;;
        *)
            echo "用法: $0 {start|check|finish|rollback|hotfix}"
            echo ""
            echo "  start <version>   开始发布 (创建 release 分支)"
            echo "  check             发布前检查清单"
            echo "  finish            完成发布 (合并 + 打标签 + 推送)"
            echo "  rollback [ver]    回滚发布"
            echo "  hotfix <version>  创建 hotfix 分支"
            echo ""
            echo "示例:"
            echo "  $0 check                    # 发布前检查"
            echo "  $0 start 1.2.0              # 开始 v1.2.0 发布"
            echo "  $0 finish                   # 完成发布"
            echo "  $0 hotfix 1.2.1             # 创建 hotfix"
            exit 1
            ;;
    esac
}

main "$@"
