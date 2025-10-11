#include "token.h"

Token *token_init() {
    // Выделить память для структуры Token 
    Token *token = (Token *)malloc(sizeof(Token));
    if (token == NULL) {
        return NULL;
    }
            
    // Выделить память для поля данных токена
    // Начальное выделение для 1 символа + нулевой терминатор
    //? @Sipxi Нужно ли добавлять память для нулевого терминатора?
    //? @Sipxi Это строка, а не символ
    token->data = (char *)malloc(sizeof(char) + 1);
    if (token->data == NULL) {
        free(token);
        return NULL;
    }
    // Инициализировать тип и номер строки значениями по умолчанию
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

char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_NULL: return "TOKEN_NULL";
        case TOKEN_STRING: return "TOKEN_STRING";
        case TOKEN_MULTI_STRING: return "TOKEN_MULTI_STRING";
        case TOKEN_INT: return "TOKEN_INT";
        case TOKEN_FLOAT: return "TOKEN_FLOAT";
        case TOKEN_EXP: return "TOKEN_EXP";
        case TOKEN_HEX: return "TOKEN_HEX";
        case TOKEN_GLOBAL_IDENTIFIER: return "TOKEN_GLOBAL_IDENTIFIER";
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_KEYWORD: return "TOKEN_KEYWORD";
        case TOKEN_DOT: return "TOKEN_DOT";
        case TOKEN_COMMA: return "TOKEN_COMMA";
        case TOKEN_WHITESPACE: return "TOKEN_WHITESPACE";
        case TOKEN_EOF: return "TOKEN_EOF";
        case TOKEN_EOL: return "TOKEN_EOL";
        case TOKEN_ASSIGN: return "TOKEN_ASSIGN";
        case TOKEN_OPEN_PAREN: return "TOKEN_OPEN_PAREN";
        case TOKEN_CLOSE_PAREN: return "TOKEN_CLOSE_PAREN";
        case TOKEN_OPEN_BRACE: return "TOKEN_OPEN_BRACE";
        case TOKEN_CLOSE_BRACE: return "TOKEN_CLOSE_BRACE";
        case TOKEN_PLUS: return "TOKEN_PLUS";
        case TOKEN_MINUS: return "TOKEN_MINUS";
        case TOKEN_MULTIPLY: return "TOKEN_MULTIPLY";
        case TOKEN_EQUAL: return "TOKEN_EQUAL";
        case TOKEN_NOT_EQUAL: return "TOKEN_NOT_EQUAL";
        case TOKEN_GREATER: return "TOKEN_GREATER";
        case TOKEN_EQUAL_GREATER: return "TOKEN_EQUAL_GREATER";
        case TOKEN_LESS: return "TOKEN_LESS";
        case TOKEN_EQUAL_LESS: return "TOKEN_EQUAL_LESS";
        case TOKEN_DIVISION: return "TOKEN_DIVISION";
        default: return "UNKNOWN_TOKEN_TYPE";
    }
}