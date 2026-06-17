#ifndef SPI_SLAVE_DRIVER_H
#define SPI_SLAVE_DRIVER_H
#include <stdint.h>
#include <stddef.h>

typedef struct { uint32_t rx_bytes; uint32_t tx_bytes; uint32_t crc_errors; } spi_stats_t;

typedef struct spi_slave {
    uint8_t instance_id; void *spi_handle; void *rx_dma; void *tx_dma;
    uint8_t rx_buf[512]; uint8_t tx_buf[512]; uint16_t rx_len; uint16_t tx_len;
    spi_stats_t stats; void *user_data;
} spi_slave_t;

int spi_slave_init(spi_slave_t *slave, uint8_t instance_id);
int spi_slave_prepare_response(spi_slave_t *slave, const uint8_t *data, uint16_t len);
int spi_slave_get_request(spi_slave_t *slave, uint8_t *buf, uint16_t capacity, uint16_t *len);
#endif
