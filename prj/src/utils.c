#include "utils.h"
#include <string.h>
#include <stdlib.h>

char *strdup_c99(const char *s) {
    size_t size = strlen(s) + 1;
    char *p = malloc(size);
    if (p) {
        memcpy(p, s, size);
    }
    return p;
}
