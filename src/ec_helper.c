#include "ec_helper.h"

bool decode_b58_privkey(
    const char *encoded_key, uint8_t *private_key_data,
    secp256k1_pubkey *public_key_data, char **error_desc
)
{
    __attribute__((unused)) int asp_res;
    uint8_t buf[44], *dptr = buf;
    size_t dsz = sizeof(buf), sz;

    assure_error_desc_empty(error_desc);


    /* Parse the given base58 private (secret) key */

    // check size (44 digits max for 32 bytes)
    if ((sz = strlen(encoded_key)) > 44) {
        asp_res = asprintf(error_desc,
            "b58-encoded private key is too large (max 44 b58 digits), got: %s (%ld digits)",
            encoded_key, sz);
        return false;
    }

    // decode b58
    if (!b58tobin((void*) buf, &dsz, encoded_key, sz)) {
        asp_res = asprintf(error_desc, "wrong b58-encoded private key: bad base58");
        return false;
    }
    dptr += sizeof(buf) - dsz;

    // assure that size is correct
    if (dsz != 32) {
        asp_res = asprintf(error_desc,
            "b58-decoded private key has incorrect length: expected 32 bytes, got: %ld",
            dsz);
        return false;
    }

    // copy buffer to dest
    memcpy(private_key_data, dptr, dsz);


    /* Verify the decoded secp256k1 secret key */

    // alloc a secp256k1 context object
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    bool result = false;

    for (;;) {
        if (!secp256k1_ec_seckey_verify(ctx, private_key_data)) {
            asp_res = asprintf(error_desc,
                "incorrect b58-decoded private key: bad secp256k1 key");
            break;
        }

        // write its public key data if ptr is not NULL
        if (public_key_data) {
            if (!secp256k1_ec_pubkey_create(ctx, public_key_data, private_key_data)) {
                asp_res = asprintf(error_desc,
                    "failed to create corresponding public key");
                break;
            }
        }

        // all done
        result = true;
        break;
    }

    // destroy the secp256k1 context object
    secp256k1_context_destroy(ctx);

    // return result
    return result;
}


bool decode_b58_pubkey(const char *encoded_key, secp256k1_pubkey *public_key_data, char **error_desc)
{
    __attribute__((unused)) int asp_res;
    uint8_t buf[46], *dptr = buf;
    size_t dsz = sizeof(buf), sz;

    assure_error_desc_empty(error_desc);


    /* Parse the given base58 private (secret) key */

    // check size (46 digits max for 33 bytes)
    if ((sz = strlen(encoded_key)) > 46) {
        asp_res = asprintf(error_desc,
            "b58-encoded public key is too large (max 46 b58 digits), got: %s (%ld digits)",
            encoded_key, sz);
        return false;
    }

    // decode b58
    if (!b58tobin((void*) dptr, &dsz, encoded_key, sz)) {
        asp_res = asprintf(error_desc, "wrong b58-encoded public key: bad base58");
        return false;
    }
    dptr += sizeof(buf) - dsz;

    // assure that size is correct
    if (dsz != 33) {
        asp_res = asprintf(error_desc,
            "b58-decoded public key has incorrect length: expected 32 bytes, got: %ld",
            dsz);
        return false;
    }

    // check compressed prefix (02 or 03 for Y parity)
    if ((dptr[0] != 0x02) && (dptr[0] != 0x03)) {
        asp_res = asprintf(error_desc,
            "incorrect b58-decoded public key: prefix byte must be 0x02 or 0x03, got: 0x%02x",
            dptr[0]);
        return false;
    }


    /* Deserialize the decoded public key data as secp256k1_pubkey */

    // alloc a secp256k1 context object
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    bool result = false;

    for (;;) {
        if (!secp256k1_ec_pubkey_parse(ctx, public_key_data, dptr, dsz)) {
            asp_res = asprintf(error_desc,
                "incorrect b58-decoded public key: bad secp256k1 pubkey");
            break;
        }

        // all done
        result = true;
        break;
    }

    // destroy the secp256k1 context object
    secp256k1_context_destroy(ctx);

    // return result
    return result;
}


bool privkey_to_pubkey(uint8_t *private_key_data, secp256k1_pubkey *public_key_data)
{
    // alloc a secp256k1 context object
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    // verify private key and compare public keys
    bool result = secp256k1_ec_seckey_verify(ctx, private_key_data)
        && secp256k1_ec_pubkey_create(ctx, public_key_data, private_key_data);

    // destroy the secp256k1 context object
    secp256k1_context_destroy(ctx);

    // return result
    return result;
}


bool check_pubkey_belongs_to_privkey(uint8_t *private_key_data, secp256k1_pubkey *public_key_data)
{
    secp256k1_pubkey orig_pubkey;

    return privkey_to_pubkey(private_key_data, &orig_pubkey)
        && (memcmp(public_key_data, &orig_pubkey, sizeof(secp256k1_pubkey)) == 0);
}


char* b58enc_data(uint8_t *data, size_t sz)
{
    char buf[sz*2];
    size_t tsz = sizeof(buf);

    if (!b58enc(buf, &tsz, data, sz)) {
        return NULL;
    }

    char *txt = malloc(tsz + 1);
    if (txt != NULL) {
        strcpy(txt, buf);
    }

    return txt;
}

char* privkey_to_b58enc_form(uint8_t *private_key_data)
{
    return b58enc_data(private_key_data, 32);
}

char* pubkey_to_b58enc_form(secp256k1_pubkey *public_key_data)
{
    char *result = NULL;
    uint8_t compressed_pubkey[33];
    size_t len = sizeof(compressed_pubkey);

    // alloc a secp256k1 context object
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    // serialize public key to compressed form
    if (secp256k1_ec_pubkey_serialize(
            ctx, compressed_pubkey, &len, public_key_data, SECP256K1_EC_COMPRESSED)
        && (len == sizeof(compressed_pubkey))
    ) {
        result = b58enc_data(compressed_pubkey, 33);
    }

    // destroy the secp256k1 context object
    secp256k1_context_destroy(ctx);

    // return result
    return result;
}

