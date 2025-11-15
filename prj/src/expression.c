#include "expression.h"
#include "stack_token.h"
#include "token.h"
#include "utils.h"

#include "ast.h"
#include "ast_printer.h"
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

 NodeType token_type_to_node(Token token) {
    switch (token.type) {
        case TOKEN_PLUS:
            return NODE_OP_PLUS;
        case TOKEN_MINUS:
            return NODE_OP_MINUS;
        case TOKEN_MULTIPLY:
            return NODE_OP_MUL;
        case TOKEN_DIVISION:
            return NODE_OP_DIV;
        case TOKEN_LESS:
            return NODE_OP_LT;
        case TOKEN_GREATER:
            return NODE_OP_GT;
        case TOKEN_EQUAL_LESS:
            return NODE_OP_LTE;
        case TOKEN_EQUAL_GREATER:
            return NODE_OP_GTE;
        case TOKEN_EQUAL:
            return NODE_OP_EQ;
        case TOKEN_NOT_EQUAL:
            return NODE_OP_NEQ;
        case TOKEN_KEYWORD:
            if (strcmp(token.data, "is") == 0) {
                return NODE_OP_IS;
            }
            if (strcmp(token.data, "Num") == 0 || strcmp(token.data, "String") == 0 || strcmp(token.data, "Null") == 0) {
                return NODE_TYPE_NAME; // default to IS for unknown keywords
            }
            if (strcmp(token.data, "null") == 0) {
                return NODE_LITERAL_NULL;
            }
            return -1; // Не оператор
        case TOKEN_IDENTIFIER:
        case TOKEN_GLOBAL_IDENTIFIER:
            return NODE_ID; // not an operator, but a term
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_EXP:
        case TOKEN_HEX:
            return NODE_LITERAL_NUM;
        case TOKEN_STRING:
            return NODE_LITERAL_STRING;
        default:
            return -1; // Не оператор
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

bool char_to_double(const char *str, double *out_value) {
    if (!str || !out_value) return false;

    int errno = 0;
    char *endptr = NULL;
    double val = strtod(str, &endptr);

    // Проверка ошибок
    if (errno != 0) return false;              // ошибка конверсии
    if (endptr == str) return false;           // ничего не распознано
    while (*endptr == ' ' || *endptr == '\t')  // пропускаем пробелы
        endptr++;
    if (*endptr != '\0') return false;         // мусор после числа

    *out_value = val;
    return true;
}


int counter = 0;
AstNode *current_op;

bool process_expression(Stack* op_stack, Stack* val_stack, AstNode *expr_node) {
    
    counter++;
    
    //! добавить проверку типов операторов после is
    
    Token right = Stack_Token_Top(val_stack);
    AstNode *right_node;
    AstNode *left_node;

    if (right.type == TOKEN_IDENTIFIER && right.data == NULL) {
        right_node = current_op;
        Stack_Token_Pop(val_stack);
        if (right.type == TOKEN_UNDEFINED) {
            return false;
        }
    } else {
        NodeType node_type_right = token_type_to_node(right);
        right_node = ast_new_id_node(node_type_right, right.line, right.data);
        Stack_Token_Pop(val_stack);
        if (right.type == TOKEN_UNDEFINED) {
            return false;
        }
    }

    
    Token left = Stack_Token_Top(val_stack);

    if (left.type == TOKEN_IDENTIFIER && left.data == NULL) {
        left_node = current_op;
        Stack_Token_Pop(val_stack);
        if (left.type == TOKEN_UNDEFINED) {
            return false;
        }
    } else {
        NodeType node_type_left = token_type_to_node(left);
        left_node = ast_new_id_node(node_type_left, left.line, left.data);
        Stack_Token_Pop(val_stack);
        if (left.type == TOKEN_UNDEFINED) {
            return false;
        }
    }


    
    Token op_token = Stack_Token_Top(op_stack);
    Stack_Token_Pop(op_stack);
    if (op_token.type == TOKEN_UNDEFINED) {
        return false;
    }
    NodeType node_type_op = token_type_to_node(op_token);
    AstNode *op_node = ast_new_bin_op(node_type_op, op_token.line, left_node, right_node); 

    current_op = op_node;
    
    Stack_Token_Pop(op_stack);
    if (op_token.type == TOKEN_UNDEFINED) {
        return false;
    }

    Token placeholder;
    placeholder.type = TOKEN_IDENTIFIER;
    placeholder.data = NULL; // Значение не важно, т.к. это просто заполнитель

    Stack_Token_Push(val_stack, placeholder);

    return true;
}


bool parser_expression(Lexer *lexer, FILE *file, AstNode *expr_node) {
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
                process_expression(&op_stack, &val_stack, expr_node);
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
                    process_expression(&op_stack, &val_stack, expr_node);
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
        process_expression(&op_stack, &val_stack, expr_node);
    }
    bool success = (!Stack_Token_IsEmpty(&val_stack) && paren_depth == 0);

    if (success) {
        if (counter == 0) {
            // Единственное значение в стеке - это корень выражения
            Token final = Stack_Token_Top(&val_stack);
            NodeType node_type_final = token_type_to_node(final);
            if (node_type_final == NODE_ID) {
                AstNode *final_node = ast_new_id_node(node_type_final, final.line, final.data);
                ast_node_add_child(expr_node, final_node);
            } else if (node_type_final == NODE_LITERAL_NUM) {
                double num_value;
                if (!char_to_double(final.data, &num_value)) {
                    success = false;
                }
                AstNode *final_node = ast_new_num_node(num_value, final.line);
                ast_node_add_child(expr_node, final_node);
            } else if (node_type_final == NODE_LITERAL_STRING) {
                AstNode *final_node = ast_new_string_node(final.data, final.line);
                ast_node_add_child(expr_node, final_node);
            } else if (node_type_final == NODE_LITERAL_NULL) {
                AstNode *final_node = ast_new_null_node(final.line);
                ast_node_add_child(expr_node, final_node);
            } else {
                // Ошибка: некорректный тип финального узла
                success = false;
            }
        }
        ast_node_add_child(expr_node, current_op);
    }

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