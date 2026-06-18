#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"

extern void SystemClock_Config(void);
extern void Board_Peripherals_Init(void);
extern void StartDefaultTask(void *argument);

static osThreadId_t defaultTaskHandle;
static const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask", .priority = (osPriority_t)osPriorityNormal, .stack_size = 128 * 4
};

int main(void) {
    HAL_Init(); SystemClock_Config(); Board_Peripherals_Init();
    osKernelInitialize();
    defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
    osKernelStart();
    while (1) {}
}
void Error_Handler(void) { __disable_irq(); while (1) {} }
