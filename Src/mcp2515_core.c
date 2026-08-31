#include "mcp2515_core.h"
#include "mcp2515_regs.h"
#include "bsp_blackpill.h"
#include "stm32f4xx.h"
#include "mcp2515_isr.h"

void MCP2515_Reset(void) {
    GPIOA->ODR &= ~(1U << 4);
    SPI_Transfer(MCP_INS_RESET);
    GPIOA->ODR |= (1U << 4);
}

uint8_t MCP2515_Read(uint8_t address) {
    uint8_t dummy;
    GPIOA->ODR &= ~(1U << 4);
    SPI_Transfer(MCP_INS_READ);
    SPI_Transfer(address);
    dummy = SPI_Transfer(0x00);
    GPIOA->ODR |= (1U << 4);
    return dummy;
}

void MCP2515_Write(uint8_t address, uint8_t data) {
    GPIOA->ODR &= ~(1U << 4);
    SPI_Transfer(MCP_INS_WRITE);
    SPI_Transfer(address);
    SPI_Transfer(data);
    GPIOA->ODR |= (1U << 4);
}

void MCP2515_BitModify(uint8_t reg, uint8_t mask, uint8_t data) {
    GPIOA->ODR &= ~(1U << 4);
    SPI_Transfer(MCP_BITMOD);
    SPI_Transfer(reg);
    SPI_Transfer(mask);
    SPI_Transfer(data);
    GPIOA->ODR |= (1U << 4);
}

/* Switch REQOP mode and wait for CANSTAT to confirm the change. */
uint8_t MCP2515_SetMode(uint8_t mode) {
    MCP2515_BitModify(MCP_CANCTRL, 0xE0, (uint8_t)(mode << 5));
    uint32_t timeout = 100000;
    while (((MCP2515_Read(MCP_CANSTAT) >> 5) & 0x07) != mode) {
        if (--timeout == 0) return 0; /* mode never took -- check SPI wiring/CS/clock */
    }
    return 1;
}

uint8_t MCP2515_Init(uint8_t cnf1, uint8_t cnf2, uint8_t cnf3) {
    MCP2515_Reset();
    for (volatile uint32_t d = 0; d < 50000; d++); /* brief settle time after reset */

    if (!MCP2515_SetMode(MCP_MODE_CONFIG)) return 0;

    MCP2515_Write(MCP_CNF1, cnf1);
    MCP2515_Write(MCP_CNF2, cnf2);
    MCP2515_Write(MCP_CNF3, cnf3);

    /* RXB0CTRL: RXM=11 (accept all, ignore filters/masks), BUKT=1 (roll
     * into RXB1 instead of dropping on overflow -- extra buffering
     * headroom on multi-frame ISO-TP bursts). */
    MCP2515_Write(0x60, 0x64);
    /* RXB1CTRL: RXM=11 (accept all here too, for symmetry) */
    MCP2515_Write(0x70, 0x60);

    MCP2515_EnableRxInterrupt(); /* declared in mcp2515_isr.h */

    return MCP2515_SetMode(MCP_MODE_NORMAL);
}

void MCP2515_LoadTxBuffer(uint8_t buffer_num, CAN_DATA_FRAME *frame) {
    uint8_t instruction = 0x40 + (buffer_num * 2);
    uint8_t dlc = frame->dlc & 0x0F;
    if (dlc > 8) dlc = 8;

    GPIOA->ODR &= ~(1U << 4);
    SPI_Transfer(instruction);
    SPI_Transfer((uint8_t)(frame->standard_id >> 3));   /* SIDH */
    SPI_Transfer((uint8_t)(frame->standard_id << 5));   /* SIDL */
    SPI_Transfer(0x00);
    SPI_Transfer(0x00);
    SPI_Transfer(dlc);
    for (uint8_t i = 0; i < dlc; i++) {
        SPI_Transfer(frame->data[i]);
    }
    GPIOA->ODR |= (1U << 4);
}

void MCP2515_RequestToSend(uint8_t buffer_num) {
    uint8_t rts_bits = (1U << buffer_num);
    GPIOA->ODR &= ~(1U << 4);
    SPI_Transfer(MCP_INS_RTS | rts_bits);
    GPIOA->ODR |= (1U << 4);
}

uint8_t MCP2515_ReadStatus(void) {
    uint8_t status;
    GPIOA->ODR &= ~(1U << 4);
    SPI_Transfer(MCP_INS_READ_STATUS);
    status = SPI_Transfer(0x00);
    GPIOA->ODR |= (1U << 4);
    return status;
}

uint8_t check_rx_status(void) {
    uint8_t status;
    GPIOA->ODR &= ~(1U << 4);
    SPI_Transfer(MCP_INS_RX_STATUS);
    status = SPI_Transfer(0xFF);
    GPIOA->ODR |= (1U << 4);
    return status;
}

/* Read RX Buffer instruction (0x90 = start at RXB0SIDH, 0x94 = start at
 * RXB1SIDH). Auto-increments through SIDH,SIDL,EID8,EID0,DLC,D0..D7 in
 * one CS-low burst -- far fewer SPI transactions than reading each
 * register individually. */
void MCP2515_ReadRxBuffer(uint8_t buffer_num, uint16_t *id, uint8_t *dlc, uint8_t *data) {
    uint8_t instruction = 0x90 + (buffer_num * 4);
    GPIOA->ODR &= ~(1U << 4);
    SPI_Transfer(instruction);
    uint8_t sidh = SPI_Transfer(0x00);
    uint8_t sidl = SPI_Transfer(0x00);
    SPI_Transfer(0x00); /* EID8, unused (standard IDs only) */
    SPI_Transfer(0x00); /* EID0, unused */
    *dlc = SPI_Transfer(0x00) & 0x0F;
    for (uint8_t i = 0; i < 8; i++) {
        data[i] = SPI_Transfer(0x00);
    }
    GPIOA->ODR |= (1U << 4);
    *id = (sidh << 3) | (sidl >> 5);
}

uint8_t CAN_SendPacket(CAN_DATA_FRAME *frame) {
    uint8_t status = MCP2515_ReadStatus();
    if (!(status & (1U << 2))) {
        MCP2515_LoadTxBuffer(0, frame);
        MCP2515_RequestToSend(0);
        return 1;
    } else if (!(status & (1U << 4))) {
        MCP2515_LoadTxBuffer(1, frame);
        MCP2515_RequestToSend(1);
        return 1;
    } else if (!(status & (1U << 6))) {
        MCP2515_LoadTxBuffer(2, frame);
        MCP2515_RequestToSend(2);
        return 1;
    } else {
        return 0; /* all three TX buffers full -- caller decides whether/how to retry */
    }
}
