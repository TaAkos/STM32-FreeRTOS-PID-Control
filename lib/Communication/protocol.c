#include "protocol.h"

#include <stddef.h>
#include <math.h>
#include <string.h>

_Static_assert(sizeof(float) == 4, "Protocol requires 32-bit float");



bool Protocol_WriteU16LE(uint8_t *buffer, uint16_t value){

    if (buffer == NULL)
    {
        return false;
    }

    buffer[0] = (uint8_t)(value & 0x00FFU);
    buffer[1] = (uint8_t)(value >> 8);
    
    return true;
}

bool Protocol_WriteU32LE(uint8_t *buffer, uint32_t value){

    if (buffer == NULL)
    {
        return false;
    }
    
    buffer[0] = (uint8_t)(value & 0xFFU);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFU);
    buffer[2] = (uint8_t)((value >> 16) & 0xFFU);
    buffer[3] = (uint8_t)((value >> 24) & 0xFFU);

    return true;
}

bool Protocol_WriteFloatLE(uint8_t *buffer, float value){

    if (buffer == NULL)
    {
        return false;
    }
    
    if (!isfinite(value))
    {
        return false;
    }

    uint32_t u;
    
    memcpy(&u, &value, sizeof(u));

    

    return Protocol_WriteU32LE(buffer, u);


}

bool Protocol_ReadU16LE(const uint8_t *buffer, uint16_t *value){

    if (buffer == NULL || value == NULL)
    {
        return false;
    }

    *value = (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
    

    return true;
}

bool Protocol_ReadU32LE(const uint8_t *buffer, uint32_t *value){

    if (buffer == NULL || value == NULL)
    {
        return false;
    }

    *value = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);

    return true;
}

bool Protocol_ReadFloatLE(const uint8_t *buffer, float *value){

    if (buffer == NULL || value == NULL)
    {
        return false;
    }

    uint32_t u;
    float f;

    if (!Protocol_ReadU32LE(buffer, &u))
    {
        return false;
    }


    memcpy(&f, &u, sizeof(f));

    if (!isfinite(f))
    {
        return false;
    }
    
    *value = f;

    return true;

}

bool Protocol_CRC16_CCITT(const uint8_t *data, uint16_t length, uint16_t *crc_out){

    if (data == NULL || crc_out == NULL)
    {
        return false;
    }

    uint16_t crc = 0xFFFFU;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);

        for (uint8_t bit = 0; bit < 8U; bit++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
               
            } else {
                crc = (uint16_t)(crc << 1);
            }
            
        }
    
    }

    *crc_out = crc;
    

    return true;   

}

bool Protocol_EncodeFrame(const protocol_frame_t *frame, uint8_t *buffer, uint16_t buffer_size, uint16_t *encoded_length){

    if (frame == NULL || buffer == NULL || encoded_length == NULL || buffer_size == 0)
    {
        return false;
    }

    if (frame->length > PROTOCOL_MAX_PAYLOAD)
    {
        return false;
    }
    

    uint16_t total_length = PROTOCOL_SOF_SIZE + PROTOCOL_HEADER_SIZE + frame->length + PROTOCOL_CRC_SIZE;

    if (total_length > buffer_size)
    {
        return false;
    }
    
    buffer[0] = PROTOCOL_SOF1;
    buffer[1] = PROTOCOL_SOF2;
    buffer[2] = frame->version;
    buffer[3] = (uint8_t)frame->type;
    buffer[4] = frame->id;
    buffer[5] = frame->sequence;
    buffer[6] = frame->length;
    
    uint16_t payload_offset = PROTOCOL_SOF_SIZE + PROTOCOL_HEADER_SIZE;

    memcpy(&buffer[payload_offset], frame->payload, frame->length);

    uint16_t crc;

    if (!Protocol_CRC16_CCITT(&buffer[PROTOCOL_SOF_SIZE], PROTOCOL_HEADER_SIZE + frame->length, &crc))
    {
        return false;
    }

    uint16_t crc_offset = payload_offset + frame->length;

    if (!Protocol_WriteU16LE(&buffer[crc_offset], crc))
    {
        return false;
    }
    
    *encoded_length = total_length;

    return true;


}

bool Protocol_DecodeFrame(const uint8_t *buffer, uint16_t buffer_length, protocol_frame_t *frame){

    if (buffer == NULL || frame == NULL)
    {
        return false;
    }

    if (buffer_length < PROTOCOL_MIN_FRAME_SIZE)
    {
        return false;
    }
    
    if (buffer[0] != PROTOCOL_SOF1 || buffer[1] != PROTOCOL_SOF2)
    {
        return false;
    }
    
    if (buffer[2] != PROTOCOL_VERSION)
    {
        return false;
    }

    uint16_t payload_length = buffer[6];

    if (payload_length > PROTOCOL_MAX_PAYLOAD)
    {
        return false;
    }
    
    uint16_t expected_length = PROTOCOL_SOF_SIZE + PROTOCOL_HEADER_SIZE + payload_length + PROTOCOL_CRC_SIZE;

    if (buffer_length != expected_length)
    {
        return false;
    }
    
    uint16_t crc_offset = PROTOCOL_SOF_SIZE + PROTOCOL_HEADER_SIZE + payload_length;

    uint16_t received_crc;
    uint16_t calculated_crc;

    if (!Protocol_ReadU16LE(&buffer[crc_offset], &received_crc))
    {
        return false;
    }
    
    if (!Protocol_CRC16_CCITT(&buffer[PROTOCOL_SOF_SIZE], PROTOCOL_HEADER_SIZE + payload_length, &calculated_crc))
    {
        return false;
    }

    if (received_crc != calculated_crc)
    {
        return false;
    }

    frame->version = buffer[2];
    frame->type = (protocol_type_t)buffer[3];
    frame->id = buffer[4];
    frame->sequence = buffer[5];
    frame->length = buffer[6];

    uint16_t payload_offset = PROTOCOL_SOF_SIZE + PROTOCOL_HEADER_SIZE;

    memcpy(frame->payload, &buffer[payload_offset], payload_length);
    
    return true;

}