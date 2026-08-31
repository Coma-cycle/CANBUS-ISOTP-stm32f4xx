#include "mcp2515_can.h"
#include <string.h>
 * board_config.h on THIS board must have: ISOTP_TX_ID = 0x7E0, ISOTP_RX_ID = 0x7E8 */

int main(void) {
    SysTick_Init();
    SPI_INIT_BLACK_PILL();
    UART2_Init();

    /* cnf1/cnf2/cnf3: bit-timing values for your MCP2515 crystal + target
     * bitrate. These are placeholders -- compute real values for your
     * oscillator (e.g. 8MHz) and bitrate (e.g. 500kbps) with the MCP2515
     * datasheet's bit-timing tables or an online calculator. */
    if (!MCP2515_Init(0x00, 0x90, 0x02)) {
        UART_SendString("MCP2515 init FAILED -- check SPI wiring/CS/clock\r\n");
        while (1);
    }
    EXTI0_INT_Init();
    UART_SendString("TX board up.\r\n");

    uint32_t last_send = 0;
//    const char *msg = "hello from transmitter, this is a multi-frame test payload";
    const char *msg = "hello from transmitter,";
    while (1) {
        /* drain any pending RX work */
        if (can_rx_flag) {
            can_rx_flag = 0;
            CAN_HandleRxInterrupt();
        }
        ISOTP_Poll();

        /* send the test payload every 2s, only when idle */
        if ((uint32_t)(millis() - last_send) > 2000) {
            if (ISOTP_Send((const uint8_t *)msg, (uint16_t)(strlen(msg) + 1))) {
                UART_SendString("[TX] queued payload\r\n");
            } else {
                UART_SendString("[TX] send busy/failed, will retry\r\n");
            }
            last_send = millis();
        }

        /* print anything that comes back */
        if (ISOTP_RxAvailable()) {
            uint8_t rx_buf[256];
            uint16_t n = ISOTP_RxRead(rx_buf, sizeof(rx_buf) - 1);
            rx_buf[n] = '\0';
            UART_SendString("[RX] got: ");
            UART_SendString((char *)rx_buf);
            UART_SendString("\r\n");
        }
    }
}
