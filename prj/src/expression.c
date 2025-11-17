#include "expression.h"
#include "stack_precedence.h"
#include "token.h"
#include "utils.h"
#include <errno.h>

#include "ast.h"
#include "ast_printer.h"
#include <string.h>
#include <stdlib.h>

bool char_to_double(const char *str, double *out_value) {
    if (!str || !out_value) return false;

    errno = 0;
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

bool parser_expression(Lexer *lexer, FILE *file, AstNode *expr_node) {
    // Инициализация стека
    PStack stack;
    PSTACK_init(&stack);

    // Помещаем $ на стек
    PStackItem dollar_item = { .symbol = GS_DOLLAR, .token = {0}, .ast_node = NULL };
    PSTACK_push(&stack, dollar_item);

    // Читаем первый токен
    Token current_token = peek_token(lexer, file);
    
    while (1) {
        // Получаем верхний терминал стека
        PStackItem *top_terminal_item = PSTACK_get_top_terminal(&stack);
        if (top_terminal_item == NULL) return false; // Ошибка

        // Преобразуем токен во входной терминал
        TermIndex input_index = token_to_index(current_token);

        TermIndex stack_index = token_to_index(top_terminal_item->token);

        // Смотрим что пришло из таблицы прецедентов
        char rule = get_precedence_rule(stack_index, input_index);

        if (stack_index == T_DOLLAR && input_index == T_DOLLAR) {
            break; // Успех!
        }

        if (rule == '<') {
            // Сдвиг (Shift)
            if (!PSTACK_insert_handle_start(&stack)) {
                return false; // Ошибка
            }
            PStackItem new_item;
            new_item.token = current_token;

            if (input_index == T_TERM) {
                // Создаем AST узел для терма
                AstNode *leaf_node;
                // Заполняем узел в зависимости от типа токена

                NodeType node_type_term = token_type_to_node(current_token);
                if (node_type_term == NODE_ID) {
                    leaf_node = ast_new_id_node(node_type_term, current_token.line, current_token.data);
                } else if (node_type_term == NODE_LITERAL_NUM) {
                    double num_value;
                    if (!char_to_double(current_token.data, &num_value)) {
                        return false; // Ошибка конверсии
                    }
                    leaf_node = ast_new_num_node(num_value, current_token.line);
                } else if (node_type_term == NODE_LITERAL_STRING) {
                    leaf_node = ast_new_string_node(current_token.data, current_token.line);
                } else if (node_type_term == NODE_LITERAL_NULL) {
                    leaf_node = ast_new_null_node(current_token.line);
                } else {
                    return false; // Неизвестный тип терма
                }
                if (leaf_node == NULL) {
                    return false; // Ошибка аллокации
                }
                new_item.symbol = GS_E;
                new_item.ast_node = leaf_node;

            }
            else {
                // Оператор
                new_item.symbol = token_to_grammar_symbol(current_token);
                new_item.ast_node = NULL; // Операторы не создают узлы AST на данном этапе
            }
            PSTACK_push(&stack, new_item);
            get_token(lexer, file); // consume token
            current_token = peek_token(lexer, file);
        }
        else if (rule == '=') {
            // Сдвиг (Shift) для скобок
            PStackItem new_item;
            new_item.token = current_token;
            new_item.symbol = token_to_grammar_symbol(current_token);
            new_item.ast_node = NULL; // Скобки не создают узлы AST на данном этапе
            PSTACK_push(&stack, new_item);

            get_token(lexer, file); // consume token
            current_token = peek_token(lexer, file);
        }
        else if (rule == '>') {
            // Свёртка (Reduce)
            if (!handle_reduce(&stack, expr_node)) {
                PSTACK_free(&stack);
                return false; // Ошибка синтаксиса
            }
        }
        else {
            PSTACK_free(&stack);
            return false; // Ошибка синтаксиса
        }
    }
    PStackItem final_item = PSTACK_pop(&stack); 
    if (final_item.symbol != GS_E) {
        // Если на верхушке не GS_E, это ошибка
        PSTACK_free(&stack); 
        return false; 
    }
}