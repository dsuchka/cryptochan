#ifndef __CRYPTOCHAN_H
#define __CRYPTOCHAN_H

#ifndef CRYPTOCHAN_DEFAULT_CONFIG_FILE
#   define CRYPTOCHAN_DEFAULT_CONFIG_FILE "/etc/cryptochan/cryptochan.conf"
#endif

typedef enum __cryptochan_mode {
    NONE = 0,
    CLIENT,
    SERVER,
    KEYGEN,
} cryptochan_mode_t;

typedef struct __cryptochan_arguments {
    cryptochan_mode_t mode;
    char *config_file;
} cryptochan_arguments_t;

#endif // __CRYPTOCHAN_H
