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

static bool is_term(Token* token) {
    switch (token->type) {
        case TOKEN_IDENTIFIER:
        case TOKEN_GLOBAL_IDENTIFIER:
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_EXP:
        case TOKEN_HEX:
        case TOKEN_STRING:
        case TOKEN_NULL:
            return true;
        default:
            return false;
    }
}



bool reduce_expression(Stack* op_stack, Stack* val_stack) {
    
  
    
    Token *op_token = Stack_Pop(op_stack);
    if (op_token == NULL) {
        return false;
    }

    Token *right = Stack_Pop(val_stack);
    if (right == NULL) {
        free(op_token); // Возвращаем оператор обратно
        return false;
    }

    Token *left = Stack_Pop(val_stack);
    if (left == NULL) {
        free(op_token); // Возвращаем оператор обратно
        free(right); // Возвращаем правый операнд обратно
        return false;
    }

    Token *placeholder = token_init();
    placeholder->type = TOKEN_IDENTIFIER;
    placeholder->data = NULL; // Значение не важно, т.к. это просто заполнитель

    Stack_Push(val_stack, placeholder);

    free(op_token);
    free(left);
    free(right);

    return true;
}

bool parser_expression(Lexer *lexer, FILE *file) {
    Stack op_stack;
    Stack val_stack;
    Stack_Init(&op_stack);
    Stack_Init(&val_stack);

    int paren_depth = 0;
    Token current = peek_token(lexer, file);

    while (current.type != TOKEN_EOF && current.type != TOKEN_EOL && paren_depth >= 0) {
        if (is_term(&current)) {
            Token tok = get_token(lexer, file);       // получаем токен
            Stack_Push(&val_stack, &tok);          // кладём в стек
        }
        else if (current.type == TOKEN_OPEN_PAREN) {
            paren_depth++;
            Token tok = get_token(lexer, file);
            Stack_Push(&op_stack, &tok);
        }
        else if (current.type == TOKEN_CLOSE_PAREN) {
            paren_depth--;
            Token tok = get_token(lexer, file); // съели ')'
            if (tok.data != NULL) {
                free(tok.data);
            }

            if (paren_depth < 0) break; // лишняя закрывающая скобка

            // Свернуть стек операторов до ближайшей '('
            while (!Stack_IsEmpty(&op_stack)) {
                Token *top = Stack_Top(&op_stack);
                if (top->type == TOKEN_OPEN_PAREN) {
                    Stack_Pop(&op_stack);
                    token_free(top);
                    break;
                }
                reduce_expression(&op_stack, &val_stack);
            }
        }
        else {
            Token tok = get_token(lexer, file);


            // Пока верхний оператор в стеке имеет **больший или равный приоритет**
            while (!Stack_IsEmpty(&op_stack)) {
                Token *top = Stack_Pop(&op_stack);
                if (get_precedence(top) >= get_precedence(&tok)) {
                    reduce_expression(&op_stack, &val_stack);
                } else break;
            }

            Stack_Push(&op_stack, &tok);
        }

        current = peek_token(lexer, file);
    }
    // Свернуть оставшиеся операторы
    while (!Stack_IsEmpty(&op_stack) && paren_depth >= 0) {
        reduce_expression(&op_stack, &val_stack);
    }
    bool success = (!Stack_IsEmpty(&val_stack) && paren_depth == 0);
    while (!Stack_IsEmpty(&val_stack)) {
        Token *t = Stack_Pop(&val_stack);
        free(t);
    }
    while (!Stack_IsEmpty(&op_stack)) {
        Token *t = Stack_Pop(&op_stack);
        free(t);
    }
    

    
        
    
    
    Stack_Dispose(&op_stack);
    Stack_Dispose(&val_stack);

    return success;
}