#include "isotp.h"
#include "mcp2515_core.h"
#include "mcp2515_types.h"
#include "bsp_blackpill.h"
#include <string.h>
#include <stdio.h>

ISOTP_TxCtx isotp_tx = {0};
ISOTP_RxCtx isotp_rx = {0};

/* ---------- internal helpers ---------- */
static uint8_t ISOTP_SendRawFrame(uint16_t id, uint8_t dlc, const uint8_t *data) {
    CAN_DATA_FRAME frame;
    frame.standard_id = id;
    frame.dlc = dlc;
    memset(frame.data, 0x00, 8);
    memcpy(frame.data, data, dlc);
    return CAN_SendPacket(&frame);
}

static uint8_t ISOTP_SendFlowControl(uint8_t fs, uint8_t block_size, uint8_t st_min) {
    uint8_t d[8] = {0};
    d[0] = ISOTP_PCI_FC | (fs & 0x0F);
    d[1] = block_size;
    d[2] = st_min;
    uint8_t ok = ISOTP_SendRawFrame(ISOTP_TX_ID, 8, d);
#if ISOTP_DEBUG_HANDSHAKE
    sprintf(buf, "[FC->] sending FC on ID=0x%03X fs=%X bs=%02X stmin=%02X -- queue %s\r\n",
            ISOTP_TX_ID, fs, block_size, st_min, ok ? "OK" : "FAILED (TXBs full)");
    UART_SendString(buf);
#endif
    return ok;
}

/* ---------- TX side ---------- */
uint8_t ISOTP_Send(const uint8_t *data, uint16_t len) {
    if (isotp_tx.state != ISOTP_TX_IDLE) return 0;
    if (len == 0 || len > ISOTP_MAX_PAYLOAD) return 0;

    memcpy(isotp_tx.buf, data, len);
    isotp_tx.len  = len;
    isotp_tx.sent = 0;
    isotp_tx.seq  = 1;

    if (len <= 7) {
        uint8_t d[8] = {0};
        d[0] = ISOTP_PCI_SF | (uint8_t)len;
        memcpy(&d[1], data, len);
        if (!ISOTP_SendRawFrame(ISOTP_TX_ID, 8, d)) return 0;
        isotp_tx.state = ISOTP_TX_IDLE;
        return 1;
    }

    uint8_t d[8] = {0};
    d[0] = ISOTP_PCI_FF | ((len >> 8) & 0x0F);
    d[1] = (uint8_t)(len & 0xFF);
    memcpy(&d[2], data, 6);
    if (!ISOTP_SendRawFrame(ISOTP_TX_ID, 8, d)) return 0;

    isotp_tx.sent = 6;
    isotp_tx.state = ISOTP_TX_WAIT_FC;
    isotp_tx.last_frame_tick = millis();
    return 1;
}

static void ISOTP_HandleFlowControl(const uint8_t *d) {
    uint8_t fs = d[0] & 0x0F;
#if ISOTP_DEBUG_HANDSHAKE
    sprintf(buf, "[<-FC] received FC: fs=%X bs=%02X stmin=%02X (tx state was %d)\r\n",
            fs, d[1], d[2], isotp_tx.state);
    UART_SendString(buf);
#endif
    if (fs == ISOTP_FC_OVFLW) {
        isotp_tx.state = ISOTP_TX_IDLE;
        return;
    }
    if (fs == ISOTP_FC_WAIT) {
        isotp_tx.last_frame_tick = millis();
        return;
    }
    isotp_tx.block_size = d[1];
    isotp_tx.st_min_ms  = (d[2] <= 0x7F) ? d[2] : 0;
    isotp_tx.frames_in_block = 0;
    isotp_tx.state = ISOTP_TX_SENDING_CF;
    isotp_tx.last_frame_tick = millis();
}

void ISOTP_Poll(void) {
    if (isotp_rx.state == ISOTP_RX_RECEIVING &&
        (uint32_t)(millis() - isotp_rx.last_frame_tick) > ISOTP_TIMEOUT_MS) {
        isotp_rx.state = ISOTP_RX_IDLE;
    }

    if (isotp_tx.state == ISOTP_TX_WAIT_FC &&
        (uint32_t)(millis() - isotp_tx.last_frame_tick) > ISOTP_TIMEOUT_MS) {
        isotp_tx.state = ISOTP_TX_IDLE;
        return;
    }

    if (isotp_tx.state != ISOTP_TX_SENDING_CF) return;

    if ((uint32_t)(millis() - isotp_tx.last_frame_tick) < isotp_tx.st_min_ms) return;

    uint16_t remaining = isotp_tx.len - isotp_tx.sent;
    uint8_t chunk = (remaining > 7) ? 7 : (uint8_t)remaining;
    uint8_t d[8] = {0};
    d[0] = ISOTP_PCI_CF | (isotp_tx.seq & 0x0F);
    memcpy(&d[1], &isotp_tx.buf[isotp_tx.sent], chunk);

    uint8_t cf_ok = ISOTP_SendRawFrame(ISOTP_TX_ID, 8, d);
#if ISOTP_DEBUG_HANDSHAKE
    sprintf(buf, "[CF->] seq=%X chunk=%d ID=0x%03X -- queue %s\r\n",
            isotp_tx.seq, chunk, ISOTP_TX_ID, cf_ok ? "OK" : "FAILED (TXBs full)");
    UART_SendString(buf);
#endif
    if (!cf_ok) return; /* all TXBs full -- retry next poll instead of losing the chunk */

    isotp_tx.sent += chunk;
    isotp_tx.seq = (isotp_tx.seq + 1) & 0x0F;
    if (isotp_tx.seq == 0) isotp_tx.seq = 1;
    isotp_tx.frames_in_block++;
    isotp_tx.last_frame_tick = millis();

    if (isotp_tx.sent >= isotp_tx.len) {
        isotp_tx.state = ISOTP_TX_IDLE;
        return;
    }
    if (isotp_tx.block_size != 0 && isotp_tx.frames_in_block >= isotp_tx.block_size) {
        isotp_tx.state = ISOTP_TX_WAIT_FC;
        isotp_tx.last_frame_tick = millis();
    }
}

