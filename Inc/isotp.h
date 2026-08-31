#ifndef ISOTP_H
#define ISOTP_H

#include <stdint.h>
#include "board_config.h"

/* PCI (Protocol Control Info) nibble, top 4 bits of byte 0 */
#define ISOTP_PCI_SF 0x00  /* Single Frame */
#define ISOTP_PCI_FF 0x10  /* First Frame */
#define ISOTP_PCI_CF 0x20  /* Consecutive Frame */
#define ISOTP_PCI_FC 0x30  /* Flow Control */

/* Flow Control status (FS) nibble */
#define ISOTP_FC_CTS   0x0  /* Clear To Send */
#define ISOTP_FC_WAIT  0x1
#define ISOTP_FC_OVFLW 0x2

typedef enum {
    ISOTP_TX_IDLE = 0,
    ISOTP_TX_SENDING_CF,
    ISOTP_TX_WAIT_FC
} ISOTP_TxState;

typedef enum {
    ISOTP_RX_IDLE = 0,
    ISOTP_RX_RECEIVING
} ISOTP_RxState;

typedef struct {
    ISOTP_TxState state;
    uint8_t  buf[ISOTP_MAX_PAYLOAD];
    uint16_t len;
    uint16_t sent;
    uint8_t  seq;
    uint8_t  block_size;
    uint8_t  st_min_ms;
    uint8_t  frames_in_block;
    uint32_t last_frame_tick;
} ISOTP_TxCtx;

typedef struct {
    ISOTP_RxState state;
    uint8_t  buf[ISOTP_MAX_PAYLOAD];
    uint16_t len;
    uint16_t received;
    uint8_t  expected_seq;
    uint32_t last_frame_tick;
    volatile uint8_t complete; /* set to 1 when a full message has been reassembled */
} ISOTP_RxCtx;

extern ISOTP_TxCtx isotp_tx;
extern ISOTP_RxCtx isotp_rx;

/* Start sending a payload. Returns 0 if busy, too large, or the first
 * frame couldn't be queued (all TX buffers full -- try again shortly). */
uint8_t ISOTP_Send(const uint8_t *data, uint16_t len);

/* Call every main loop iteration -- paces Consecutive Frames per
 * STmin/BS, retries on TX-buffer-full, and times out stale transfers. */
void ISOTP_Poll(void);

/* Called from CAN_HandleRxInterrupt with the ID/DLC/data already parsed out. */
void ISOTP_ProcessRxFrame(uint16_t rx_id, uint8_t dlc, const uint8_t *d);

uint8_t  ISOTP_RxAvailable(void);
uint16_t ISOTP_RxRead(uint8_t *out, uint16_t max_len);

#endif /* ISOTP_H */
