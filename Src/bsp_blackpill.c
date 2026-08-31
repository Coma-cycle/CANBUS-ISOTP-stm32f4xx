#include "bsp_blackpill.h"
#include "board_config.h"

char buf[128];
static volatile uint32_t g_ms_ticks = 0;

/* ---------------------------------------------------------------------
 * SysTick millisecond tick -- required by ISO-TP for STmin pacing and
 * timeouts. Call SysTick_Init() once at startup, after SystemInit().
 * ------------------------------------------------------------------- */
void SysTick_Init(void) {
    SysTick_Config(CPU_CLOCK_HZ / 1000U);
}

void SysTick_Handler(void) {
    g_ms_ticks++;
}

uint32_t millis(void) {
    return g_ms_ticks;
}

void SPI_INIT_BLACK_PILL(void) { /* MASTER controls the clock !!!!!!!!! */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    /* PA5=SCK, PA6=MISO, PA7=MOSI (AF5), PA4=CS as GPIO output */
    GPIOA->MODER &= ~((3U << (4*2))|(3U << (5*2))|(3U << (6*2))|(3U << (7*2)));
    GPIOA->MODER |= ((1U << (4*2))|(2U << (5*2))|(2U << (6*2))|(2U << (7*2)));
    GPIOA->AFR[0] &= ~((0xFU << (5*4))|(0xFU << (6*4))|(0xFU << (7*4)));
    GPIOA->AFR[0] |= (5U << (5*4))|(5U << (6*4))|(5U << (7*4));
    GPIOA->ODR |= (1U << 4); /* CS idle high */

    SPI1->CR1 = 0;
    SPI1->CR1 |= (1U << 2)|(1U << 9)|(1U << 8); /* MSTR, SSM, SSI, BR=000 (~8MHz) */
    SPI1->CR1 |= (1U << 6); /* enable */
}

uint8_t SPI_Transfer(uint8_t byte) {
    uint32_t timeout = 0xFFFF;
    while (!(SPI1->SR & (1U << 1))) {
        if (--timeout == 0) return 0xFF;
    }
    SPI1->DR = byte;
    timeout = 0xFFFF;
    while (!(SPI1->SR & (1U << 0))) {
        if (--timeout == 0) return 0xFF;
    }
    return (uint8_t)SPI1->DR;
}

/* PA2=TX, PA3=RX, 115200 8N1 */
void UART2_Init(void) {
    RCC->AHB1ENR |= (1U << 0);
    RCC->APB1ENR |= (1U << 17);
    GPIOA->MODER &= ~((3U << (2*2)) | (3U << (3*2)));
    GPIOA->MODER |=  ((2U << (2*2)) | (2U << (3*2)));
    GPIOA->AFR[0] &= ~((0xFU << (4*2)) | (0xFU << (4*3)));
    GPIOA->AFR[0] |=  ((0x7U << (4*2)) | (0x7U << (4*3)));
    USART2->BRR = 0x082; /* ~115200 @ 16MHz APB1 -- recompute if your APB1 clock differs */
    USART2->CR1 = (1U << 3) | (1U << 2) | (1U << 13);
}

void UART_SendChar(char c) {
    while (!(USART2->SR & (1U << 7)));
    USART2->DR = (uint8_t)c;
}

void UART_SendString(const char *str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}
