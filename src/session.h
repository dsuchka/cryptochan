#ifndef __SESSION_H
#define __SESSION_H

#include "cyclic-buffer.h"

typedef enum __cryptochan_session_client_state {
    CSCS_CONNECT_TO_SERVER = 0,
    CSCS_SEND_ENTROPY_CLIENT_PART,
    CSCS_WAIT_ENTROPY_SERVER_PART,
    CSCS_SEND_ECDH_FINGERPRINT,
    CSCS_SEND_ECDH_SHARED_SECRET_CLIENT_PART,
    CSCS_WAIT_ECDH_SHARED_SECRET_SERVER_PART,
    CSCS_SEND_ECDH_SHARED_SECRET_HASH_CLIENT_SIGNATURE,
    CSCS_WAIT_ECDH_SHARED_SECRET_HASH_SERVER_SIGNATURE,
    CSCS_CHANNELLING,
} cryptochan_session_client_state_t;

typedef enum __cryptochan_session_server_state {
    CSSS_SEND_ENTROPY = 0,
    CSSS_WAIT_ENTROPY_CLIENT_PART,
    CSSS_WAIT_ECDH_FINGERPRINT,
    CSSS_DETECT_CLIENT,
    CSSS_SEND_ECDH_SHARED_SECRET_SERVER_PART,
    CSSS_WAIT_ECDH_SHARED_SECRET_CLIENT_PART,
    CSSS_SEND_ECDH_SHARED_SECRET_HASH_SERVER_SIGNATURE,
    CSSS_WAIT_ECDH_SHARED_SECRET_HASH_CLIENT_SIGNATURE,
    CSSS_CHANNELLING,
} cryptochan_session_server_state_t;

typedef struct __cryptochan_session {
    uint8_t domestic_entropy[64];       // 16 for ECDH FP, 3 x 16 for ECDH SS
    uint8_t foreign_entropy[64];        // 16 for ECDH FP, 3 x 16 for ECDH SS
    uint8_t shared_secret[64];
    cyclic_buffer_t input_buffer;
    cyclic_buffer_t decode_buffer;
    cyclic_buffer_t output_buffer;
    cyclic_buffer_t encode_buffer;
    int state;
} cryptochan_session_t;

#endif // __SESSION_H
