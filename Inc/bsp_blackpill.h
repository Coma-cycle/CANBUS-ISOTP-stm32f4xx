#ifndef BSP_BLACKPILL_H
#define BSP_BLACKPILL_H

#include <stdint.h>
#include "stm32f4xx.h"   /* adjust to your MCU family's CMSIS header if not F4 */

/* ---- millisecond tick ---- */
void SysTick_Init(void);
uint32_t millis(void);

/* ---- SPI1 bus, wired to the MCP2515 (PA4=CS, PA5=SCK, PA6=MISO, PA7=MOSI) ---- */
void SPI_INIT_BLACK_PILL(void);
uint8_t SPI_Transfer(uint8_t byte);

/* ---- UART2 debug console (PA2=TX, PA3=RX, 115200-8N1) ---- */
void UART2_Init(void);
void UART_SendChar(char c);
void UART_SendString(const char *str);

/* Shared scratch buffer used by the debug sprintf() calls throughout the lib. */
extern char buf[128];

#endif /* BSP_BLACKPILL_H */
