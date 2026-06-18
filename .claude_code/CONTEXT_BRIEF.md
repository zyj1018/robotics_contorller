# 上下文速览 — 新设备快速上手

> 换设备后，**按顺序阅读以下文件即可恢复全部上下文**：

---

## 阅读顺序（5 分钟恢复上下文）

### 1. 本文件 (`CONTEXT_BRIEF.md`) — 30 秒了解项目
### 2. `WORKFLOW.md` — 分支管理 & 开发流程
### 3. `PROJECT_STATE.md` — 当前状态和待办
### 3. `DESIGN_DECISIONS.md` — 8 个关键决策及理由
### 4. `SESSION_LOG.md` — 完整会话历史
### 5. `../ARCHITECTURE_V2.md` — 当前推荐架构（完整设计）

---

## 30 秒速览

```
项目: RK3588 + 双 STM32G474 机器人控制器
阶段: 总体架构设计完成，待进入 P0 架构验证

硬件:
  RK3588 (Linux) ←SPI/USB/USART→ MCU-A (底盘域) + MCU-B (机械臂域)
  每MCU: 2×CAN FD + 4×RS485，整机 4×CAN FD + 8×RS485

架构方案: V2 子系统划分（底盘 vs 机械臂/上装）
  - MCU-A: 行走/转向/舵轮/BMS/IMU/超声/灯光
  - MCU-B: 关节1-7/夹爪/云台/升降/末端传感器
  - 4路CAN全部参与运动控制，代码复用 ~85%

切换V2的原因:
  V1(运动/辅助域)下全部运动设备压在MCU-A 2路CAN →
  经典CAN场景过载 108% →
  MCU-B 的CAN资源闲置 →
  切换到V2(底盘/臂域)后 4路CAN均衡 → 全部<55%

关键文件:
  ARCHITECTURE_V2.md  - 当前推荐架构（1253行，39章节）
  ARCHITECTURE.md     - V1 架构（保留参考，2672行）
  .claude_code/       - 会话记录和设计决策（本目录）

阻塞项（需要外部确认）:
  1. STM32G474 具体封装型号
  2. 机械臂轴数上限
  3. 驱动器CAN协议类型（经典CAN/CAN FD）
  4. 功能安全目标等级
```

---

## 给 Claude 的启动指令

换到新设备后，对 Claude 说：

> 我在开发 RK3588 + 双 STM32G474 机器人控制器项目。请先阅读 `.claude_code/CONTEXT_BRIEF.md` 了解项目概要，然后阅读 `ARCHITECTURE_V2.md` 了解当前推荐架构。之后告诉我你对项目状态的理解，以及建议的下一步工作。
