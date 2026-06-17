/* MCU-A (底盘域) FreeRTOS 任务入口 */
#include "cmsis_os.h"
#include "main.h"
#include <stdint.h>
#include <string.h>

/* ---- 外部 HAL Handles (来自 CubeMX main.c) ---- */
extern FDCAN_HandleTypeDef hfdcan2;
extern FDCAN_HandleTypeDef hfdcan3;
extern UART_HandleTypeDef hlpuart1;   /* RS485-A1 */
extern UART_HandleTypeDef huart3;     /* RS485-A2 */
extern UART_HandleTypeDef huart1;     /* USART1 → RK3588 */
extern SPI_HandleTypeDef hspi2;       /* SPI Slave */
extern DMA_HandleTypeDef hdma_lpuart1_rx, hdma_lpuart1_tx;
extern DMA_HandleTypeDef hdma_usart3_rx, hdma_usart3_tx;
extern DMA_HandleTypeDef hdma_spi2_rx, hdma_spi2_tx;

/* ---- mcu_common 头文件 ---- */
#include "protocol_frame.h"
#include "protocol_codec.h"
#include "protocol_session.h"
#include "canfd_driver.h"
#include "rs485_driver.h"
#include "spi_slave_driver.h"

/* ---- 通道实例 ---- */
static canfd_channel_t g_canfd_ch[2];
static rs485_channel_t g_rs485_ch[2];
static spi_slave_t g_spi_slave;
static ProtocolSession g_session;

/* ---- 任务句柄 ---- */
static osThreadId_t safety_task_h, control_task_h, host_rx_task_h, host_tx_task_h;
static osThreadId_t can_rx_task_h, can_tx_task_h, rs485_task_h, dev_mgr_task_h;
static osThreadId_t diag_task_h, param_task_h, log_task_h, watchdog_task_h;
static osThreadId_t time_sync_task_h;

/* ---- 任务原型 ---- */
static void SafetyTask(void *arg);
static void RealtimeControlTask(void *arg);
static void HostLinkRxTask(void *arg);
static void HostLinkTxTask(void *arg);
static void CanFdRxTask(void *arg);
static void CanFdTxSchedulerTask(void *arg);
static void Rs485SchedulerTask(void *arg);
static void DeviceManagerTask(void *arg);
static void DiagnosticsTask(void *arg);
static void ParameterTask(void *arg);
static void LoggerTask(void *arg);
static void WatchdogTask(void *arg);
static void TimeSyncTask(void *arg);

/* ---- StartDefaultTask: 系统初始化 + 创建所有任务 ---- */
void AppInit(void) {
    

    /* 1. 初始化协议会话 */
    protocol_session_init(&g_session, 50);

    /* 2. 初始化 CAN FD 通道 */
    canfd_channel_init(&g_canfd_ch[0], 0, &hfdcan2);
    canfd_channel_init(&g_canfd_ch[1], 1, &hfdcan3);

    /* 3. 初始化 RS485 通道 */
    rs485_channel_init(&g_rs485_ch[0], 0, &hlpuart1, &hdma_lpuart1_tx, &hdma_lpuart1_rx,
                       GPIOA, GPIO_PIN_4);
    rs485_channel_init(&g_rs485_ch[1], 1, &huart3, &hdma_usart3_tx, &hdma_usart3_rx,
                       GPIOB, GPIO_PIN_0);

    /* 4. 初始化 SPI Slave */
    spi_slave_init(&g_spi_slave, 0, &hspi2, &hdma_spi2_rx, &hdma_spi2_tx);

    /* 5. 创建 FreeRTOS 任务 (优先级从高到低) */
    const osThreadAttr_t attr_critical  = { .name="Safety",   .priority=osPriorityRealtime7, .stack_size=512 };
    const osThreadAttr_t attr_high      = { .name="Realtime", .priority=osPriorityRealtime5, .stack_size=1024 };
    const osThreadAttr_t attr_mid_high  = { .name="HostRx",   .priority=osPriorityRealtime4, .stack_size=768 };
    const osThreadAttr_t attr_mid       = { .name="Mid",      .priority=osPriorityNormal,   .stack_size=512 };
    const osThreadAttr_t attr_low       = { .name="Low",      .priority=osPriorityLow,      .stack_size=512 };

    watchdog_task_h   = osThreadNew(WatchdogTask, NULL, &attr_critical);
    safety_task_h     = osThreadNew(SafetyTask, NULL,
        &(osThreadAttr_t){.name="Safety", .priority=osPriorityRealtime6, .stack_size=512});
    control_task_h    = osThreadNew(RealtimeControlTask, NULL,
        &(osThreadAttr_t){.name="Ctrl", .priority=osPriorityRealtime5, .stack_size=1024});
    host_rx_task_h    = osThreadNew(HostLinkRxTask, NULL,
        &(osThreadAttr_t){.name="HostRx", .priority=osPriorityRealtime4, .stack_size=768});
    host_tx_task_h    = osThreadNew(HostLinkTxTask, NULL,
        &(osThreadAttr_t){.name="HostTx", .priority=osPriorityRealtime3, .stack_size=768});
    can_rx_task_h     = osThreadNew(CanFdRxTask, NULL,
        &(osThreadAttr_t){.name="CanRx", .priority=osPriorityRealtime3, .stack_size=640});
    can_tx_task_h     = osThreadNew(CanFdTxSchedulerTask, NULL,
        &(osThreadAttr_t){.name="CanTx", .priority=osPriorityHigh, .stack_size=512});
    rs485_task_h      = osThreadNew(Rs485SchedulerTask, NULL,
        &(osThreadAttr_t){.name="RS485", .priority=osPriorityHigh, .stack_size=768});
    dev_mgr_task_h    = osThreadNew(DeviceManagerTask, NULL,
        &(osThreadAttr_t){.name="DevMgr", .priority=osPriorityNormal, .stack_size=512});
    diag_task_h       = osThreadNew(DiagnosticsTask, NULL,
        &(osThreadAttr_t){.name="Diag", .priority=osPriorityNormal, .stack_size=512});
    param_task_h      = osThreadNew(ParameterTask, NULL,
        &(osThreadAttr_t){.name="Param", .priority=osPriorityLow, .stack_size=512});
    log_task_h        = osThreadNew(LoggerTask, NULL,
        &(osThreadAttr_t){.name="Log", .priority=osPriorityLow, .stack_size=512});
    time_sync_task_h  = osThreadNew(TimeSyncTask, NULL,
        &(osThreadAttr_t){.name="Time", .priority=osPriorityLow, .stack_size=384});

    /* 6. 启动 SPI 从机首次接收 */
    spi_slave_prepare_response(&g_spi_slave, NULL, 0);

    /* 默认任务退出 (不再需要) */
    osThreadExit();
}

