#ifndef SPI_SLAVE_DRIVER_H
#define SPI_SLAVE_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

#define SPI_FRAME_SIZE 256

typedef struct {
    uint32_t rx_bytes, tx_bytes, crc_errors, overrun_count;
} spi_stats_t;

typedef struct spi_slave {
    uint8_t  instance_id;
    void    *spi_handle;       /* SPI_HandleTypeDef* */
    void    *rx_dma;
    void    *tx_dma;
    uint8_t  rx_buf[SPI_FRAME_SIZE];
    uint8_t  tx_buf[SPI_FRAME_SIZE];
    volatile bool rx_complete;
    volatile bool tx_complete;
    spi_stats_t stats;
    void    *user_data;
} spi_slave_t;

int  spi_slave_init(spi_slave_t *s, uint8_t id, void *spi_handle, void *rx_dma, void *tx_dma);
int  spi_slave_prepare_response(spi_slave_t *s, const uint8_t *data, uint16_t len);
int  spi_slave_wait_transfer(spi_slave_t *s, uint32_t timeout_ms);
void spi_slave_isr_rx_complete(spi_slave_t *s);
#endif
