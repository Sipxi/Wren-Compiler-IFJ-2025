#include "expression.h"
#include "stack_precedence.h"
#include "precedence.h"
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
            exit(25); // Не оператор
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
            exit(25); // Не оператор
    }
}

NodeType grammar_symbol_to_node_type(GrammarSymbol symbol) {
    switch (symbol) {
        case GS_PLUS:   return NODE_OP_PLUS;
        case GS_MINUS:  return NODE_OP_MINUS;
        case GS_MUL:    return NODE_OP_MUL;
        case GS_DIV:    return NODE_OP_DIV;
        case GS_LT:     return NODE_OP_LT;
        case GS_GT:     return NODE_OP_GT;
        case GS_LTE:    return NODE_OP_LTE;
        case GS_GTE:    return NODE_OP_GTE;
        case GS_EQ:     return NODE_OP_EQ;
        case GS_NEQ:    return NODE_OP_NEQ;
        case GS_IS:     return NODE_OP_IS;
        default:        exit(25); // Ошибка
    }
}

bool handle_reduce(PStack *stack) {
    PStackItem handle[MAX_RULE_LENGTH];
    int count = 0;
    // Извлекаем элементы до маркера начала рукоятки
    for (int i = 0; i < MAX_RULE_LENGTH-1; i++) {
        if (PSTACK_is_empty(stack)) {
            // Ошибка: дошли до $ и не нашли маркер '<'
            return false; 
        }
        PStackItem item = PSTACK_pop(stack);
        // if (item.symbol == GS_HANDLE_START) {
        //     PSTACK_pop(stack);
        //     break; // Найден маркер начала рукоятки
        // }
        if (count >= MAX_RULE_LENGTH) {
            // Ошибка: рукоятка длиннее, чем любое из наших правил
            return false; 
        }
        handle[count++] = item;
    }
    if (count == 0) {
        // Ошибка: пустая рукоятка (например, <>)
        return false;
    }
    AstNode *new_ast_node = NULL;
    bool rule_found = false;

    for (int i = 0; i < NUM_GRAMMAR_RULES; i++) {
        // Сравниваем рукоятку с правилом
        if (grammar_rules[i][1] == GS_TERM) {
            continue; 
        }
        const int* rule = grammar_rules[i];
        int rule_length = 0;
        for (int k = 1; k < MAX_RULE_LENGTH; k++) {
            if (rule[k] == GS_UNDEF) break;
            rule_length++;
        }
        if (rule_length != count) {
            continue;
        }
        bool match = true;
        for (int j = 0; j < count; j++) {
            if ((int) handle[j].symbol != rule[rule_length - j]) {
                match = false;
                break;
            }
        }
        if (match) {
            rule_found = true;
            if (i >=0 && i <= 9) {
                // E -> E op E
                AstNode *right_node = handle[0].ast_node;
                AstNode *left_node = handle[2].ast_node;
                Token op_token = handle[1].token;
                NodeType node_type = grammar_symbol_to_node_type(handle[1].symbol);
                new_ast_node = ast_new_bin_op(node_type, op_token.line, left_node, right_node);
                
            }
            else if (i == 10) {
                // E -> E is k
                AstNode *left_node = handle[2].ast_node;
                Token type_token = handle[0].token; // Токен "Num", "String", "Null"
                Token is_token = handle[1].token;

                NodeType node_type = NODE_OP_IS;
                new_ast_node = ast_node_create(node_type, is_token.line);
                if (new_ast_node == NULL) {
                    return false; // Ошибка аллокации
                }
                ast_node_add_child(new_ast_node, left_node);
                AstNode *type_node = ast_new_id_node(NODE_TYPE_NAME, type_token.line, type_token.data);
                if (type_node == NULL) {
                    return false; // Ошибка аллокации
                }
                ast_node_add_child(new_ast_node, type_node);
            }
            else if (i == 11) {
                // E -> ( E )
                new_ast_node = handle[1].ast_node;
            }

            break; // Правило найдено и обработано
        }
    }
    if (!rule_found) {
        // Ошибка: рукоятка не соответствует ни одному правилу
        return false;
    }
    PStackItem result_item = {0}; // Инициализируем нулями
    result_item.symbol = GS_E; // Результат свёртки - это всегда GS_E
    result_item.ast_node = new_ast_node; // С новым АСТ-узлом
    
    PSTACK_push(stack, result_item);

    return true; // Заглушка
}

bool parser_expression(Lexer *lexer, FILE *file, AstNode *expr_node) {
    // Инициализация стека
    PStack stack;
    PSTACK_init(&stack);

    // Помещаем $ на стек
    PStackItem dollar_item = { .symbol = GS_DOLLAR, .token.type = TOKEN_UNDEFINED, .ast_node = NULL };
    PSTACK_push(&stack, dollar_item);

    // Читаем первый токен
    Token current_token = peek_token(lexer, file);
    
    while (1) {
        // Получаем верхний терминал стека
        PStackItem *top_terminal_item = PSTACK_get_top_terminal(&stack);
        if (top_terminal_item == NULL) return false; // Ошибка

        TermIndex stack_index = token_to_index(top_terminal_item->token);

        // Преобразуем токен во входной терминал
        TermIndex input_index = token_to_index(current_token);


        // Смотрим что пришло из таблицы прецедентов
        char rule = get_precedence_rule(stack_index, input_index);

        if (stack_index == T_DOLLAR && input_index == T_DOLLAR) {
            break; // Успех!
        }

        if (rule == '<') {
            // Сдвиг (Shift)
            // if (!PSTACK_insert_handle_start(&stack)) {
            //     return false; // Ошибка
            // }
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
            if (!handle_reduce(&stack)) {
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

    PStackItem item = PSTACK_pop(&stack);

    // Проверка 2: Это действительно $?
    if (item.symbol != GS_DOLLAR) {
        // Если под GS_E был не $, это ошибка
        PSTACK_free(&stack);
        return false;
    }

    if (!PSTACK_is_empty(&stack)) {
        // Если под $ что-то еще осталось, это ошибка
        PSTACK_free(&stack);
        return false;
    }
    ast_node_add_child(expr_node, final_item.ast_node);
    PSTACK_free(&stack); 
    return true;
}