#include "spi_slave_driver.h"
#include <string.h>
#ifdef DRIVER_BACKEND_HAL
#ifdef DRIVER_BACKEND_HAL
#include "stm32g4xx_hal.h"
#endif
#endif

int spi_slave_init(spi_slave_t *s, uint8_t id, void *spi_handle, void *rx_dma, void *tx_dma) {
    if (!s) return -1;
    memset(s, 0, sizeof(*s));
    s->instance_id = id;
    s->spi_handle = spi_handle;
    s->rx_dma = rx_dma;
    s->tx_dma = tx_dma;
    /* 预填充 TX 缓冲为空闲帧 */
    memset(s->tx_buf, 0x00, SPI_FRAME_SIZE);
    return 0;
}

int spi_slave_prepare_response(spi_slave_t *s, const uint8_t *data, uint16_t len) {
    if (!s || len > SPI_FRAME_SIZE) return -1;
    memset(s->tx_buf, 0x00, SPI_FRAME_SIZE);
    if (data && len) memcpy(s->tx_buf, data, len);
    s->tx_complete = false;
    s->rx_complete = false;
    /* 启动 SPI DMA 收发 */
#ifdef DRIVER_BACKEND_HAL
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)s->spi_handle;
    HAL_SPI_TransmitReceive_DMA(hspi, s->tx_buf, s->rx_buf, SPI_FRAME_SIZE);
#endif
    return 0;
}

int spi_slave_wait_transfer(spi_slave_t *s, uint32_t timeout_ms) {
    (void)timeout_ms;
    /* 实际: 等待信号量或轮询 rx_complete */
    return s->rx_complete ? 0 : -1;
}

void spi_slave_isr_rx_complete(spi_slave_t *s) {
    if (!s) return;
    s->rx_complete = true;
    s->tx_complete = true;
    s->stats.rx_bytes += SPI_FRAME_SIZE;
    s->stats.tx_bytes += SPI_FRAME_SIZE;
}
