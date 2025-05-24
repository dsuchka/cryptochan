#ifndef __CRYPTOCHAN_CONFIG_H
#define __CRYPTOCHAN_CONFIG_H

#include "common.h"

typedef struct __cryptochan_config_sock_addr {
    const char* host;
    int port;
} cryptochan_config_sock_addr_t;

typedef struct __cryptochan_config_client {
    bool present;
    cryptochan_config_sock_addr_t listen;
    cryptochan_config_sock_addr_t target;
    const char *server_public_key;
    alignas(32) uint8_t server_public_key_data[64];
} cryptochan_config_client_t;

typedef struct __cryptochan_config_server_allowed_client {
    const char *name;
    const char *public_key;
    struct __cryptochan_config_server_allowed_client *next;
    alignas(32) uint8_t public_key_data[64];
} cryptochan_config_server_allowed_client_t;

typedef struct __cryptochan_config_server {
    bool present;
    cryptochan_config_sock_addr_t listen;
    cryptochan_config_sock_addr_t target;
    cryptochan_config_server_allowed_client_t *clients;
} cryptochan_config_server_t;

typedef struct __cryptochan_config {
    const char *private_key;
    const char *public_key;
    cryptochan_config_client_t client;
    cryptochan_config_server_t server;
    alignas(32) uint8_t private_key_data[32];
    alignas(32) uint8_t public_key_data[64];
} cryptochan_config_t;

extern bool cryptochan_config_load(cryptochan_config_t *cc_config, const char *config_filepath);

#endif
