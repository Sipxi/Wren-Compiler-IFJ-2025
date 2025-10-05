#include "token.h"

Token *token_init() {
    // Allocate memory for the Token structure 
    Token *token = (Token *)malloc(sizeof(Token));
    if (token == NULL) {
        return NULL;
    }
            
    // Allocate memory for the token data field
    // Initial allocation for 1 character + null terminator
    //? @Sipxi Do we need to add memory for null terminator?
    //? @Sipxi This is string, not char
    token->data = (char *)malloc(sizeof(char) + 1);
    if (token->data == NULL) {
        free(token);
        return NULL;
    }
    // Initialize type and line number to default values
    token->type = TOKEN_NULL;
    token->line = -1;
    return token;
}

void token_free(Token *token) {
    if (token == NULL) {
        return;
    }

    free(token->data);
    free(token);
}