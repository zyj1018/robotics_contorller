/* 协议 CRC 校验 — CRC8 (Header) + CRC16-CCITT (Payload) */
#ifndef PROTOCOL_CRC_H
#define PROTOCOL_CRC_H
#include <stdint.h>
#include <stddef.h>

/* CRC-8 (多项式 0x07, 用于 ATM/HDLC) */
uint8_t crc8_compute(const uint8_t *data, size_t length);

/* CRC-16-CCITT (多项式 0x1021) */
uint16_t crc16_compute(const uint8_t *data, size_t length);

/* 验证 Header CRC8 (计算 Header 的 0..29 字节 比对 header_crc8) */
int crc8_verify_header(const uint8_t *header_buffer);

/* 计算并填入 Header CRC8 */
void crc8_fill_header(uint8_t *header_buffer);

#endif /* PROTOCOL_CRC_H */
