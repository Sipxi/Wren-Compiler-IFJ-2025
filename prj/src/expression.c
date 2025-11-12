#include "expression.h"
#include "stack.h"
#include "token.h"
#include <string.h>
#include <stdlib.h>

static int get_precedence(Token *token) {
    switch (token->type) {
        case TOKEN_MULTIPLY:      // *
        case TOKEN_DIVISION:      // /
            return 5; // P1 (высший)

        case TOKEN_PLUS:     // +
        case TOKEN_MINUS:    // -
            return 4; // P2

        case TOKEN_LESS:       // <
        case TOKEN_EQUAL_LESS:      // <=
        case TOKEN_GREATER:       // >
        case TOKEN_EQUAL_GREATER:      // >=
            return 3; // P3

        case TOKEN_KEYWORD:       // is
            if (strcmp(token->data, "is") == 0) {
                return 2; // P4
            }
            return 0; // Не оператор

        case TOKEN_EQUAL:       // ==
        case TOKEN_NOT_EQUAL:      // !=
            return 1; // P5 (низший)

        default:
            return 0; // Не оператор
    }
}