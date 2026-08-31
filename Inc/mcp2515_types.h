#ifndef MCP2515_TYPES_H
#define MCP2515_TYPES_H

#include <stdint.h>

/* A single classical CAN frame, standard (11-bit) ID only. */
typedef struct {
    uint16_t standard_id; /* 11 bits used */
    uint8_t  dlc;          /* 0-8 */
    uint8_t  data[8];
} CAN_DATA_FRAME;

#endif /* MCP2515_TYPES_H */
