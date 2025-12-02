/**
 * @file error_codes.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Definice kódů chyb používaných v různých fázích překladu.
 *
 * @author
 *     - Serhij Čepil (253038)
 */

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

/*===== ANSI Kódy pro barevný výstup do terminálu =====*/
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_STYLE_BOLD    "\x1b[1m"
#define ANSI_STYLE_DIM     "\x1b[2m"


/*===== Kódy chyb =====*/
typedef enum {
    LEXER_ERROR = 1,
    SYNTAX_ERROR = 2,
    SEMANTIC_ERROR_UNDEFINED = 3,
    SEMANTIC_ERROR_REDECLARATION = 4,
    SEMANTIC_ERROR_ARGUMENT = 5,      
    SEMANTIC_ERROR_TYPE_MISMATCH = 6,
    SEMANTIC_ERROR_OTHER = 10,
    RUNTIME_ERROR_INVALID_TYPE = 25,
    RUNTIME_ERROR_TYPE_MISMATCH = 26,
    INTERNAL_ERROR = 99
} ErrorCode;

#endif 
