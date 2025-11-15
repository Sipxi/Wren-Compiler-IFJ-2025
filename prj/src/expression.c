#include "expression.h"
#include "stack_token.h"
#include "token.h"
#include <string.h>
#include <stdlib.h>

static int get_precedence(Token token) {
    switch (token.type) {
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
            if (strcmp(token.data, "is") == 0) {
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

bool is_term(Token token) {
    switch (token.type) {
        case TOKEN_IDENTIFIER:
        case TOKEN_GLOBAL_IDENTIFIER:
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_EXP:
        case TOKEN_HEX:
        case TOKEN_STRING:
            return true;
        case TOKEN_KEYWORD:
            if (token.data == NULL) return false;
            if (strcmp(token.data, "Num") == 0) return true;
            if (strcmp(token.data, "String") == 0) return true;
            if (strcmp(token.data, "Null") == 0) return true;
            if (strcmp(token.data, "null") == 0) return true;
            return false;
        default:
            return false;
    }
}



bool reduce_expression(Stack* op_stack, Stack* val_stack) {
    
  
    
    Token op_token = Stack_Token_Top(op_stack);
    Stack_Token_Pop(op_stack);
    if (op_token.type == TOKEN_UNDEFINED) {
        return false;
    }

    //! добавить проверку типов операторов после is

    Token right = Stack_Token_Top(val_stack);
    Stack_Token_Pop(val_stack);
    if (right.type == TOKEN_UNDEFINED) {
        return false;
    }

    Token left = Stack_Token_Top(val_stack);
    Stack_Token_Pop(val_stack);
    if (left.type == TOKEN_UNDEFINED) {
        return false;
    }

    Token placeholder;
    placeholder.type = TOKEN_IDENTIFIER;
    placeholder.data = NULL; // Значение не важно, т.к. это просто заполнитель

    Stack_Token_Push(val_stack, placeholder);

    return true;
}

char *strdup_c99(const char *s) {
    size_t size = strlen(s) + 1;
    char *p = malloc(size);
    if (p) {
        memcpy(p, s, size);
    }
    return p;
}

void token_copy_data(Token* dest, const Token* src) {
    dest->type = src->type;
    dest->line = src->line;
    if (src->data != NULL) {
        dest->data = strdup_c99(src->data);
    } else {
        dest->data = NULL;
    }
}

bool parser_expression(Lexer *lexer, FILE *file) {
    Stack op_stack;
    Stack val_stack;
    Stack_Token_Init(&op_stack);
    Stack_Token_Init(&val_stack);

    int paren_depth = 0;
    Token current = peek_token(lexer, file);

    while (current.type != TOKEN_EOF && current.type != TOKEN_EOL && paren_depth >= 0 && current.type != TOKEN_OPEN_BRACE) {
        if (is_term(current)) {
            Token tok = get_token(lexer, file);       // получаем токен
            Token *to_push = token_init();
            token_copy_data(to_push, &tok);
            Stack_Token_Push(&val_stack, *to_push);          // кладём в стек
        }
        else if (current.type == TOKEN_OPEN_PAREN) {
            paren_depth++;
            Token tok = get_token(lexer, file);
            Token *to_push = token_init();
            token_copy_data(to_push, &tok);
            Stack_Token_Push(&op_stack, *to_push);
        }
        else if (current.type == TOKEN_CLOSE_PAREN) {
            paren_depth--;
            get_token(lexer, file); // съели ')'

            Token next = peek_token(lexer, file);
            if (next.type == TOKEN_OPEN_PAREN) {
                return false; // ошибка: лишняя открывающая скобка
            }

            if (paren_depth < 0) break; // лишняя закрывающая скобка

            // Свернуть стек операторов до ближайшей '('
            while (!Stack_Token_IsEmpty(&op_stack)) {
                Token top = Stack_Token_Top(&op_stack);
                if (top.type == TOKEN_OPEN_PAREN) {
                    Stack_Token_Pop(&op_stack);
                    break;
                }
                reduce_expression(&op_stack, &val_stack);
            }
        }
        else if (get_precedence(current) > 0) {
            Token tok = get_token(lexer, file);
            Token *to_push = token_init();
            token_copy_data(to_push, &tok);


            // Пока верхний оператор в стеке имеет **больший или равный приоритет**
            while (!Stack_Token_IsEmpty(&op_stack)) {
                Token top = Stack_Token_Top(&op_stack);
                // Stack_Token_Pop(&op_stack);
                if (get_precedence(top) >= get_precedence(tok)) {
                    reduce_expression(&op_stack, &val_stack);
                } else break;
            }

            Stack_Token_Push(&op_stack, *to_push);
        }
            else {
                // Некорректный токен в выражении
                return false;
            }

        current = peek_token(lexer, file);
    }
    // Свернуть оставшиеся операторы
    while (!Stack_Token_IsEmpty(&op_stack) && paren_depth >= 0) {
        reduce_expression(&op_stack, &val_stack);
    }
    bool success = (!Stack_Token_IsEmpty(&val_stack) && paren_depth == 0);
    while (!Stack_Token_IsEmpty(&val_stack)) {
        Token t = Stack_Token_Top(&val_stack);
        if (t.data) free(t.data);
        Stack_Token_Pop(&val_stack);
    }
    while (!Stack_Token_IsEmpty(&op_stack)) {
        Token t = Stack_Token_Top(&op_stack);
        if (t.data) free(t.data);
        Stack_Token_Pop(&op_stack);
    }
    

    
        
    
    
    Stack_Token_Dispose(&op_stack);
    Stack_Token_Dispose(&val_stack);

    return success;
}