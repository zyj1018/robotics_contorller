# STM32CubeMX 配置清单

> 这是 MCU-A 和 MCU-B 的 CubeMX 配置规格。按此清单配置后，将生成的工程文件夹交给我继续开发。

## 一、假设与约束

| 项目 | 假设值 | 说明 |
|------|--------|------|
| MCU 型号 | **STM32G474RETx** (64-pin) | 如有变更需同步修改引脚分配 |
| CubeMX 版本 | 6.x | H7/F7/G4 均可 |
| RTOS | FreeRTOS CMSIS_V2 | 选 CMSIS V2 API |
| 固件包 | STM32Cube FW_G4 V1.5+ | — |

> ⚠️ 如果实际型号不同，请告知我修改。关键约束：该封装必须同时支持 2×FDCAN + 4×UART + SPI + USB + SWD。

## 二、MCU-A（底盘域）配置

### 2.1 总体设置

```
Project Name:   mcu_a_cubemx
Toolchain:      Makefile (或直接生成代码文件夹)
堆栈:           Stack=0x800, Heap=0x2000
时钟:           HSE=8MHz → PLL → SYSCLK=170MHz
                HCLK=170MHz, APB1=170MHz, APB2=170MHz
                FDCAN Clock = PCLK1 (170MHz) 或 PLLQ
```

### 2.2 外设分配

| 外设 | 实例 | 功能 | 引脚(64pin) | DMA | 中断 |
|------|------|------|-------------|-----|:--:|
| **FDCAN1** | FDCAN1 | CAN FD-A1 (主驱动总线) | PD0(RX), PD1(TX) | — | RX FIFO0 NEW | 
| **FDCAN2** | FDCAN2 | CAN FD-A2 (扩展+BMS) | PB12(RX), PB13(TX) | — | RX FIFO0 NEW |
| **USART2** | USART2 | RS485-A1 | PD5(TX), PD6(RX) | DMA_TX + DMA_RX(循环) | UART IDLE |
| **USART3** | USART3 | RS485-A2 | PB10(TX), PB11(RX) | DMA_TX + DMA_RX(循环) | UART IDLE |
| **UART4** | UART4 | RS485-A3 | PA0(TX), PA1(RX) | DMA_TX + DMA_RX(循环) | UART IDLE |
| **UART5** | UART5 | RS485-A4 | PC12(TX), PD2(RX) | DMA_TX + DMA_RX(循环) | UART IDLE |
| **SPI1** | SPI1 | SPI Slave (RK通信) | PA5(SCK), PA6(MISO), PA7(MOSI), PA4(NSS) | DMA_TX + DMA_RX | RX Complete |
| **USB** | USB_OTG_FS | USB Device (CDC) | PA11(DM), PA12(DP) | — | — |
| **USART1** | USART1 | 救援串口 (RK通信) | PA9(TX), PA10(RX) | — | — |
| **SWD** | — | 调试接口 | PA13(SWDIO), PA14(SWCLK) | — | — |

### 2.3 额外 GPIO

| GPIO | 方向 | 用途 |
|------|------|------|
| PC4 | Output | RS485-A1 方向控制 (DE/RE) |
| PC5 | Output | RS485-A2 方向控制 (DE/RE) |
| PC6 | Output | RS485-A3 方向控制 (DE/RE) |
| PC7 | Output | RS485-A4 方向控制 (DE/RE) |
| PB0 | Output | SPI Data Ready (通知RK) |
| PB1 | Input | RK→MCU 硬件复位 |
| PA8 | Output | 外部WDT喂狗 |
| PC13 | Input | 硬件急停输入 (EXTI) |
| PB5 | Output | 驱动器使能 (DRV_ENABLE, 高有效) |
| PB3 | Input/Output | MCU-B 心跳 (交叉连接) |
| PB4 | Input/Output | MCU-B 安全状态 (交叉连接) |

### 2.4 FreeRTOS 配置

