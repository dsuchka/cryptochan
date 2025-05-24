#include "common.h"

void assure_error_desc_empty(char **error_desc)
{
    // assure that error_desc does not hold any valid ptr
    if (*error_desc != NULL) {
        fprintf(stderr, "PANIC: error_desc ptr = %p (expected NULL)\n", (void*) error_desc);
        abort();
    }
}