/* ---------- RX side ---------- */
void ISOTP_ProcessRxFrame(uint16_t rx_id, uint8_t dlc, const uint8_t *d) {
    if (rx_id != ISOTP_RX_ID) {
#if ISOTP_DEBUG_HANDSHAKE
        sprintf(buf, "[ID?] frame ID=0x%03X ignored (this board's ISOTP_RX_ID=0x%03X)\r\n",
                rx_id, ISOTP_RX_ID);
        UART_SendString(buf);
#endif
        return;
    }
    (void)dlc; /* classical CAN ISO-TP frames are always padded to 8 */

    uint8_t pci_type = d[0] & 0xF0;
    switch (pci_type) {
        case ISOTP_PCI_SF: {
            uint8_t sf_len = d[0] & 0x0F;
            if (sf_len == 0 || sf_len > 7) return;
            memcpy(isotp_rx.buf, &d[1], sf_len);
            isotp_rx.len = sf_len;
            isotp_rx.received = sf_len;
            isotp_rx.state = ISOTP_RX_IDLE;
            isotp_rx.complete = 1;
            break;
        }
        case ISOTP_PCI_FF: {
            uint16_t total_len = ((d[0] & 0x0F) << 8) | d[1];
            if (total_len > ISOTP_MAX_PAYLOAD) return;
#if ISOTP_DEBUG_HANDSHAKE
            sprintf(buf, "[FF<-] accepted, ID=0x%03X total_len=%d\r\n", rx_id, total_len);
            UART_SendString(buf);
#endif
            memcpy(isotp_rx.buf, &d[2], 6);
            isotp_rx.len = total_len;
            isotp_rx.received = 6;
            isotp_rx.expected_seq = 1;
            isotp_rx.state = ISOTP_RX_RECEIVING;
            isotp_rx.complete = 0;
            isotp_rx.last_frame_tick = millis();
            /* STmin=0x00: with CAN_RX_VERBOSE_DEBUG off, per-frame processing
             * is microseconds, so back-to-back CFs don't overflow RXB0 -- no
             * artificial pacing needed. Raise this (e.g. 0x0A) if you re-enable
             * verbose debug for troubleshooting, or overflow/drop will return. */
            ISOTP_SendFlowControl(ISOTP_FC_CTS, 0x00, 0x00);
            break;
        }
        case ISOTP_PCI_CF: {
            uint8_t seq = d[0] & 0x0F;
#if ISOTP_DEBUG_HANDSHAKE
            sprintf(buf, "[CF<-] seq=%X expected=%X rx_state=%d\r\n", seq, isotp_rx.expected_seq, isotp_rx.state);
            UART_SendString(buf);
#endif
            if (isotp_rx.state != ISOTP_RX_RECEIVING) {
#if ISOTP_DEBUG_HANDSHAKE
                UART_SendString("[CF<-] DROPPED -- not in RECEIVING state\r\n");
#endif
                return;
            }
            if (seq != isotp_rx.expected_seq) {
#if ISOTP_DEBUG_HANDSHAKE
                sprintf(buf, "[CF<-] SEQ MISMATCH -- aborting reassembly (got %X, wanted %X)\r\n", seq, isotp_rx.expected_seq);
                UART_SendString(buf);
#endif
                isotp_rx.state = ISOTP_RX_IDLE;
                return;
            }

            uint16_t remaining = isotp_rx.len - isotp_rx.received;
            uint8_t chunk = (remaining > 7) ? 7 : (uint8_t)remaining;
            memcpy(&isotp_rx.buf[isotp_rx.received], &d[1], chunk);
            isotp_rx.received += chunk;
            isotp_rx.expected_seq = (isotp_rx.expected_seq + 1) & 0x0F;
            if (isotp_rx.expected_seq == 0) isotp_rx.expected_seq = 1;
            isotp_rx.last_frame_tick = millis();
            if (isotp_rx.received >= isotp_rx.len) {
                isotp_rx.state = ISOTP_RX_IDLE;
                isotp_rx.complete = 1;
            }
            break;
        }
        case ISOTP_PCI_FC: {
            ISOTP_HandleFlowControl(d);
            break;
        }
        default:
            break;
    }
}

uint8_t ISOTP_RxAvailable(void) {
    return isotp_rx.complete;
}

uint16_t ISOTP_RxRead(uint8_t *out, uint16_t max_len) {
    if (!isotp_rx.complete) return 0;
    uint16_t n = (isotp_rx.len < max_len) ? isotp_rx.len : max_len;
    memcpy(out, isotp_rx.buf, n);
    isotp_rx.complete = 0;
    return n;
}
