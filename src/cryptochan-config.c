#include "cryptochan-config.h"
#include <libconfig.h>

void assure_error_desc_empty(char **error_desc)
{
    // assure that error_desc does not hold any valid ptr
    if (*error_desc != NULL) {
        fprintf(stderr, "PANIC: error_desc ptr = %p (expected NULL)\n", (void*) error_desc);
        abort();
    }
}

bool cryptochan_config_parse_sock_addr(
    config_setting_t *root_setting,
    const char *setting_name,
    cryptochan_config_sock_addr_t *cc_sa,
    char **error_desc
)
{
    config_setting_t *nest_setting;
    __attribute__((unused)) int asp_res;

    assure_error_desc_empty(error_desc);

    // lookup nested setting
    if (!(nest_setting = config_setting_lookup(root_setting, setting_name))) {
        asp_res = asprintf(error_desc, "missing `%s' setting", setting_name);
        return false;
    }

    // parse host
    if (!config_setting_lookup_string(nest_setting, "host", &(cc_sa->host))) {
        asp_res = asprintf(error_desc, "incomplete `%s' setting: missing `host'", setting_name);
        return false;
    }

    // parse port
    if (!config_setting_lookup_int(nest_setting, "port", &(cc_sa->port))) {
        if (!config_setting_lookup(nest_setting, "port")) {
            asp_res = asprintf(error_desc, "incomplete `%s' setting: missing `host'", setting_name);
        } else {
            asp_res = asprintf(error_desc, "incorrect `%s' setting: invalid `port'", setting_name);
        }
        return false;
    }

    // all done
    return true;
}

bool cryptochan_config_parse_client(
    config_setting_t *setting,
    cryptochan_config_client_t *cc_client,
    char **error_desc
)
{
    char *nest_error_desc = NULL;
    __attribute__((unused)) int asp_res;

    assure_error_desc_empty(error_desc);

    if (!cryptochan_config_parse_sock_addr(
            setting, "listen", &(cc_client->listen), &nest_error_desc)
        || !cryptochan_config_parse_sock_addr(
            setting, "target", &(cc_client->target), &nest_error_desc)
    ) {
        if (nest_error_desc != NULL) {
            asp_res = asprintf(error_desc, "bad `client' config: %s", nest_error_desc);
            free(nest_error_desc);
        }
        return false;
    }

    // all done
    return true;
}

bool cryptochan_config_parse_server(
    config_setting_t *setting,
    cryptochan_config_server_t *cc_server,
    char **error_desc
)
{
    char *nest_error_desc = NULL;
    __attribute__((unused)) int asp_res;

    assure_error_desc_empty(error_desc);

    if (!cryptochan_config_parse_sock_addr(
            setting, "listen", &(cc_server->listen), &nest_error_desc)
        || !cryptochan_config_parse_sock_addr(
            setting, "target", &(cc_server->target), &nest_error_desc)
    ) {
        if (nest_error_desc != NULL) {
            asp_res = asprintf(error_desc, "bad `server' config: %s", nest_error_desc);
            free(nest_error_desc);
        }
    }

    // all done
    return true;
}

bool cryptochan_config_load(cryptochan_config_t *cc_config, const char *config_filepath)
{
    bool result = false;
    char *error_desc = NULL;

    // config vars
    config_t config;
    config_setting_t *setting;

    // init the config struct
    config_init(&config);

    for (;;) {
        // try to load configuration file
        if (!config_read_file(&config, config_filepath)) {
            fprintf(stderr, "Error parsing config file `%s' at line %d: %s\n",
                config_filepath, config_error_line(&config), config_error_text(&config));
            break;
        }

        // parse config setting: private-key (mandatory)
        if (!config_lookup_string(&config, "private-key", &(cc_config->private_key))) {
            fprintf(stderr, "Missing private-key setting in config file `%s'\n",
                config_filepath);
            break;
        }

        // parse config setting: public-key (optional, must be )
        if (config_lookup_string(&config, "public-key", &(cc_config->public_key))) {
            // TODO
        }

        // parse client settings (if any)
        if ((setting = config_lookup(&config, "client")) != NULL) {
            cc_config->client.present = true;

            if (!cryptochan_config_parse_client(setting, &(cc_config->client), &error_desc)) {
                fprintf(stderr, "Failed to load config file: %s\n", error_desc);
                break;
            }
        }

        // parse server settings (if any)
        if ((setting = config_lookup(&config, "server")) != NULL) {
            cc_config->server.present = true;

            if (!cryptochan_config_parse_server(setting, &(cc_config->server), &error_desc)) {
                fprintf(stderr, "Failed to load config file: %s\n", error_desc);
                break;
            }
        }

        // all done
        result = true;
        break;
    }

    /* finalize */
    if (error_desc != NULL) free(error_desc);
    config_destroy(&config);
    return result;
}
