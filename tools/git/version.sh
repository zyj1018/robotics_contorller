#!/bin/bash
# ============================================================
# version.sh — 版本管理工具
# 用法:
#   ./version.sh show          # 显示当前版本
#   ./version.sh bump major    # 大版本 +1  (1.0.0 → 2.0.0)
#   ./version.sh bump minor    # 次版本 +1  (1.0.0 → 1.1.0)
#   ./version.sh bump patch    # 补丁版本 +1 (1.0.0 → 1.0.1)
#   ./version.sh set 2.1.0     # 直接设置版本
#   ./version.sh tag           # 打 git tag (带注释)
#   ./version.sh init          # 初始化版本文件
# ============================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel 2>/dev/null || echo "$SCRIPT_DIR/../..")"
VERSION_FILE="$REPO_ROOT/VERSION"
CHANGELOG_FILE="$REPO_ROOT/CHANGELOG.md"

# 颜色
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

# -----------------------------------------------------------
# 读取当前版本
# -----------------------------------------------------------
read_version() {
    if [ -f "$VERSION_FILE" ]; then
        cat "$VERSION_FILE" | tr -d '[:space:]'
    else
        echo "0.0.0"
    fi
}

# -----------------------------------------------------------
# 写入版本
# -----------------------------------------------------------
write_version() {
    echo "$1" > "$VERSION_FILE"
    echo -e "${GREEN}版本已更新: $(read_version)${NC}"
}

# -----------------------------------------------------------
# 版本号自增
# -----------------------------------------------------------
bump_version() {
    local current=$(read_version)
    local major=$(echo "$current" | cut -d. -f1)
    local minor=$(echo "$current" | cut -d. -f2)
    local patch=$(echo "$current" | cut -d. -f3)

    case "${1:-patch}" in
        major)
            major=$((major + 1)); minor=0; patch=0
            ;;
        minor)
            minor=$((minor + 1)); patch=0
            ;;
        patch)
            patch=$((patch + 1))
            ;;
        *)
            echo -e "${RED}错误: 未知的自增类型 '$1'。请使用 major/minor/patch${NC}"
            exit 1
            ;;
    esac

    echo "${major}.${minor}.${patch}"
}

# -----------------------------------------------------------
# 解析语义化版本
# -----------------------------------------------------------
parse_semver() {
    local v="$1"
    if ! echo "$v" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.]+)?(\+[a-zA-Z0-9.]+)?$'; then
        echo -e "${RED}错误: 无效的语义化版本 '$v'${NC}"
        exit 1
    fi
}

# -----------------------------------------------------------
# 打 Git 标签
# -----------------------------------------------------------
create_tag() {
    local version=$(read_version)
    local tag="v${version}"

    # 检查工作区是否干净
    if ! git diff-index --quiet HEAD -- 2>/dev/null; then
        echo -e "${YELLOW}警告: 工作区有未提交的更改${NC}"
        read -p "是否继续打标签? [y/N] " confirm
        if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
            exit 0
        fi
    fi

    # 检查标签是否已存在
    if git rev-parse "$tag" >/dev/null 2>&1; then
        echo -e "${RED}错误: 标签 $tag 已存在${NC}"
        exit 1
    fi

    # 获取最近提交信息作为 tag 注释
    local last_commit_msg=$(git log -1 --pretty=%B 2>/dev/null | head -5)

    git tag -a "$tag" -m "Release $tag

$last_commit_msg"

    echo -e "${GREEN}已创建标签: $tag${NC}"
    echo ""
    echo "推送到远程:"
    echo "  git push origin $tag"
}

# -----------------------------------------------------------
# 显示当前版本
# -----------------------------------------------------------
show_version() {
    local version=$(read_version)
    local last_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "无")

    echo "================================"
    echo "  项目版本信息"
    echo "================================"
    echo "  版本文件 (VERSION):  $version"
    echo "  最新 Git 标签:       $last_tag"
    echo "  当前分支:            $(git branch --show-current 2>/dev/null || echo 'N/A')"
    echo "  最近提交:            $(git log -1 --format='%h %s' 2>/dev/null || echo 'N/A')"
    echo "================================"
}

# -----------------------------------------------------------
# 初始化
# -----------------------------------------------------------
init_version() {
    if [ -f "$VERSION_FILE" ]; then
        echo -e "${YELLOW}VERSION 文件已存在: $(read_version)${NC}"
        read -p "是否覆盖? [y/N] " confirm
        if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
            exit 0
        fi
    fi
    write_version "0.1.0"
    echo -e "${GREEN}版本文件已初始化${NC}"
}

# -----------------------------------------------------------
# 主入口
# -----------------------------------------------------------
main() {
    cd "$REPO_ROOT"

    case "${1:-show}" in
        show)
            show_version
            ;;
        bump)
            if [ $# -lt 2 ]; then
                echo -e "${RED}用法: $0 bump <major|minor|patch>${NC}"
                exit 1
            fi
            local new_version=$(bump_version "$2")
            write_version "$new_version"
            echo ""
            echo "下一步:"
            echo "  git add VERSION"
            echo "  git commit -m 'chore: bump version to $new_version'"
            echo "  $0 tag"
            ;;
        set)
            if [ $# -lt 2 ]; then
                echo -e "${RED}用法: $0 set <version>${NC}"
                exit 1
            fi
            parse_semver "$2"
            write_version "$2"
            ;;
        tag)
            create_tag
            ;;
        init)
            init_version
            ;;
        *)
            echo "用法: $0 {show|bump|set|tag|init}"
            echo ""
            echo "  show          显示当前版本"
            echo "  bump <type>   版本自增 (major/minor/patch)"
            echo "  set <version> 直接设置版本"
            echo "  tag           打 Git 标签"
            echo "  init          初始化 VERSION 文件"
            exit 1
            ;;
    esac
}

main "$@"
