# 开发 & 审核工作流

## 分支模型

```
main ──●──●──●──●─────────●──●──●──●──────→
        \         /        \         /
codex    ●──●──●─●          ●──●──●─●
         审核+修复  merge    审核+修复  merge
```

| 分支 | 用途 | 生命周期 |
|------|------|----------|
| `main` | 日常开发，所有代码产出 | 永久 |
| `codex` | 代码审核，审核修复 | 临时（每次审核后删除重建） |

## 操作流程

### 日常开发（在 main）

```bash
git checkout main
# 写代码、提交...
git push origin main
```

### 需要审核时

```bash
git checkout main
git checkout -b codex         # 从 main 创建新的 codex 分支
git push origin codex
# → 触发 codex agent 对 codex 分支进行审核
```

### 审核修复（在 codex）

```bash
git checkout codex
# 根据审核报告修复问题
git add ...
git commit -m "fix: ..."
git push origin codex
# → 可反复触发审核直到通过
```

### 审核通过后合并

```bash
git checkout main
git merge codex               # 合并修复回 main
git push origin main
# 清理 codex：
git branch -d codex
git push origin --delete codex
```

### 如果审核期间 main 有新的提交

```bash
git checkout codex
git merge main                # 同步 main 最新代码到 codex
# 解决冲突（如有），继续审核/修复
```

## 提交规范

遵循 [约定式提交](https://www.conventionalcommits.org/zh-hans/)：

```
<type>[scope]: <description>

type:
  feat      新功能     feat(canfd): add Bus Off auto-recovery
  fix       Bug修复    fix(spi): handle CRC error resync
  perf      性能优化   perf(motion): optimize trajectory planner
  refactor  代码重构   refactor(rs485): extract transaction FSM
  docs      文档       docs: update CAN bus allocation
  test      测试       test(protocol): add sync boundary tests
  chore     构建/工具  chore(build): add MISRA check
  arch      架构变更   arch: migrate to subsystem division (V2)
```

## 版本发布

```bash
./tools/git/release.sh check             # 发布前检查
./tools/git/version.sh bump minor        # 版本自增
./tools/git/release.sh start x.y.z       # 开始发布
# ... 测试验证 ...
./tools/git/release.sh finish            # 合并+打标签+推送
```

## 审核标准

每次 codex 审核必须通过以下检查：

| 检查项 | 标准 |
|--------|------|
| PC 编译 | `cmake -DTARGET_PLATFORM=PC && make` 无错误 |
| ARM 编译 | `cmake -DTARGET_PLATFORM=ARM -DCMAKE_TOOLCHAIN_FILE=... && make` 无错误 |
| 单元测试 | `ctest` 100% 通过 |
| 代码风格 | 遵循项目现有风格 |
| 关键路径 | 协议编解码、CAN驱动、RS485驱动、安全状态机 |
