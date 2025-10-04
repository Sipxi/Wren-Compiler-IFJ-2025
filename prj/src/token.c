#include "token.h"


Token *token_init() {
    Token *token = (Token *)malloc(sizeof(Token));
    if (token == NULL) {
        return NULL; // Memory allocation failure
    }

    token->data = (char *)malloc(sizeof(char));
    if (token->data == NULL) {
        free(token);
        return NULL; // Memory allocation failure
    }
    token->type = TOKEN_NULL;
    token->line = 0; // Start at line 0
    return token;
}

void token_free(Token *token) {
    if (token == NULL) {
        return; // Error: NULL pointer
    }

    free(token->data);
    free(token);
    //hello
}