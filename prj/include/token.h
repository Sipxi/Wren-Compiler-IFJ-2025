#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>
#include <stdlib.h>

/*===== Token Structure and Functions =====*/

/* Enumeration of possible token types */
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

/**
 * Structure representing a token in the source code.
 *
 * Fields:
 * - type: The type of the token (from the TokenType enumeration).
 * - data: Pointer to the string data of the token.
 * - line: The line number where the token was found.
 */
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
 * @return Pointer for the initialized Token structure.
 */
Token *token_init();

/**
 * Frees the memory allocated for the data field of a Token structure.
 *
 * @param token Pointer to the Token structure to free.
 */
void token_free(Token *token);

#endif
