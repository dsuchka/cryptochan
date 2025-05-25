#ifndef __EC_HELPER_H
#define __EC_HELPER_H

#include "common.h"

#include <secp256k1.h>

extern bool decode_b58_privkey(
    const char *encoded_key, uint8_t *private_key_data,
    secp256k1_pubkey *public_key_data, char **error_desc
);
extern bool decode_b58_pubkey(
    const char *encoded_key, secp256k1_pubkey *public_key_data,
    char **error_desc
);
extern bool check_pubkey_belongs_to_privkey(
    uint8_t *private_key_data, secp256k1_pubkey *public_key_data
);
extern bool privkey_to_pubkey(
    uint8_t *private_key_data, secp256k1_pubkey *public_key_data
);
extern char* privkey_to_b58enc_form(uint8_t *private_key_data);
extern char* pubkey_to_b58enc_form(secp256k1_pubkey *public_key_data);

#endif // __EC_HELPER_H
