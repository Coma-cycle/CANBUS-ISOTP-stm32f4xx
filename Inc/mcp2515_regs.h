#ifndef MCP2515_REGS_H
#define MCP2515_REGS_H

/* Guarded with #ifndef throughout so this won't collide if you already
 * have some of these defined elsewhere (e.g. a vendor mcp2515.h). */

/* ===================== MCP2515 SPI instruction set ===================== */
#define MCP_INS_RESET       0xC0
#define MCP_INS_READ        0x03
#define MCP_INS_WRITE       0x02
#define MCP_INS_RTS         0x80   /* OR with (1<<bufnum) */
#define MCP_INS_READ_STATUS 0xA0
#define MCP_INS_RX_STATUS   0xB0
#ifndef MCP_BITMOD
#define MCP_BITMOD 0x05
#endif

/* ===================== Control / status registers ===================== */
#ifndef MCP_CANSTAT
#define MCP_CANSTAT  0x0E
#endif
#ifndef MCP_CANCTRL
#define MCP_CANCTRL  0x0F
#endif
#ifndef MCP_CNF3
#define MCP_CNF3     0x28
#endif
#ifndef MCP_CNF2
#define MCP_CNF2     0x29
#endif
#ifndef MCP_CNF1
#define MCP_CNF1     0x2A
#endif
#ifndef MCP_CANINTE
#define MCP_CANINTE  0x2B
#endif
#ifndef MCP_CANINTF
#define MCP_CANINTF  0x2C
#endif
#ifndef MCP_EFLG
#define MCP_EFLG     0x2D
#endif
#define MCP_TEC  0x1C
#define MCP_REC  0x1D

/* CANINTF bits */
#ifndef CANINTF_RX0IF
#define CANINTF_RX0IF 0x01
#endif
#define CANINTF_RX1IF 0x02

/* EFLG bits (receive overflow) */
#ifndef EFLG_RX0OVR
#define EFLG_RX0OVR 0x40
#endif
#ifndef EFLG_RX1OVR
#define EFLG_RX1OVR 0x80
#endif

/* REQOP / OPMOD values, go in bits [7:5] of CANCTRL / CANSTAT */
#define MCP_MODE_NORMAL      0x00
#define MCP_MODE_SLEEP       0x01
#define MCP_MODE_LOOPBACK    0x02
#define MCP_MODE_LISTENONLY  0x03
#define MCP_MODE_CONFIG      0x04

#endif /* MCP2515_REGS_H */
