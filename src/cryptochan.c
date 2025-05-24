#include <argp.h>
#include "common.h"
#include "cryptochan.h"
#include "cryptochan-config.h"

const char *argp_program_version = "cryptochan 1.0.0";
const char *argp_program_bug_address = "<dsuchka@gmail.com>";
static char doc[] = "Simple custom crypto-channel like stunnel/socat,"
    " but with no certs (no SSL/TLS), an Ellipic Curve and AES block"
    " ciphers are used instead.";
static struct argp_option options[] = {
    { "config", 'C', "FILE", 0, "Path to the configuration file."},
    { "client", 'c', 0, 0, "Run as client."},
    { "server", 's', 0, 0, "Run as server."},
    { 0 }
};


void fail_ambiguous_mode(struct argp_state *state)
{
    argp_error(state, "Both options SERVER and CLIENT are specified, choose only one.\n");
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    cryptochan_arguments_t *arguments = state->input;
    switch (key) {
        case 'C': {
            arguments->config_file = arg;
            break;
        }
        case 'c': {
            if (arguments->mode == SERVER) fail_ambiguous_mode(state);
            arguments->mode = CLIENT;
            break;
        }
        case 's': {
            if (arguments->mode == CLIENT) fail_ambiguous_mode(state);
            arguments->mode = SERVER;
            break;
        }
        case ARGP_KEY_ARG: {
            argp_error(state, "Unrecognized option: %s\n", arg);
        }
        case ARGP_KEY_END: {
            if (arguments->mode == NONE) {
                argp_error(state, "No CLIENT nor SERVER mode is specified, choose one.\n");
            }
            if (arguments->config_file == NULL) {
                arguments->config_file = CRYPTOCHAN_DEFAULT_CONFIG_FILE;
            }
            break;
        }
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, /*args_doc*/ NULL, doc };

int main(int argc, char **argv)
{
    /* Declare structs  */
    cryptochan_arguments_t arguments;
    cryptochan_config_t cc_config;

    /* Empty declared structs */
    memset(&arguments, 0, sizeof(cryptochan_arguments_t));
    memset(&cc_config, 0, sizeof(cryptochan_config_t));

    /* Parse CLI arguments */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    /* Load config file */
    if (!cryptochan_config_load(&cc_config, arguments.config_file)) {
        fprintf(stderr, "Could not load config: `%s'\nExiting.\n",
            arguments.config_file);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

