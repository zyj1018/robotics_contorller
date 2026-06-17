/* MCU-B (机械臂域) FreeRTOS 任务入口 */
#include "cmsis_os.h"
#include "main.h"

extern FDCAN_HandleTypeDef hfdcan2, hfdcan3;
extern UART_HandleTypeDef hlpuart1, huart3, huart1;
extern SPI_HandleTypeDef hspi2;
extern DMA_HandleTypeDef hdma_lpuart1_rx, hdma_lpuart1_tx;
extern DMA_HandleTypeDef hdma_usart3_rx, hdma_usart3_tx;
extern DMA_HandleTypeDef hdma_spi2_rx, hdma_spi2_tx;

#include "protocol_frame.h"
#include "protocol_codec.h"
#include "protocol_session.h"
#include "canfd_driver.h"
#include "rs485_driver.h"
#include "spi_slave_driver.h"

static canfd_channel_t g_canfd_ch[2];
static rs485_channel_t g_rs485_ch[2];
static spi_slave_t g_spi_slave;
static ProtocolSession g_session;

void AppInit(void) {
    
    protocol_session_init(&g_session, 50);
    canfd_channel_init(&g_canfd_ch[0], 0, &hfdcan2);
    canfd_channel_init(&g_canfd_ch[1], 1, &hfdcan3);
    rs485_channel_init(&g_rs485_ch[0], 0, &hlpuart1, &hdma_lpuart1_tx, &hdma_lpuart1_rx, GPIOA, GPIO_PIN_4);
    rs485_channel_init(&g_rs485_ch[1], 1, &huart3, &hdma_usart3_tx, &hdma_usart3_rx, GPIOB, GPIO_PIN_0);
    spi_slave_init(&g_spi_slave, 0, &hspi2, &hdma_spi2_rx, &hdma_spi2_tx);
    spi_slave_prepare_response(&g_spi_slave, NULL, 0);

    /* MCU-B 仅创建骨架任务 (后续完善) */
    osThreadExit();
}
