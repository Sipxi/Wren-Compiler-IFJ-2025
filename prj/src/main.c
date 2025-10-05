#include <stdio.h>

#include "lexer.h"

int main() {

    FILE *file = fopen("example.wren", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return 1;
    }

    Lexer *lexer = lexer_init();


    if (lexer == NULL) {
        fprintf(stderr, "Error initializing lexer.\n");
        fclose(file);
        return 1;
    }
    // Example usage: get tokens until EOF
    get_next_token(lexer, file);

    if (lexer->current_token->type != TOKEN_NULL) {
        printf("Token Type: %d, Data: %s, Line: %d\n",
               lexer->current_token->type, lexer->current_token->data,
               lexer->current_token->line);
    } else {
        printf(
            "\033[1;31mLexical error.\nError code: 1\nUnexpected character at "
            "line %d, position %d\033[0m\n",
            lexer->line, lexer->position);
    }

    fclose(file);
    lexer_free(lexer);
    return 0;
}