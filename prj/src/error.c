#include "error.h"


void raise_error(int error_code, int line, int position, const char *message) {
    fprintf(stderr, "\033[1;31mError code: %d\nLine: %d, Position: %d\n%s\033[0m\n", error_code, line, position, message);
    exit(error_code);
}