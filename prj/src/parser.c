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
    
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_STRING ||
        strcmp(lexer->current_token->data, "\"ifj25\"") != 0) {
        printf("Expected string literal after 'import' keyword.\n");
    }

    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(lexer->current_token->data, "for") != 0) {
        printf("Expected 'for' keyword after import string literal.\n");
    }

    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(lexer->current_token->data, "Ifj") != 0) {
        printf("Expected identifier 'Ifj' after 'for' keyword.\n");
    }

    get_token(lexer, file);
}

void parser_kostra(Lexer *lexer, FILE *file) {

    // Здесь будет дальнейшая реализация парсинга основной структуры программы
    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(lexer->current_token->data, "class") != 0) {
        printf("Expected 'class' keyword to start class definition.\n"); 
    }

    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_IDENTIFIER ||
        strcmp(lexer->current_token->data, "Program") != 0) {
        printf("Expected 'Program' identifier after 'class' keyword.\n");
    }

    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_OPEN_BRACE) {
        printf("Expected '{' to open class body.\n");
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after class body.\n");
    }
    get_token(lexer, file);


    // если следующий токен не закрывающая фигурная скобка, то будет список с функциями


    if (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        printf("Expected '}' to close class body.\n");
    }
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
    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after import statement.\n");
    }
    get_token(lexer, file); // consume EOL
    parser_kostra(lexer, file);

    lexer_free(lexer);
    if (fclose(file) != 0) { // обработка ошибки закрытия файла
        fprintf(stderr, "Error closing file.\n");
    }
    
    return;
}