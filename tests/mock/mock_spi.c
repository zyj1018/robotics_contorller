#include "mock_spi.h"
#include <string.h>
void mock_spi_link_init(mock_spi_link_t *link){memset(link,0,sizeof(*link));}
void mock_spi_master_transfer(mock_spi_link_t *link){memcpy(link->master_rx,link->slave_tx,MOCK_SPI_BUF_SIZE);memcpy(link->slave_rx,link->master_tx,MOCK_SPI_BUF_SIZE);link->drdy=false;}
void mock_spi_slave_prepare(mock_spi_link_t *link, const uint8_t *data, uint16_t len){memcpy(link->slave_tx,data,len);link->drdy=true;}
void mock_spi_inject_crc_error(mock_spi_link_t *link){link->master_rx[0]^=0xFF;}
