#ifndef MCP2515_CAN_H
#define MCP2515_CAN_H

/* ===========================================================================
 * mcp2515_can -- single umbrella header.
 *
 * Add every .c file under Src/ to your build, put Inc/ on your include
 * path, and #include this one header from your application code:
 *
 *     #include "mcp2515_can.h"
 *
 * That's the whole integration step -- see README.md for the full
 * step-by-step and a minimal main.c example.
 * ==========================================================================*/

#include "board_config.h"   /* <-- edit this per project/per node */
#include "mcp2515_regs.h"
#include "mcp2515_types.h"
#include "bsp_blackpill.h"
#include "mcp2515_core.h"
#include "mcp2515_isr.h"
#include "isotp.h"

#endif /* MCP2515_CAN_H */
