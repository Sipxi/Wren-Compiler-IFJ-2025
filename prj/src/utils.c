/**
 * @file utils.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Hlavní zdrojový soubor pro pomocné funkce používané v různých částech projektu.
 *
 * @author
 *     - Serhij Čepil (253038)
 */

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
