#include <stdio.h>

#include "lexer.h"

/*
Игровая площадка для тестирования чего угодно
Пожалуйста, не удаляйте этот файл, он нам еще пригодится
Если хотите запустить этот файл:

make test-pg
*/

int main() {
    // printf("Hello, Lexer!\n");
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
    while (lexer->current_token->type != TOKEN_EOF) {
        get_next_token(lexer, file);
        printf("Token Type: %s, Data: %s, Line: %d\n",
               token_type_to_string(lexer->current_token->type),
               lexer->current_token->data,
               lexer->current_token->line);
        
    }

    fclose(file);
    lexer_free(lexer);
    return 0;
}