#include "common.h"
#include "random.h"
#include "cryptochan.h"
#include "cryptochan_config.h"
#include "ec_helper.h"
#include "server.h"
#include "client.h"

#include <argp.h>
#include <secp256k1.h>

const char *argp_program_version = "cryptochan 1.0.0";
const char *argp_program_bug_address = "<dsuchka@gmail.com>";
static char doc[] = "Simple custom crypto-channel like stunnel/socat,"
    " but with no certs (no SSL/TLS), an Ellipic Curve and AES block"
    " ciphers are used instead."
    "\v"
    "Acceptable modes:\n"
    "  * server - run as server (listen for clients, redirect to apps)\n"
    "  * client - run as client (listen for apps, redirect to server)\n"
    "  * keygen - generate key pair (b58 encoded) and exit\n"
;
static char args_doc[] = "MODE";
static struct argp_option options[] = {
    { "config", 'C', "FILE", 0, "Path to the configuration file" },
    { "prng", 'P', 0, 0, "Use PRNG instead of /dev/urandom" },
    { 0 }
};


static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    cryptochan_arguments_t *arguments = state->input;
    switch (key) {
        case 'C': {
            arguments->config_file = arg;
            break;
        }
        case 'P': {
            set_use_prng(true);
            break;
        }
        case ARGP_KEY_ARG: {
            if (arguments->mode != NONE) {
                argp_error(state, "Unexpected option: %s\n", arg);
            } else if (!strcasecmp("client", arg)) {
                arguments->mode = CLIENT;
            } else if (!strcasecmp("server", arg)) {
                arguments->mode = SERVER;
            } else if (!strcasecmp("keygen", arg)) {
                arguments->mode = KEYGEN;
            } else {
                argp_error(state, "Unrecognized mode: %s\n", arg);
            }
        }
        case ARGP_KEY_END: {
            if (arguments->mode == NONE) {
                argp_error(state, "MODE is not specified.\n");
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

static struct argp argp = { options, parse_opt, args_doc, doc };

int keygen_display(FILE *file);

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

    /* Handle KEYGEN mode */
    if (arguments.mode == KEYGEN) {
        return keygen_display(stdout);
    }

    /* Load config file */
    if (!cryptochan_config_load(&cc_config, arguments.config_file)) {
        fprintf(stderr, "Could not load config: `%s'\nExiting.\n",
            arguments.config_file);
        return EXIT_FAILURE;
    }

    switch (arguments.mode) {
        case SERVER:
            if (!cc_config.server.present) {
                fprintf(stderr, "Cannot run as SERVER: config `%s': missing `server'.\n",
                    arguments.config_file);
                break;
            }
            return run_server(&cc_config);
        case CLIENT:
            if (!cc_config.client.present) {
                fprintf(stderr, "Cannot run as CLIENT: config `%s': missing `client'.\n",
                    arguments.config_file);
                break;
            }
            return run_client(&cc_config);
        default:
            fprintf(stderr, "Could not determine MODE. Aborting.\n");
            abort();
    }

    return EXIT_FAILURE;
}

int keygen_display(FILE *file)
{
    uint8_t private_key_data[32];
    secp256k1_pubkey public_key_data;
    char *private_key = NULL;
    char *public_key = NULL;

    // fill random
    if (fill_random(private_key_data, 32) != 32) {
        fprintf(stderr, "FATAL: failed to fill random private key data.\n");
        return EXIT_FAILURE;
    }

    // generate key pair (verify private key and get its public key)
    if (!privkey_to_pubkey(private_key_data, &public_key_data)) {
        private_key_data[0] >>= 1; // order overflow? shift and repeat
        if (!privkey_to_pubkey(private_key_data, &public_key_data)) {
            fprintf(stderr, "FATAL: could not generate key pair.\n");
            return EXIT_FAILURE;
        }
    }

    // encode to base58 and display
    if ((private_key = privkey_to_b58enc_form(private_key_data)) != NULL) {
        fprintf(file, "private-key = \"%s\";\n", private_key);
        free(private_key);
    } else {
        fprintf(stderr, "FATAL: could not encode private key.\n");
    }

    if ((public_key = pubkey_to_b58enc_form(&public_key_data)) != NULL) {
        fprintf(file, "public-key = \"%s\";\n", public_key);
        free(public_key);
    } else {
        fprintf(stderr, "FATAL: could not encode public key.\n");
    }

    return EXIT_SUCCESS;
}

