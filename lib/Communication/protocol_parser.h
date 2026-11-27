#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H


#include "protocol.h"


#include <stdint.h>
#include <stdbool.h>

#define PROTOCOL_PARSER_TIMEOUT_MS 100U

typedef enum
{
    PARSER_WAIT_SOF1,
    PARSER_WAIT_SOF2,
    PARSER_READ_HEADER,
    PARSER_READ_PAYLOAD,
    PARSER_READ_CRC
} protocol_parser_state_t;

typedef struct
{
    protocol_parser_state_t state;
    uint8_t buffer[PROTOCOL_MAX_FRAME_SIZE];
    uint16_t index;
    uint16_t expected_length;

    uint32_t last_byte_time_ms;

    uint32_t invalid_length_count;
    uint32_t decode_error_count;
    uint32_t timeout_count;
} protocol_parser_t;

bool ProtocolParser_Init(protocol_parser_t *parser);
bool ProtocolParser_ProcessByte(protocol_parser_t *parser, uint8_t byte, uint32_t current_time_ms, protocol_frame_t *frame_ready);



#endif