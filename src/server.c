#include "common.h"
#include "server.h"
#include "session.h"
#include "dispatcher.h"

int run_server(cryptochan_config_t *config)
{
    cryptochan_dispatcher_context_t cntx;

    // init context
    if (!dispatcher_init(&cntx, &(config->server.listen))) {
        fprintf(stderr, "Could not init dispatcher. Exiting.\n");
        return EXIT_FAILURE;
    }

    // destroy context
    dispatcher_destroy_context(&cntx);

    // all done
    return EXIT_SUCCESS;
}
