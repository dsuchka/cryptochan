#include "common.h"
#include "dispatcher.h"
#include "session.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>


bool setnonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        { return false; }
    if (fcntl(fd, F_SETFL, flags|O_NONBLOCK) == -1)
        { return false; }
    return true;
}

void dispatcher_destroy_context(cryptochan_dispatcher_context_t *cntx)
{
    // close listen socket (if any)
    if (cntx->listen_sockfd > 0)
        { close(cntx->listen_sockfd); }

    // release memory (if any)
    if (cntx->listen_socket_ptr)
        { free(cntx->listen_socket_ptr); }

    // erase any context data
    memset(cntx, 0, sizeof(cryptochan_dispatcher_context_t));
}


bool dispatcher_init(
    cryptochan_dispatcher_context_t *cntx,
    cryptochan_config_sock_addr_t *bind_conf
)
{
    struct addrinfo hints = {0}, *res;
    struct sockaddr_in *sa_ptr;

    // resolve config listen hostname (usually IP)
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // server

    int err = getaddrinfo(bind_conf->host, NULL, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "dispatcher: could not resolve bind host address `%s': %s.\n",
            bind_conf->host, gai_strerror(err));
        return false;
    }

    sa_ptr = malloc(sizeof(struct sockaddr_in));
    if (!sa_ptr) {
        perror("dispatcher: malloc");
        return false;
    }

    // configure server socket to bind
    sa_ptr->sin_family = AF_INET; // IPv4
    sa_ptr->sin_port = htons(bind_conf->port);
    sa_ptr->sin_addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr;

    // setup basic context
    memset(cntx, 0, sizeof(cryptochan_dispatcher_context_t));
    cntx->listen_socket_ptr = (struct sockaddr*) sa_ptr;

    bool result = false;

    // configure socket and do bind
    for (;;) {
        int reuseaddr = 1; // TRUE

        // create socket
        cntx->listen_sockfd = socket(sa_ptr->sin_family, SOCK_STREAM, 0);
        if (cntx->listen_sockfd == -1)
            { perror("dispatcher: socket"); break; }

        // set nonblocking mode
        if (!setnonblocking(cntx->listen_sockfd))
            { perror("dispatcher: setnonblocking"); break; }

        // set reuseaddr sock opt
        if (setsockopt(cntx->listen_sockfd,
                SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) != 0)
            { perror("dispatcher: setsockopt(SO_REUSEADDR)"); break; }

        // bind address
        if (bind(cntx->listen_sockfd, cntx->listen_socket_ptr,
                sizeof(struct sockaddr_in)) != 0)
            { perror("dispatcher: bind"); break; }

        // all done
        result = true;
        break;
    }

    // destroy allocated data on failure
    if (!result) {
        dispatcher_destroy_context(cntx);
    }

    // return result
    return result;
}
