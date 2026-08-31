# mcp2515_can

Bare-metal **MCP2515** (SPI CAN controller + TJA1050 transceiver) driver with a from-scratch **ISO-TP (ISO 15765-2)** transport layer for STM32F4-family MCUs (“Black Pill” boards).

No HAL, no Arduino wrapper — direct register access, one SPI peripheral + one interrupt line.

## Features

- Full MCP2515 register-level driver  
  (reset, mode switching, TX/RX buffers, status & bit-modify instructions)
- Interrupt-driven RX with RXB0→RXB1 BUKT rollover  
  (extra buffering so back-to-back frames are not silently dropped)
- ISO-TP (ISO 15765-2)  
  - Single Frame / First Frame / Consecutive Frames  
  - Flow Control (CTS / WAIT / OVFLW)  
  - STmin pacing & block-size handling  
  - Stale-transfer timeouts
- Minimal BSP layer (`SysTick` millis, SPI1, UART2) so the protocol code has zero direct register dependencies of its own

## Status

| Component                              | State          |
|----------------------------------------|----------------|
| SPI / register-level MCP2515 driver    | ✅ Working     |
| Interrupt-driven RX, TX via first-free | ✅ Working     |
| ISO-TP single-frame & multi-frame      | ✅ Tested      |
| Hardware mask/filter configuration     | 🚧 In progress (currently accept-all) |
| >2-node bus testing                    | ⏳ Planned     |

## Project layout

```
Inc/
  board_config.h     ← edit this per project / per node
                       (clock, ISO-TP IDs, timeouts, debug flags)
  mcp2515_regs.h     MCP2515 SPI instructions + register/bit definitions
  mcp2515_types.h    CAN_DATA_FRAME struct
  bsp_blackpill.h    SysTick / SPI1 / UART2 (swap for a different MCU)
  mcp2515_core.h     low-level MCP2515 driver
  mcp2515_isr.h      EXTI + CANINTF handling → feeds ISO-TP
  isotp.h            ISO-TP transport (Send / Poll / RxAvailable / RxRead)
  mcp2515_can.h      umbrella header — #include this from your application

Src/
  (matching .c files)
```

## Wiring (Black Pill – STM32F401 / F411)

| Signal          | Pin          |
|-----------------|--------------|
| SPI1 SCK        | PA5          |
| SPI1 MISO       | PA6          |
| SPI1 MOSI       | PA7          |
| MCP2515 CS      | PA4          |
| MCP2515 INT     | PB0 (EXTI0, active-low) |
| UART2 TX        | PA3          |
| UART2 RX        | PA2          |

## Integrating into your project

1. Copy `Inc/*.h` into your include path and `Src/*.c` into your build  
   (e.g. `Core/Inc` and `Core/Src` in STM32CubeIDE).

2. Open `Inc/board_config.h` and set:
   - `CPU_CLOCK_HZ` to your actual core clock
   - `ISOTP_TX_ID` / `ISOTP_RX_ID` for this node  
     (**swap TX/RX relative to the peer node**)

3. `#include "mcp2515_can.h"` from your `main.c`.

4. **Important – ISR names**  
   This library defines `SysTick_Handler` and `EXTI0_IRQHandler`.  
   Your `startup_stm32*.s` already provides weak aliases, so the linker will pick up the library versions automatically.  
   Do **not** define your own handlers with the same names.

## Minimal example

```c
#include "mcp2515_can.h"

int main(void) {
    SysTick_Init();
    SPI_INIT_BLACK_PILL();
    UART2_Init();

    /* CNF1/CNF2/CNF3: bit-timing for your crystal + target bitrate.
     * Example values below are placeholders — compute real ones for
     * your oscillator (e.g. 8 MHz) and bitrate (e.g. 500 kbit/s). */
    if (!MCP2515_Init(0x00, 0x90, 0x02)) {
        UART_SendString("MCP2515 init FAILED\r\n");
        while (1);
    }
    EXTI0_INT_Init();

    uint8_t msg[] = "hello over isotp";
    ISOTP_Send(msg, sizeof(msg));

    while (1) {
        if (can_rx_flag) {
            can_rx_flag = 0;
            CAN_HandleRxInterrupt();
        }
        ISOTP_Poll();

        if (ISOTP_RxAvailable()) {
            uint8_t rx_buf[64];
            uint16_t n = ISOTP_RxRead(rx_buf, sizeof(rx_buf));
            UART_SendString("got: ");
            UART_SendString((char *)rx_buf);
            UART_SendString("\r\n");
        }
    }
}
```

## Important notes

- **Standard (11-bit) IDs only** — extended ID fields are written as zero.
- **TX and RX IDs must be swapped** between the two nodes.  
  The CAN protocol (and ISO-TP) will reject traffic if both boards use the same ID pair.
- RX masks/filters are currently disabled (accept-all on both buffers).  
  Fine for a 2-node bench; real filtering will be added when a third node appears.
- `mcp2515_regs.h`, `mcp2515_types.h` and the ISO-TP context structs were reconstructed from usage.  
  Double-check register addresses against the MCP2515 datasheet if you encounter problems.

## Example applications

- `main_transmitter.c` — periodically sends a multi-frame test payload
- `main_receiver.c` — receives the payload and echoes it back

Both files expect the correct ID swap in `board_config.h`.

---

Happy CAN bus hacking!
