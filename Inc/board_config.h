#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* ===================== Edit these per project / per node ===================== */

/* Core clock actually running after your SystemInit()/PLL config.
 * SysTick_Init() derives its 1ms tick from this -- if it's wrong, every
 * ISO-TP timing value derived from millis() will be wrong too.
 * Default here is 16MHz because this project runs off the default HSI
 * with no PLL configured -- change it if you enable the PLL. */
#define CPU_CLOCK_HZ  16000000UL

/* ISO-TP addressing: each node needs a TX id and RX id, swapped relative
 * to its peer. This is the TRANSMITTER/main board's config -- for the
 * RECEIVER_BOARD variant, swap these two values (0x7E8 / 0x7E0). */
#define ISOTP_TX_ID   0x7E0   /* ID we transmit on (request) */
#define ISOTP_RX_ID   0x7E8   /* ID we listen for (response) */

/* Max reassembled ISO-TP payload this node will accept (bytes). 4095 is
 * the ISO-TP spec maximum (12-bit length field in the First Frame) --
 * trim it down (e.g. 256) if RAM is tight, since isotp_tx and isotp_rx
 * each hold a buffer of this size (so it costs RAM twice over). */
#define ISOTP_MAX_PAYLOAD 4095

/* ISO-TP N_Bs / N_Cr style timeout: how long to wait for a Flow Control
 * or a Consecutive Frame before abandoning a transfer, in ms. */
#define ISOTP_TIMEOUT_MS 1000

/* ===================== Debug switches -- 0 in production ===================== */
#define CAN_RX_VERBOSE_DEBUG   0
#define ISOTP_DEBUG_HANDSHAKE  0

#endif /* BOARD_CONFIG_H */
