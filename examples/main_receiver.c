#include "mcp2515_can.h"

/* Receiver board.
 * board_config.h on THIS board must have: ISOTP_TX_ID = 0x7E8, ISOTP_RX_ID = 0x7E0
 * (swapped relative to the transmitter board) */

int main(void) {
    SysTick_Init();
    SPI_INIT_BLACK_PILL();
    UART2_Init();

    /* Must match the transmitter's crystal + bitrate exactly. */
    if (!MCP2515_Init(0x00, 0x90, 0x02)) {
        UART_SendString("MCP2515 init FAILED -- check SPI wiring/CS/clock\r\n");
        while (1);
    }
    EXTI0_INT_Init();
    UART_SendString("RX board up, waiting...\r\n");

    while (1) {
        if (can_rx_flag) {
            can_rx_flag = 0;
            CAN_HandleRxInterrupt();  /* feeds ISOTP_ProcessRxFrame() internally */
        }
        ISOTP_Poll();  /* still needed here: paces/sends this board's own Flow Control frames */

        if (ISOTP_RxAvailable()) {
            uint8_t rx_buf[256];
            uint16_t n = ISOTP_RxRead(rx_buf, sizeof(rx_buf) - 1);
            rx_buf[n] = '\0';
            UART_SendString("[RX] got: ");
            UART_SendString((char *)rx_buf);
            UART_SendString("\r\n");

            /* echo it straight back, to sanity-check the round trip */
            ISOTP_Send(rx_buf, n);
        }
    }
}
