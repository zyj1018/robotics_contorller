/* MCU-A Board: HAL 外设句柄 + 初始化函数 (从 CubeMX main.c 提取)
 * CubeMX 重新生成时, 用新的 main.c 内容覆盖此文件的对应函数即可。
 */
#include "main.h"

/* ---- HAL 外设句柄 ---- */
FDCAN_HandleTypeDef hfdcan2;
FDCAN_HandleTypeDef hfdcan3;

UART_HandleTypeDef hlpuart1;   /* RS485-A1 */
UART_HandleTypeDef huart1;     /* USART1 → RK3588 */
UART_HandleTypeDef huart3;     /* RS485-A2 */
DMA_HandleTypeDef hdma_lpuart1_rx;
DMA_HandleTypeDef hdma_lpuart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

PCD_HandleTypeDef hpcd_USB_FS;

/* ---- System Clock: HSE 8MHz → PLL → 170MHz ---- */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN = 20;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV6;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) { Error_Handler(); }
}

/* ---- FDCAN2: CAN FD-A1 (主驱动总线) ---- */
static void MX_FDCAN2_Init(void) {
    hfdcan2.Instance = FDCAN2;
    hfdcan2.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan2.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan2.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan2.Init.AutoRetransmission = DISABLE;
    hfdcan2.Init.TransmitPause = DISABLE;
    hfdcan2.Init.ProtocolException = DISABLE;
    hfdcan2.Init.NominalPrescaler = 2;
    hfdcan2.Init.NominalSyncJumpWidth = 20;
    hfdcan2.Init.NominalTimeSeg1 = 59;
    hfdcan2.Init.NominalTimeSeg2 = 20;
    hfdcan2.Init.DataPrescaler = 2;
    hfdcan2.Init.DataSyncJumpWidth = 4;
    hfdcan2.Init.DataTimeSeg1 = 11;
    hfdcan2.Init.DataTimeSeg2 = 4;
    hfdcan2.Init.StdFiltersNbr = 10;
    hfdcan2.Init.ExtFiltersNbr = 2;
    hfdcan2.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    if (HAL_FDCAN_Init(&hfdcan2) != HAL_OK) { Error_Handler(); }
}

/* ---- FDCAN3: CAN FD-A2 (扩展+BMS) ---- */
static void MX_FDCAN3_Init(void) {
    hfdcan3.Instance = FDCAN3;
    hfdcan3.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan3.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan3.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan3.Init.AutoRetransmission = DISABLE;
    hfdcan3.Init.TransmitPause = DISABLE;
    hfdcan3.Init.ProtocolException = DISABLE;
    hfdcan3.Init.NominalPrescaler = 2;
    hfdcan3.Init.NominalSyncJumpWidth = 20;
    hfdcan3.Init.NominalTimeSeg1 = 59;
    hfdcan3.Init.NominalTimeSeg2 = 20;
    hfdcan3.Init.DataPrescaler = 2;
    hfdcan3.Init.DataSyncJumpWidth = 4;
    hfdcan3.Init.DataTimeSeg1 = 11;
    hfdcan3.Init.DataTimeSeg2 = 4;
    hfdcan3.Init.StdFiltersNbr = 10;
    hfdcan3.Init.ExtFiltersNbr = 2;
    hfdcan3.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    if (HAL_FDCAN_Init(&hfdcan3) != HAL_OK) { Error_Handler(); }
}

/* ---- LPUART1: RS485-A1 ---- */
static void MX_LPUART1_UART_Init(void) {
    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 921600;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&hlpuart1) != HAL_OK) { Error_Handler(); }
}

/* ---- USART1: 救援串口 → RK3588 ---- */
static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK) { Error_Handler(); }
}

/* ---- USART3: RS485-A2 ---- */
static void MX_USART3_UART_Init(void) {
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 921600;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart3) != HAL_OK) { Error_Handler(); }
}

/* ---- SPI2: SPI Slave (RK通信) ---- */
static void MX_SPI2_Init(void) {
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_SLAVE;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_HARD_INPUT;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 7;
    hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    if (HAL_SPI_Init(&hspi2) != HAL_OK) { Error_Handler(); }
}

/* ---- USB PCD ---- */
static void MX_USB_PCD_Init(void) {
    hpcd_USB_FS.Instance = USB;
    hpcd_USB_FS.Init.dev_endpoints = 8;
    hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_FS.Init.Sof_enable = DISABLE;
    hpcd_USB_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_FS.Init.lpm_enable = DISABLE;
    hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
    if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK) { Error_Handler(); }
}

/* ---- DMA ---- */
static void MX_DMA_Init(void) {
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0); HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0); HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0); HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0); HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

/* ---- GPIO ---- */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(RS485_1_DIR_GPIO_Port, RS485_1_DIR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RS485_2_DIR_GPIO_Port, RS485_2_DIR_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = RS485_1_DIR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RS485_1_DIR_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RS485_2_DIR_Pin;
    HAL_GPIO_Init(RS485_2_DIR_GPIO_Port, &GPIO_InitStruct);
}

/* ---- 统一初始化入口 ---- */
void Board_Peripherals_Init(void) {
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_FDCAN2_Init();
    MX_FDCAN3_Init();
    MX_LPUART1_UART_Init();
    MX_USART1_UART_Init();
    MX_USART3_UART_Init();
    MX_SPI2_Init();
    MX_USB_PCD_Init();
}
