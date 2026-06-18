#include <stdbool.h>
#ifndef MOCK_SPI_H
#define MOCK_SPI_H
#include <stdint.h>
#define MOCK_SPI_BUF_SIZE 256
typedef struct {
    uint8_t master_tx[MOCK_SPI_BUF_SIZE]; uint8_t slave_tx[MOCK_SPI_BUF_SIZE];
    uint8_t master_rx[MOCK_SPI_BUF_SIZE]; uint8_t slave_rx[MOCK_SPI_BUF_SIZE];
    uint16_t tx_len; uint16_t rx_len; bool drdy;
} mock_spi_link_t;
void mock_spi_link_init(mock_spi_link_t *link);
void mock_spi_master_transfer(mock_spi_link_t *link);
void mock_spi_slave_prepare(mock_spi_link_t *link, const uint8_t *data, uint16_t len);
void mock_spi_inject_crc_error(mock_spi_link_t *link);
#endif
