#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>
#include <stdlib.h>
typedef enum {
    TOKEN_NULL,
    TOKEN_STRING,
    TOKEN_MULTI_STRING,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_EXP,
    TOKEN_HEX,
    TOKEN_GLOBAL_IDENTIFIER,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_WHITESPACE,
    TOKEN_EOF,
    TOKEN_EOL,
    TOKEN_OPERATOR,
    TOKEN_BRACKET,
    TOKEN_COMMENT
} TokenType;


typedef struct {
    TokenType type;
    char *data;
    int line;
} Token;

/**
 * Initializes a Token structure.
 * This functions allocates memory for the data field 
 * and sets the type and line number to default values.
 * 
 * @param token Pointer to the Token structure to initialize.
 * @return 0 on success, -1 on failure (e.g., memory allocation failure).
 */
Token *token_init(); 
/**
 * Frees the memory allocated for the data field of a Token structure.
 * 
 * @param token Pointer to the Token structure to free.
 */
void token_free(Token *token);

#endif