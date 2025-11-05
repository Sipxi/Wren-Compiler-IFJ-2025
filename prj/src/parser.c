#include "parser.h"
#include <stdio.h>
#include <string.h>




void parser_prolog(Lexer *lexer, FILE *file) {
    if (peek_token(lexer, file).type == TOKEN_EOF) {
        printf("Empty file.\n");
        return;
    }
    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(lexer->current_token->data, "import") != 0) {
        printf("Expected 'import' keyword at the beginning.");
    }
    printf("Found 'import' keyword.\n");
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_STRING ||
        strcmp(lexer->current_token->data, "ifj25") != 0) {
        printf("Expected string literal after 'import' keyword.");
    }
    printf("Found import string literal: %s\n", lexer->current_token->data);
    get_token(lexer, file);
}

void parser_run() {
    FILE *file = fopen("example.wren", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return;
    }

    Lexer *lexer = lexer_init();
    if (lexer == NULL) {
        fprintf(stderr, "Error initializing lexer.\n");
        fclose(file);
        return;
    }


    parser_prolog(lexer, file);


    lexer_free(lexer);
    if (fclose(file) != 0) { // обработка ошибки закрытия файла
        fprintf(stderr, "Error closing file.\n");
    }
    
    return;
}