#include <stdio.h>

#include "lexer.h"
/*
Основной файл который будет запускать сам проект пока что
Если хотите запустить этот файл:

make run
*/

int main() {
    //TODO - нужно изменить с файла на stdin
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
    scan_token(lexer, file);

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