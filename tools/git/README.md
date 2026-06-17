# Git 工具集

用于日志生成、版本发布和版本管控的脚本集合。

## 目录

```
tools/git/
├── README.md              # 本文件
├── version.sh             # 版本号管理 (bump/set/tag)
├── changelog.sh           # CHANGELOG 生成
├── release.sh             # 发布流程管理
├── prepare-commit-msg     # Git Hook: 提交信息模板
└── commit-msg             # Git Hook: 提交信息校验
```

## 快速开始

### 1. 安装 Git Hooks (推荐)

```bash
cd robot_controller
cp tools/git/prepare-commit-msg .git/hooks/prepare-commit-msg
cp tools/git/commit-msg .git/hooks/commit-msg
chmod +x .git/hooks/prepare-commit-msg .git/hooks/commit-msg
```

安装后，每次 `git commit` 会自动生成提交模板并校验格式。

### 2. 初始化版本

```bash
./tools/git/version.sh init     # 创建 VERSION 文件
./tools/git/version.sh show     # 查看当前版本
```

## 日常使用

### 提交代码 (约定式提交)

```
<type>[scope]: <description>

类型:
  feat      新功能      feat(motion): add S-curve acceleration
  fix       Bug修复     fix(spi): handle CRC error resync
  perf      性能优化     perf(canfd): reduce TX latency by 30%
  refactor  代码重构     refactor(rs485): extract transaction state machine
  docs      文档         docs(arch): update CAN bus allocation
  test      测试         test(protocol): add sequence number replay test
  chore     构建/工具    chore(build): add MISRA check to CI
  ci        CI/CD       ci: add automatic firmware signing
  security  安全修复     security: validate firmware signature before boot
  arch      架构变更     arch: migrate to subsystem division (V2)
```

### 版本管理

```bash
# 查看当前版本
./tools/git/version.sh show

# 自增版本号
./tools/git/version.sh bump patch    # 0.1.0 → 0.1.1 (Bug修复)
./tools/git/version.sh bump minor    # 0.1.0 → 0.2.0 (新功能, 向后兼容)
./tools/git/version.sh bump major    # 0.1.0 → 1.0.0 (架构/协议不兼容变更)

# 打标签
git add VERSION && git commit -m "chore(release): bump version to 1.2.0"
./tools/git/version.sh tag
git push origin v1.2.0
```

### 变更日志

```bash
# 预览自上次发布以来的变更
./tools/git/changelog.sh preview

# 生成完整 CHANGELOG.md
./tools/git/changelog.sh generate

# 查看两版本间变更
./tools/git/changelog.sh between v1.0.0 v1.1.0
```

### 发布流程

```bash
# 1. 发布前检查
./tools/git/release.sh check

# 2. 开始发布 (创建 release 分支 + 更新版本 + 生成 CHANGELOG)
./tools/git/release.sh start 1.2.0

# 3. 在此分支上做最后测试和修复...

# 4. 完成发布 (合并到 main + 打标签 + 推送)
./tools/git/release.sh finish

# 紧急修复
./tools/git/release.sh hotfix 1.2.1
```

## 版本号规则 (语义化版本)

```
MAJOR.MINOR.PATCH

MAJOR (主版本): 架构/协议不兼容变更
  例: V1(方案一)→V2(方案四)、协议帧格式变更

MINOR (次版本): 新增功能，向后兼容
  例: 新增 CANopen 设备、新增 RS485 协议适配

PATCH (补丁版本): Bug 修复，向后兼容
  例: SPI CRC 处理修复、CAN Bus Off 恢复时序修正
```

## 分支规范

```
main               # 生产分支，只接受 release/hotfix 合并
develop            # 开发分支 (可选)
feature/<name>     # 功能分支: feat/xxx
fix/<name>         # 修复分支: fix/xxx
release/vX.Y.Z     # 发布分支
hotfix/vX.Y.Z      # 紧急修复分支
```