```
TICK_RATE_HZ:           1000 Hz
MINIMAL_STACK_SIZE:     128 words
TOTAL_HEAP_SIZE:        32768 (32KB)
MAX_PRIORITIES:         7
USE_PREEMPTION:         Enabled
USE_MUTEXES:            Enabled
USE_RECURSIVE_MUTEXES:  Disabled
USE_COUNTING_SEMAPHORES: Enabled
USE_TASK_NOTIFICATIONS:  Enabled
USE_EVENT_GROUPS:        Enabled
USE_TIMERS:              Enabled
```

---

## 三、MCU-B（机械臂/上装域）配置

MCU-B 的外设配置**基本与 MCU-A 相同**，仅 GPIO 和功能标签不同。建议直接复制 MCU-A 的 CubeMX 工程，修改以下内容：

### 3.1 总体设置

```
Project Name:   mcu_b_cubemx
(其余设置同 MCU-A)
```

### 3.2 外设分配（同 MCU-A，仅功能标签不同）

| 外设 | 实例 | 功能(MCU-B) | 引脚(同MCU-A) |
|------|------|------------|--------------|
| FDCAN1 | FDCAN1 | CAN FD-B1 (主关节总线 关节1-4) | PD0/PD1 |
| FDCAN2 | FDCAN2 | CAN FD-B2 (末端+上装 关节5-7) | PB12/PB13 |
| USART2 | USART2 | RS485-B1 (关节编码器组1-4) | PD5/PD6 |
| USART3 | USART3 | RS485-B2 (关节编码器组5-7) | PB10/PB11 |
| UART4 | UART4 | RS485-B3 (升降机构) | PA0/PA1 |
| UART5 | UART5 | RS485-B4 (末端传感器) | PC12/PD2 |
| SPI1 | SPI1 | SPI Slave (RK通信) | PA5-PA7/PA4 |
| USB | USB_OTG_FS | USB Device | PA11/PA12 |
| USART1 | USART1 | 救援串口 | PA9/PA10 |

### 3.3 MCU-B 特定 GPIO

| GPIO | 方向 | 用途 |
|------|------|------|
| PB5 | Output | 机械臂驱动器使能 (DRV_ENABLE_B) |
| PB3 | Input/Output | MCU-A 心跳 (交叉连接) |
| PB4 | Input/Output | MCU-A 安全状态 (交叉连接) |
| (其余同 MCU-A) | | |

---

## 四、生成后交给我的文件

CubeMX 配置完成后，将整个工程文件夹打包发给我，我需要：

```
mcu_a_cubemx/
├── mcu_a_cubemx.ioc           # CubeMX 工程文件（最重要）
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── stm32g4xx_hal_conf.h
│   │   ├── stm32g4xx_it.h
│   │   └── FreeRTOSConfig.h
│   └── Src/
│       ├── main.c
│       ├── stm32g4xx_hal_msp.c
│       ├── stm32g4xx_it.c
│       └── freertos.c
└── Drivers/                   # HAL 库（可以只给我路径，我用 ST 官方版本）
```

或者**只给我 `.ioc` 文件**，我可以自行生成代码。

---

## 五、后续工作（P1 计划）

拿到 CubeMX 配置后，我将完成：

1. ✅ 把生成的 HAL 代码移植到 `mcu_a/board/` 和 `mcu_b/board/`
2. ✅ 在生成的 HAL 基础上实现真实外设驱动（替换当前的 stub）
3. ✅ 集成 FreeRTOS，创建 V2 架构中规划的 14 个任务骨架
4. ✅ 实现 SPI Slave 中断+DMA 接收、CAN FD RX 中断处理
5. ✅ 实现 RS485 DMA 循环接收 + UART IDLE 帧检测
6. ✅ 编写 Link Manager（见架构文档第 6 节）
7. ✅ ARM 目标编译 + 在 Nucleo-G474 开发板上验证（如果你有的话）
