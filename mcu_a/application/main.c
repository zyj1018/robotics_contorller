/* MCU-A 自定义 main — 复用 CubeMX HAL 初始化 */
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"

/* 外部函数声明 (来自 CubeMX main.c) */
extern void SystemClock_Config(void);
extern void MX_GPIO_Init(void);
extern void MX_DMA_Init(void);
extern void MX_FDCAN2_Init(void);
extern void MX_FDCAN3_Init(void);
extern void MX_LPUART1_UART_Init(void);
extern void MX_USART1_UART_Init(void);
extern void MX_USART3_UART_Init(void);
extern void MX_SPI2_Init(void);

/* 我们的 StartDefaultTask (在 app_freertos.c 中定义) */
extern void StartDefaultTask(void *argument);

static osThreadId_t defaultTaskHandle;
static const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .priority = (osPriority_t)osPriorityNormal,
    .stack_size = 128 * 4
};

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_FDCAN2_Init();
    MX_FDCAN3_Init();
    MX_LPUART1_UART_Init();
    MX_USART1_UART_Init();
    MX_USART3_UART_Init();
    MX_SPI2_Init();
    MX_USB_PCD_Init();

    osKernelInitialize();
    defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
    osKernelStart();

    while (1) {}
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}
