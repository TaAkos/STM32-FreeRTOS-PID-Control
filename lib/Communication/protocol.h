#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#define PROTOCOL_SOF1 0xAAU
#define PROTOCOL_SOF2 0x55U
#define PROTOCOL_VERSION 0x01U
#define PROTOCOL_MAX_PAYLOAD 32U
#define PROTOCOL_HEADER_SIZE 5U
#define PROTOCOL_CRC_SIZE 2U
#define PROTOCOL_SOF_SIZE 2U

#define PROTOCOL_MAX_FRAME_SIZE (PROTOCOL_SOF_SIZE + PROTOCOL_HEADER_SIZE + PROTOCOL_MAX_PAYLOAD + PROTOCOL_CRC_SIZE)
#define PROTOCOL_MIN_FRAME_SIZE (PROTOCOL_SOF_SIZE + PROTOCOL_HEADER_SIZE + PROTOCOL_CRC_SIZE)

#define CMD_GROUP_MASK 0xF0U
#define CMD_SUBCOMMAND_MASK 0x0FU



typedef enum
{
    REQUEST = 0x01,
    RESPONSE = 0x02,
    TELEMETRY = 0x03,
    EVENT = 0x04
} protocol_type_t;

typedef enum
{
    CMD_GROUP_SYSTEM        = 0x00U,
    CMD_GROUP_CONTROL       = 0x10U,
    CMD_GROUP_SETPOINT      = 0x20U,
    CMD_GROUP_DISTURBANCE   = 0x30U,
    CMD_GROUP_NOISE         = 0x40U
} command_group_t;


typedef enum
{
    SYSTEM_CMD_START        = 0x01U,
    SYSTEM_CMD_PAUSE        = 0x02U,
    SYSTEM_CMD_RESET        = 0x03U,
    SYSTEM_CMD_STATUS       = 0x04U,
    SYSTEM_CMD_GET_TIMING   = 0x05U
} system_command_t;


typedef enum
{
    CONTROL_CMD_SET_PID_PARAM       = 0x00U,
    CONTROL_CMD_SET_OUTPUT_LIMIT    = 0x01U
} control_command_t;


typedef enum
{
    SIGNAL_CMD_MANUAL   = 0x00U,
    SIGNAL_CMD_CONSTANT = 0x01U,
    SIGNAL_CMD_STEP     = 0x02U,
    SIGNAL_CMD_RAMP     = 0x03U,
    SIGNAL_CMD_SQUARE   = 0x04U,
    SIGNAL_CMD_SINE     = 0x05U,
} signal_command_t;


typedef enum
{
    NOISE_CMD_CONFIG = 0x00U
} noise_command_t;


typedef enum
{
    TELEMETRY_CONTROL = 0x01
} telemetry_id_t;


typedef enum
{
    PID_PARAM_KP = 0x01,
    PID_PARAM_TI = 0x02,
    PID_PARAM_TD = 0x03,
    PID_PARAM_TF = 0x04,
    PID_PARAM_TT = 0x05
} pid_parameter_id_t;


typedef enum
{
    PROTOCOL_STATUS_OK              = 0x00,
    PROTOCOL_STATUS_UNKNOWN_COMMAND = 0x01,
    PROTOCOL_STATUS_INVALID_LENGTH  = 0x02,
    PROTOCOL_STATUS_INVALID_VALUE   = 0x03,
    PROTOCOL_STATUS_INVALID_STATE   = 0x04,
    PROTOCOL_STATUS_INTERNAL_ERROR  = 0x05
} protocol_status_t;


typedef enum
{
    PROTOCOLSYSTEM_STATE_IDLE       = 0x00U,
    PROTOCOLSYSTEM_STATE_RUNNING    = 0x01U,
    PROTOCOLSYSTEM_STATE_PAUSED     = 0x02U,
    PROTOCOLSYSTEM_STATE_FAULT      = 0x03U
} protocol_system_state_t;


typedef struct
{
    uint8_t version;
    protocol_type_t type;
    uint8_t id;
    uint8_t sequence;
    uint8_t length;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];
}protocol_frame_t;


bool Protocol_WriteU16LE(uint8_t *buffer, uint16_t value);

bool Protocol_WriteU32LE(uint8_t *buffer, uint32_t value);

bool Protocol_WriteFloatLE(uint8_t *buffer, float value);

bool Protocol_ReadU16LE(const uint8_t *buffer, uint16_t *value);

bool Protocol_ReadU32LE(const uint8_t *buffer, uint32_t *value);

bool Protocol_ReadFloatLE(const uint8_t *buffer, float *value);

bool Protocol_CRC16_CCITT(const uint8_t *data, uint16_t length, uint16_t *crc_out);

bool Protocol_EncodeFrame(const protocol_frame_t *frame, uint8_t *buffer, uint16_t buffer_size, uint16_t *encoded_length);

bool Protocol_DecodeFrame(const uint8_t *buffer, uint16_t buffer_length, protocol_frame_t *frame);


#endif