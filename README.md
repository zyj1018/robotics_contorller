# RK3588 + 双 STM32G474 机器人控制器

基于 RK3588 (Linux) + 双 STM32G474 (FreeRTOS) 的工业/移动机器人控制器软硬件架构。

## 架构概览

```
RK3588 (Linux / ROS 2)
  ├── SPI-A ──→ MCU-A STM32G474 (底盘域)
  │              ├── CAN FD-A1/A2 → 行走/转向/舵轮/BMS
  │              └── RS485-A1~A4  → 编码器/IMU/超声/安全IO
  │
  └── SPI-B ──→ MCU-B STM32G474 (机械臂/上装域)
                 ├── CAN FD-B1/B2 → 关节1-7/夹爪/云台
                 └── RS485-B1~B4  → 编码器/升降/末端传感器
```

**架构文档**: 见 `ARCHITECTURE_V2.md`（当前推荐方案：子系统划分）

## 目录结构

```
robot_controller/
├── rk3588/              # RK3588 Linux 侧 (C++17)
│   ├── transport/       #   SPI/USB/UART Transport 层
│   ├── protocol/        #   协议 C++ 封装
│   ├── mcu_gateway/     #   MCU Gateway (McuEndpoint/McuManager)
│   ├── kinematics/      #   运动学协调器
│   └── ...
├── mcu_common/          # STM32 公共代码 (C11, MCU-A/B 共享)
│   ├── osal/            #   OSAL 抽象层 (FreeRTOS / POSIX)
│   ├── protocol/        #   协议帧 + 编解码 + CRC + 会话管理
│   ├── drivers/         #   外设驱动接口 (CAN FD / RS485 / SPI)
│   ├── canfd/           #   CAN FD 框架
│   ├── rs485/           #   RS485 框架
│   └── ...
├── mcu_a/               # MCU-A 底盘域业务代码
├── mcu_b/               # MCU-B 机械臂域业务代码
├── idl/                 # 数据字典 (单一事实源, YAML)
├── generated/           # 自动生成代码 (禁止手工修改)
├── tools/
│   ├── git/             #   版本发布工具链
│   └── codegen/         #   代码生成器
├── tests/
│   ├── unit/            #   单元测试
│   ├── mock/            #   Mock 驱动
│   └── fault_injection/ #   故障注入
└── docs/                # 文档
```

## 环境安装

### 必要工具

```bash
# Ubuntu 22.04
sudo apt update
sudo apt install -y \
    build-essential cmake git \
    python3 python3-pip \
    gcc g++

# Python 依赖 (代码生成器)
pip install pyyaml

# ARM 交叉编译工具链 (可选, 编译 MCU 固件时需要)
sudo apt install -y gcc-arm-none-eabi
```

### 克隆仓库

```bash
git clone https://github.com/zyj1018/robotics_contorller.git
cd robotics_contorller

# 安装 Git Hooks (推荐)
cp tools/git/prepare-commit-msg .git/hooks/
cp tools/git/commit-msg .git/hooks/
chmod +x .git/hooks/prepare-commit-msg .git/hooks/commit-msg
```

## 构建

### PC 端编译 (开发+测试)

```bash
mkdir build && cd build
cmake .. -DTARGET_PLATFORM=PC
make -j$(nproc)
```

### ARM 交叉编译 (MCU 固件)

```bash
mkdir build_arm && cd build_arm
cmake .. \
    -DTARGET_PLATFORM=ARM \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain_arm_gcc.cmake
make -j$(nproc)
```

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `TARGET_PLATFORM` | `PC` | `PC`(POSIX+Mock) 或 `ARM`(FreeRTOS+HAL) |
| `BUILD_TESTS` | `ON` | 编译单元测试 |
| `BUILD_RK3588` | `ON` | 编译 RK3588 侧代码 |
| `BUILD_MCU_A` | `ON` | 编译 MCU-A 固件 |
| `BUILD_MCU_B` | `ON` | 编译 MCU-B 固件 |
| `ENABLE_WERROR` | `ON` | 警告视为错误 |

## 测试

### 运行全部测试

```bash
cd build
ctest --output-on-failure
```

### 运行单个测试

```bash
./build/tests/unit/mcu_common/test_protocol_codec
./build/tests/unit/mcu_common/test_protocol_crc
./build/tests/unit/mcu_common/test_protocol_session
```

## 固件输出

ARM 交叉编译后，产出物位于 `build_arm/` 下：

```
build_arm/
├── mcu_a/
│   ├── mcu_a_firmware.elf     # ELF (含调试符号, 192KB)
│   ├── mcu_a_firmware.bin     # 原始二进制 (54KB, 烧录用)
│   └── mcu_a_firmware.hex     # Intel HEX (151KB, 烧录用)
├── mcu_b/
│   ├── mcu_b_firmware.elf
│   ├── mcu_b_firmware.bin
│   └── mcu_b_firmware.hex
└── mcu_common/
    └── libmcu_common.a        # 公共库 (4.8KB)
```

