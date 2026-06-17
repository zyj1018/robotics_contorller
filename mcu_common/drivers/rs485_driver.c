#include "rs485_driver.h"
#include <string.h>
int rs485_channel_init(rs485_channel_t *ch, uint8_t id) { ch->channel_id=id; ch->txn_state=RS485_IDLE; memset(&ch->stats,0,sizeof(ch->stats)); return 0; }
int rs485_submit_transaction(rs485_channel_t *ch, const rs485_txn_t *txn) { (void)ch;(void)txn; return 0; }
int rs485_cancel_transaction(rs485_channel_t *ch, uint32_t txn_id) { (void)ch;(void)txn_id; return 0; }
int rs485_channel_poll(rs485_channel_t *ch) { (void)ch; return 0; }
