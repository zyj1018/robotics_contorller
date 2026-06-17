/* StartDefaultTask — CubeMX FreeRTOS入口, 调用我们的 AppInit */
#include "cmsis_os.h"
extern void AppInit(void);

void StartDefaultTask(void *argument) {
    (void)argument;
    AppInit();
    osThreadExit();
}
