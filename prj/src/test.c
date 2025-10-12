#include <stdio.h>
#include "lexer.h"

/*
Игровая площадка для тестирования чего угодно
Пожалуйста, не удаляйте этот файл, он нам еще пригодится
Если хотите запустить этот файл:

make test-pg

*/

// Print token data safely, visualizing special characters
void print_token_data(const char *data) {
    for (const char *p = data; *p; p++) {
        switch (*p) {
            case '\n': printf("\\n"); break;
            case '\t': printf("\\t"); break;
            case '\r': printf("\\r"); break;
            default:   putchar(*p); break;
        }
    }
}

int main() {
    // Use stdin for input (supports redirection like: ./test < input.wren)
    FILE *file = fopen("example.wren", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return 1;
    }

    Lexer *lexer = lexer_init();
    if (!lexer) {
        fprintf(stderr, "Error initializing lexer.\n");
        fclose(file);
        return 1;
    }

    while (lexer->current_token->type != TOKEN_EOF) {
        get_next_token(lexer, file);

        printf("Token Type: %s, Data: ",
               token_type_to_string(lexer->current_token->type));
        
        print_token_data(lexer->current_token->data);

        printf(", Line: %d\n", lexer->current_token->line);
    }

    // Don't close stdin
    lexer_free(lexer);
    return 0;
}
