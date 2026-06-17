#include "cmsis_os.h"
extern void AppInit(void);

void StartDefaultTask(void *argument) {
    (void)argument;
    AppInit();
    osThreadExit();
}
