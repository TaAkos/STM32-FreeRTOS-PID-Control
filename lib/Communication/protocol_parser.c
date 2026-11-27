#include "protocol_parser.h"

#include <string.h>



static void ProtocolParser_Reset(protocol_parser_t *parser);



bool ProtocolParser_Init(protocol_parser_t *parser){

    if (parser == NULL)
    {
        return false;
    }

    parser->state = PARSER_WAIT_SOF1;
    parser->index = 0U;
    parser->expected_length = 0U;

    memset(parser->buffer, 0, sizeof(parser->buffer));

    parser->last_byte_time_ms = 0U;

    parser->invalid_length_count = 0U;
    parser->decode_error_count = 0U;
    parser->timeout_count = 0U;

    return true;
}



bool ProtocolParser_ProcessByte(protocol_parser_t *parser, uint8_t byte, uint32_t current_time_ms, protocol_frame_t *frame_ready){

    if (parser == NULL || frame_ready == NULL)
    {
        return false;
    }

    if (parser->state != PARSER_WAIT_SOF1)
    {
        if ((current_time_ms - parser->last_byte_time_ms) >= PROTOCOL_PARSER_TIMEOUT_MS)
        {
            parser->timeout_count++;
            ProtocolParser_Reset(parser);
        }
        
    }
    

    switch (parser->state)
    {

    case PARSER_WAIT_SOF1:
        
        if (byte == PROTOCOL_SOF1)
        {
            parser->buffer[0] = byte;
            parser->index = 1U;
            parser->state = PARSER_WAIT_SOF2;
        }
        
        break;

    case PARSER_WAIT_SOF2:

        if (byte == PROTOCOL_SOF2)
        {
            parser->buffer[1] = byte;
            parser->index = 2U;
            parser->state = PARSER_READ_HEADER;
        } else if (byte == PROTOCOL_SOF1){

            parser->buffer[0] = byte;
            parser->index = 1U;
            parser->state = PARSER_WAIT_SOF2;
        } else{
            
            ProtocolParser_Reset(parser);
        }

        break;

    case PARSER_READ_HEADER:

        parser->buffer[parser->index] = byte;
        parser->index ++; 
        
        
        if (parser->index == PROTOCOL_SOF_SIZE + PROTOCOL_HEADER_SIZE)
        {
            uint16_t payload_length = parser->buffer[6];

            if (payload_length > PROTOCOL_MAX_PAYLOAD)
            {
                parser->invalid_length_count++;
                ProtocolParser_Reset(parser);
                break;
            }

            parser->expected_length = PROTOCOL_SOF_SIZE + PROTOCOL_HEADER_SIZE + payload_length + PROTOCOL_CRC_SIZE;
            

            if (payload_length > 0U)
            {
                parser->state = PARSER_READ_PAYLOAD;
            } else {
                parser->state = PARSER_READ_CRC;
            }
            
        }
        
        break;

    case PARSER_READ_PAYLOAD:
    {

        uint16_t crc_offset = parser->expected_length - PROTOCOL_CRC_SIZE;

        parser->buffer[parser->index] = byte;
        parser->index++;

        if (parser->index == crc_offset)
        {
            parser->state = PARSER_READ_CRC;
        }
        break;
    }

    case PARSER_READ_CRC:
    {
    parser->buffer[parser->index] = byte;
    parser->index++;

        if (parser->index == parser->expected_length)
        {
            bool frame_valid = Protocol_DecodeFrame(parser->buffer, parser->expected_length, frame_ready);
        
            if (!frame_valid)
            {
                parser->decode_error_count++;
            }
            
            ProtocolParser_Reset(parser);
            return frame_valid;
        
        }
        break;
    }

    default:
        ProtocolParser_Reset(parser);
        break;
    }

    if (parser->state != PARSER_WAIT_SOF1)
    {
        parser->last_byte_time_ms = current_time_ms;
    }
    
    return false;

}

static void ProtocolParser_Reset(protocol_parser_t *parser){
    
    parser->index = 0U;
    parser->expected_length = 0U;
    parser->state = PARSER_WAIT_SOF1;
}