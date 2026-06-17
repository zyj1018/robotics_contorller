#include "spi_slave_driver.h"
#include <string.h>
int spi_slave_init(spi_slave_t *slave, uint8_t id) { slave->instance_id=id; slave->rx_len=0; slave->tx_len=0; memset(&slave->stats,0,sizeof(slave->stats)); return 0; }
int spi_slave_prepare_response(spi_slave_t *slave, const uint8_t *data, uint16_t len) { memcpy(slave->tx_buf,data,len); slave->tx_len=len; return 0; }
int spi_slave_get_request(spi_slave_t *slave, uint8_t *buf, uint16_t cap, uint16_t *len) { if(slave->rx_len>cap)return-1; memcpy(buf,slave->rx_buf,slave->rx_len); *len=slave->rx_len; return 0; }
