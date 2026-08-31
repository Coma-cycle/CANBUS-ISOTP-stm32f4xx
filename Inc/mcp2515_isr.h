#ifndef MCP2515_ISR_H
#define MCP2515_ISR_H

#include <stdint.h>

/* Set nonzero by EXTI0_IRQHandler when the MCP2515 INT line fires.
 * Your main loop can poll this to decide whether to call
 * CAN_HandleRxInterrupt(), instead of doing SPI traffic from ISR context. */
extern volatile uint8_t can_rx_flag;

/* Optional hook fired for EVERY raw frame the MCP2515 hands us, before
 * any ISO-TP ID filtering happens. NULL by default -- set it yourself if
 * you want to probe "is a frame physically arriving at all" separately
 * from "does the ISO-TP layer accept it" (e.g. from a scope-trigger GPIO
 * toggle in your own test code). */
extern void (*g_can_raw_rx_hook)(void);

/* Enable MCP2515 to assert its INT pin on RX0 AND RX1 buffer full. */
void MCP2515_EnableRxInterrupt(void);

/* Configure PB0 as EXTI0 input, falling edge (MCP2515 INT is active-low,
 * open-drain). Call once at startup after MCP2515_Init(). */
void EXTI0_INT_Init(void);

/* Drains whichever RX buffer(s) triggered the interrupt, feeds each frame
 * into ISOTP_ProcessRxFrame(), and clears the relevant flags. Call this
 * from your main loop when can_rx_flag is set (clear the flag yourself). */
void CAN_HandleRxInterrupt(void);

#endif /* MCP2515_ISR_H */
