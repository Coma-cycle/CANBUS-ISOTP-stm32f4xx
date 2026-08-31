#include "mcp2515_isr.h"
#include "mcp2515_core.h"
#include "mcp2515_regs.h"
#include "bsp_blackpill.h"
#include "board_config.h"
#include "isotp.h"
#include "stm32f4xx.h"
#include <stdio.h>

volatile uint8_t can_rx_flag = 0;
void (*g_can_raw_rx_hook)(void) = 0; /* NULL by default -- see mcp2515_isr.h */

void CAN_HandleRxInterrupt(void) {
    uint8_t intf = MCP2515_Read(MCP_CANINTF);

    if (intf & CANINTF_RX0IF) {
        uint8_t eflg = MCP2515_Read(MCP_EFLG);
#if CAN_RX_VERBOSE_DEBUG
        uint8_t tec = MCP2515_Read(MCP_TEC);
        uint8_t rec = MCP2515_Read(MCP_REC);
        sprintf(buf, "INTF=%02X EFLG=%02X TEC=%02X REC=%02X\r\n", intf, eflg, tec, rec);
        UART_SendString(buf);
#endif
        uint16_t real_id;
        uint8_t dlc;
        uint8_t data[8];
        MCP2515_ReadRxBuffer(0, &real_id, &dlc, data); /* single burst read */

        if (g_can_raw_rx_hook) g_can_raw_rx_hook();

#if CAN_RX_VERBOSE_DEBUG
        sprintf(buf, "RX0 ID=%03X DLC=%d D0=%02X D1=%02X D2=%02X D3=%02X D4=%02X D5=%02X D6=%02X D7=%02X\r\n",
                real_id, dlc, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
        UART_SendString(buf);
#endif
        ISOTP_ProcessRxFrame(real_id, dlc, data);

        if (eflg & (EFLG_RX0OVR | EFLG_RX1OVR)) {
            MCP2515_BitModify(MCP_EFLG, EFLG_RX0OVR | EFLG_RX1OVR, 0x00);
#if CAN_RX_VERBOSE_DEBUG
            UART_SendString("Cleared RX overflow\r\n");
#endif
        }
        MCP2515_BitModify(MCP_CANINTF, CANINTF_RX0IF, 0x00);
    }

    /* Second RX buffer -- with BUKT rollover enabled in MCP2515_Init(), a
     * frame that arrives while RXB0 is still occupied lands here instead
     * of being dropped as an overflow. Must be drained the same way. */
    if (intf & CANINTF_RX1IF) {
        uint16_t real_id;
        uint8_t dlc;
        uint8_t data[8];
        MCP2515_ReadRxBuffer(1, &real_id, &dlc, data);

#if CAN_RX_VERBOSE_DEBUG
        sprintf(buf, "RX1 ID=%03X DLC=%d D0=%02X D1=%02X D2=%02X D3=%02X D4=%02X D5=%02X D6=%02X D7=%02X (rolled over from RXB0)\r\n",
                real_id, dlc, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
        UART_SendString(buf);
#endif
        ISOTP_ProcessRxFrame(real_id, dlc, data);
        MCP2515_BitModify(MCP_CANINTF, CANINTF_RX1IF, 0x00);
    }
}

void MCP2515_EnableRxInterrupt(void) {
    MCP2515_Write(MCP_CANINTE, 0x03); /* RX0IE + RX1IE */
}

void EXTI0_INT_Init(void) {
    RCC->AHB1ENR |= (1U << 1);    /* GPIOB clock */
    RCC->APB2ENR |= (1U << 14);   /* SYSCFG clock */
    GPIOB->MODER &= ~(3U << (0*2));   /* PB0 input mode */
    GPIOB->PUPDR &= ~(3U << (0*2));
    GPIOB->PUPDR |=  (1U << (0*2));   /* pull-up (idle high, INT pulls low) */
    SYSCFG->EXTICR[0] &= ~(0xF << 0);
    SYSCFG->EXTICR[0] |=  (0x1 << 0); /* EXTI0 source = port B */
    EXTI->IMR  |= (1U << 0);   /* unmask line 0 */
    EXTI->FTSR |= (1U << 0);   /* trigger on falling edge */
    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* Called from ISR context (kept minimal) -- just set the flag */
void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1U << 0)) {
        EXTI->PR |= (1U << 0);   /* clear pending bit (write 1 to clear) */
        can_rx_flag = 1;
    }
}