/* ---- 任务骨架实现 ---- */
static void SafetyTask(void *arg) {
    (void)arg;
    for (;;) {
        /* TODO: 安全状态机 + 硬件IO监控 */
        osDelay(1);
    }
}

static void RealtimeControlTask(void *arg) {
    (void)arg;
    for (;;) {
        /* TODO: 控制命令执行 + 限幅 + 斜坡 */
        osDelay(1);
    }
}

static void HostLinkRxTask(void *arg) {
    (void)arg;
    for (;;) {
        /* TODO: 解析RK下发的协议帧 */
        if (g_spi_slave.rx_complete) {
            ProtocolHeader hdr;
            if (protocol_header_deserialize(g_spi_slave.rx_buf, SPI_FRAME_SIZE, &hdr) == 0) {
                /* 分发消息 */
            }
            g_spi_slave.rx_complete = false;
            spi_slave_prepare_response(&g_spi_slave, g_spi_slave.tx_buf, SPI_FRAME_SIZE);
        }
        osDelay(1);
    }
}

static void HostLinkTxTask(void *arg) {
    (void)arg;
    for (;;) {
        /* TODO: 组装状态反馈帧 */
        osDelay(2);
    }
}

static void CanFdRxTask(void *arg) {
    (void)arg;
    for (;;) {
        canfd_frame_t frame;
        for (int i = 0; i < 2; i++) {
            while (canfd_channel_receive(&g_canfd_ch[i], &frame, 0) == 0) {
                /* TODO: 协议解析 + 分发 */
            }
        }
        osDelay(1);
    }
}

static void CanFdTxSchedulerTask(void *arg) {
    (void)arg;
    for (;;) {
        /* TODO: 周期帧调度 + 紧急帧优先 */
        osDelay(1);
    }
}

static void Rs485SchedulerTask(void *arg) {
    (void)arg;
    static uint32_t tick = 0;
    for (;;) {
        for (int i = 0; i < 2; i++) {
            rs485_channel_poll(&g_rs485_ch[i], tick);
        }
        tick++;
        osDelay(1);
    }
}

static void DeviceManagerTask(void *arg) {
    (void)arg;
    for (;;) { osDelay(10); }
}

static void DiagnosticsTask(void *arg) {
    (void)arg;
    for (;;) { osDelay(10); }
}

static void ParameterTask(void *arg) {
    (void)arg;
    for (;;) { osDelay(100); }
}

static void LoggerTask(void *arg) {
    (void)arg;
    for (;;) { osDelay(100); }
}

static void WatchdogTask(void *arg) {
    (void)arg;
    for (;;) { osDelay(5); }
}

static void TimeSyncTask(void *arg) {
    (void)arg;
    for (;;) { osDelay(1000); }
}