| 格式 | 用途 | 烧录工具 |
|------|------|----------|
| `.elf` | 调试 (含符号表) | STM32CubeIDE / GDB |
| `.bin` | 量产烧录 | STM32CubeProgrammer / J-Flash |
| `.hex` | 量产烧录 | STM32CubeProgrammer / J-Flash |

### 当前测试覆盖

| 测试套件 | 用例数 | 覆盖内容 |
|----------|:-----:|----------|
| `protocol_codec` | 16 | Header/Frame 序列化, 同步搜索, CRC错误, 版本校验, 序列号/TTL/会话, 空Payload, sync边界(NULL/0/1字节), CRC篡改 |
| `protocol_crc` | 6 | CRC8/CRC16 已知向量, 空数据, 一致性, 差异性 |
| `protocol_session` | 7 | 会话初始化/启停, 心跳, 入站校验, 重放拒绝, TTL超时, 发送序列号 |
| `link_manager` | 8 | 链路仲裁(SPI优先), USB拒绝控制, USART救援白名单, 序列号重放, TTL过期, SPI断开撤销控制权 |
| `spi_full_duplex` | 3 | SPI全双工帧交换, Link Manager过期拒绝, 10帧持续交换 |

## 代码生成

从数据字典 (YAML) 自动生成 C/C++ 头文件：

```bash
python3 tools/codegen/generate.py
# 输出:
#   generated/mcu_common/generated_enums.h
#   generated/mcu_common/generated_messages.h
#   generated/mcu_common/generated_fault_codes.h
#   generated/rk3588/generated_enums.hpp
```

编辑 `idl/*.yaml` 后重新运行即可。**不要手工修改 `generated/` 下的文件。**

## 开发工作流

### Git 提交规范 (约定式提交)

```
<type>[scope]: <description>

类型:
  feat      新功能     feat(canfd): add Bus Off recovery
  fix       Bug修复    fix(spi): handle CRC error resync
  perf      性能优化   perf(motion): optimize trajectory
  refactor  代码重构   refactor(rs485): extract FSM
  docs      文档       docs: update CAN allocation
  test      测试       test(protocol): add replay test
  chore     构建/工具  chore(build): add MISRA check
  arch      架构变更   arch: migrate to subsystem division
```

> 安装 Git Hook 后会自动校验提交信息格式。

### 版本管理

```bash
./tools/git/version.sh show              # 查看当前版本
./tools/git/version.sh bump patch        # 补丁版本 +1
./tools/git/version.sh bump minor        # 次版本 +1
./tools/git/version.sh tag               # 打 Git 标签
```

### 发布流程

```bash
./tools/git/release.sh check             # 发布前检查 (10项)
./tools/git/release.sh start 1.0.0       # 开始发布
# ... 测试和修复 ...
./tools/git/release.sh finish            # 完成发布 (合并+标签+推送)
```

### CHANGELOG

```bash
./tools/git/changelog.sh preview         # 预览未发布变更
./tools/git/changelog.sh generate        # 生成 CHANGELOG.md
```

## 开发阶段

| 阶段 | 状态 | 说明 |
|------|:----:|------|
| P0: 架构验证 | ✅ 完成 | 构建系统, OSAL, 协议层, Mock, 测试, 代码生成 |
| P1: 驱动开发 | ✅ 完成 | CAN FD/RS485/SPI HAL 驱动, FreeRTOS 任务骨架, CubeMX 集成 |
| P2: 协议联调 | ✅ 完成 | Link Manager (链路仲裁/会话/救援), Transport 层, SPI 全双工集成测试 |
| P3: 设备接入 | ⬜ 待开始 | CANopen, Modbus, 电机驱动, 编码器 |
| P4: 控制闭环 | ⬜ 待开始 | 运动控制, 限幅/斜坡, 安全状态机 |
| P5: 系统服务 | ⬜ 待开始 | 参数管理, 故障诊断, 日志, 看门狗, 固件升级 |
| P6-P8 | ⬜ 后续 | 整机联调, 测试验证, 产品化 |

## 关键设计决策

详见 `.claude_code/DESIGN_DECISIONS.md`。核心决策：

- **子系统划分** (V2): 底盘域 vs 机械臂/上装域，4路CAN均衡参与运动控制
- **两组独立 SPI**: 故障隔离，一块MCU故障不影响另一块
- **同类通道共用任务**: 节省RAM，CAN接收由中断+DMA保证实时性
- **自定义定长二进制协议**: 32字节Header，零解析开销，适合硬实时
- **配置注入而非条件编译**: 禁止 `#ifdef MCU_A`

## 许可

MIT License — 见 `LICENSE` 文件。
