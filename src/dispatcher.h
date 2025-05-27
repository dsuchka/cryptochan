#ifndef __DISPATCHER_H
#define __DISPATCHER_H

#include "cryptochan_config.h"

typedef struct __cryptochan_dispatcher_context {
    struct sockaddr *listen_socket_ptr;
    int listen_sockfd;

} cryptochan_dispatcher_context_t;


extern bool dispatcher_init(
    cryptochan_dispatcher_context_t *cntx,
    cryptochan_config_sock_addr_t *bind_conf
);

extern void dispatcher_destroy_context(
    cryptochan_dispatcher_context_t *cntx
);

#endif // __DISPATCHER_H
