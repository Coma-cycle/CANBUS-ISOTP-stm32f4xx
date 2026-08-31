#ifndef MCP2515_CORE_H
#define MCP2515_CORE_H

#include <stdint.h>
#include "mcp2515_types.h"

void    MCP2515_Reset(void);
uint8_t MCP2515_Read(uint8_t address);
void    MCP2515_Write(uint8_t address, uint8_t data);
void    MCP2515_BitModify(uint8_t reg, uint8_t mask, uint8_t data);
uint8_t MCP2515_SetMode(uint8_t mode);

/* One-shot bring-up: reset, program bit timing, accept-all masks with
 * RXB0->RXB1 rollover (BUKT), enable RX0+RX1 interrupts, go to Normal mode. */
uint8_t MCP2515_Init(uint8_t cnf1, uint8_t cnf2, uint8_t cnf3);

void    MCP2515_LoadTxBuffer(uint8_t buffer_num, CAN_DATA_FRAME *frame);
void    MCP2515_RequestToSend(uint8_t buffer_num);
uint8_t MCP2515_ReadStatus(void);
uint8_t check_rx_status(void);
void    MCP2515_ReadRxBuffer(uint8_t buffer_num, uint16_t *id, uint8_t *dlc, uint8_t *data);

/* Queues a frame into the first free TX buffer. Returns 1 if queued,
 * 0 if all three TX buffers were full (frame dropped -- caller decides
 * whether/how to retry). */
uint8_t CAN_SendPacket(CAN_DATA_FRAME *frame);

#endif /* MCP2515_CORE_H */
