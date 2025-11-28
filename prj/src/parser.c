#include "parser.h"
#include "expression.h"
#include "ast.h"
#include "error_codes.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

// Глобальная переменная для корня AST
static AstNode *program = NULL;

#define MAX_BUFFER_SIZE 100

/**
 * @brief Обрабатывает пролог программы (проверяет наличие 'import "Ifj25" for Ifj')
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 */
static void parser_prolog(Lexer *lexer, FILE *file);

/**
 * @brief Обрабатывает пролог программы (проверяет наличие 'class Program { EOL ... }')
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 */
static void parser_kostra(Lexer *lexer, FILE *file);

/**
 * @brief Обрабатывает список функций внутри класса Program
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 */
static void parser_function_list(Lexer *lexer, FILE *file);

/**
 * @brief Обрабатывает определение одной функции
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 */
static void parser_function_definition(Lexer *lexer, FILE *file);

/**
 * @brief Обрабатывает имя функции (static id ...)
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @return AstNode* Указатель на узел функции
 */
static AstNode *name_function(Lexer *lexer, FILE *file);

/**
 * @brief Обрабатывает хвост функции (... ) { ... }
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param func_node Указатель на узел функции
 */
static void tail_function(Lexer *lexer, FILE *file, AstNode *func_node);

/**
 * @brief Обрабатывает параметры функции (id, id, ...)
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param param_list Указатель на узел списка параметров
 */
static void function_params(Lexer *lexer, FILE *file, AstNode *param_list);

/**
 * @brief Обрабатывает тело функции { ... }
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param func_node Указатель на узел функции
 */
static void function_block(Lexer *lexer, FILE *file, AstNode *func_node);

/**
 * @brief Обрабатывает блок кода { всё что внутри разные операции ... }
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param block_node Указатель на узел блока
 */
static void handle_blok_body(Lexer *lexer, FILE *file, AstNode *block_node);

/**
 * @brief Обрабатывает одно выражение в правой части присваивания (id = ... )
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param block_node Указатель на узел блока
 */
static void right_side_expression(Lexer *lexer, FILE *file, AstNode *parent_node);

/**
 * @brief Парсит список термов ( term, term, ... )
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @return AstNode* Указатель на узел списка аргументов
 */
static AstNode *handle_fun_call_params(Lexer *lexer, FILE *file);

/**
 * @brief Проверяет, является ли имя встроенной функцией Ifj
 * 
 * @param name Имя функции
 * @return true Если функция встроенная
 */
static bool is_builtin(const char *name);

/**
 * @brief Находит соответствующий тип узла AST для данного токена
 * 
 * @param token Входной токен
 */
static NodeType token_to_node(Token token);

/**
 * @brief Проверяет, является ли токен терминальным (term)
 * 
 * @param token Входной токен
 */
static bool is_term(Token token);

/**
 * @brief Пропускает токен EOL, если он есть
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 */
static void skip_EOL(Lexer *lexer, FILE *file);

/**
 * @brief Форматирует полное имя встроенной функции Ifj.что-то
 * 
 * @param identifier Токен идентификатора функции
 * @param output_buffer Буфер для записи полного имени
 * @param buffer_size Размер буфера
 */
static void format_full_function_name(Token identifier, char *output_buffer, size_t buffer_size);

/**
 * @brief Обрабатывает идентификатор в присваивании
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param block_node Узел блока, куда будет добавлено присваивание
 */
static void handle_identifier (Lexer *lexer, FILE *file, AstNode *block_node);

/**
 * @brief Обрабатывает вызов встроенной функции Ifj.что-то
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param assignment_node Узел присваивания, куда будет добавлен вызов функции
 */
static void handle_builtin_call(Lexer *lexer, FILE *file, AstNode *assignment_node);

/**
 * @brief Обрабатывает вызов пользовательской функции id(...)
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param assignment_node Узел присваивания, куда будет добавлен вызов функции
 */
static void handle_function_call(Lexer *lexer, FILE *file, AstNode *assignment_node);

/**
 * @brief Обрабатывает выражение в правой части присваивания
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param assignment_node Узел присваивания, куда будет добавлено выражение
 */
static void handle_parser_expression(Lexer *lexer, FILE *file, AstNode *assignment_node);
    
        




static bool is_term(Token token) {
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
            if (strcmp(token.data, "null") == 0) return true;
            return false;
        default:
            return false;
    }
}

static NodeType token_to_node(Token token) {
    switch (token.type) {
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
        case TOKEN_KEYWORD:
            return NODE_LITERAL_NULL;
        default:
            exit(SYNTAX_ERROR); // Не оператор
    }
}

static bool is_builtin(const char *name) {
    const char *builtins[] = {
        "read_str",
        "read_num",
        "write",
        "floor",
        "str",
        "length",
        "substring",
        "strcmp",
        "ord",
        "chr"
    };
    // Проверяем наличие имени в списке встроенных функций
    // Ищем имя в массиве builtins
    int num_builtins = sizeof(builtins) / sizeof(builtins[0]);
    for (int i = 0; i < num_builtins; i++) {
        if (strcmp(name, builtins[i]) == 0) {
            return true;
        }
    }
    return false;
}

static void skip_EOL(Lexer *lexer, FILE *file) {
    if (peek_token(lexer, file).type == TOKEN_EOL) {
        get_token(lexer, file); // consume EOL
    }
}

static void format_full_function_name(Token identifier, char *output_buffer, size_t buffer_size) {
    snprintf(output_buffer, buffer_size, "Ifj.%s", identifier.data);
}

static void handle_identifier (Lexer *lexer, FILE *file, AstNode *block_node) {
    Token identifier = peek_token(lexer, file);
    if (peek_next_token(lexer, file).type == TOKEN_ASSIGN) {
        // создаем узел присваивания
        AstNode *assignment_node = ast_node_create(NODE_ASSIGNMENT, peek_next_token(lexer, file).line);
        ast_node_add_child(block_node, assignment_node);
        // создаем узел идентификатора для левой части
        AstNode *id_node = ast_new_id_node(NODE_ID, identifier.line, identifier.data);
        ast_node_add_child(assignment_node, id_node);
        
        get_token(lexer, file); // consume identifier
        get_token(lexer, file); // consume '='

        // обрабатываем правую часть выражения
        right_side_expression(lexer, file, assignment_node);

    } else {
        printf("Expected '=' after identifier in assignment.\n");
        get_token(lexer, file); // consume identifier token
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
}

static void handle_builtin_call(Lexer *lexer, FILE *file, AstNode *assignment_node) {
    get_token(lexer, file); // consume 'Ifj' keyword
    if (peek_token(lexer, file).type == TOKEN_DOT) {
        get_token(lexer, file); // consume '.'

        skip_EOL(lexer, file);
        Token current_token = peek_token(lexer, file);
        // Проверяем, что это действительно встроенная функция
        if (!is_builtin(current_token.data)) {
            printf("Unknown method name: %s\n", current_token.data);
            exit(SYNTAX_ERROR);
        }

        // Формируем полное имя встроенной функции, для красивой записи в AST
        char full_function_name[MAX_BUFFER_SIZE];
        format_full_function_name(current_token, full_function_name, sizeof(full_function_name));
        
        AstNode *node_statement = ast_node_create(NODE_CALL_STATEMENT, current_token.line);
        
        AstNode *builtin_func_name = ast_new_id_node(NODE_ID, current_token.line, full_function_name);

        get_token(lexer, file); // consume method name

        if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
            printf("Expected '(' after method name.\n");
            get_token(lexer, file); // consume invalid token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // consume '('

        // Здесь обработка параметров метода
        AstNode *list_args = handle_fun_call_params(lexer, file);

        if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
            printf("Expected ')' after method parameters.\n");
            get_token(lexer, file); // consume invalid token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // consume ')'

        // Создаем узлы для вызова встроенной функции
        ast_node_add_child(node_statement, builtin_func_name);
        ast_node_add_child(node_statement, list_args);
        ast_node_add_child(assignment_node, node_statement);

        if (peek_token(lexer, file).type != TOKEN_EOL) {
            printf("Expected end of line after method call.\n");
            get_token(lexer, file); // consume invalid token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // consume EOL
        return;
    }
    else {
        printf("Expected '.' after 'Ifj'.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
}

static void handle_function_call(Lexer *lexer, FILE *file, AstNode *assignment_node) {
    Token identifier = peek_token(lexer, file);
    AstNode *node_statement = ast_node_create(NODE_CALL_STATEMENT, identifier.line);
    AstNode *node_id = ast_new_id_node(NODE_ID, identifier.line, identifier.data);
    get_token(lexer, file); // consume identifier

    // Здесь можно добавить обработку параметров функции
    if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
        printf("Expected '(' after function name.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // consume '('
    AstNode *list_args = handle_fun_call_params(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
        printf("Expected ')' after function parameters.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // consume ')'
    

    ast_node_add_child(node_statement, node_id);
    ast_node_add_child(node_statement, list_args);
    ast_node_add_child(assignment_node, node_statement);


    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after function call.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // consume EOL
    return;
}

static void handle_parser_expression(Lexer *lexer, FILE *file, AstNode *assignment_node) {
    if (!parser_expression(lexer, file, assignment_node)) {
        printf("Invalid expression on right side of assignment.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after expression.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // consume EOL
}

static AstNode *handle_fun_call_params(Lexer *lexer, FILE *file) {
    // Создаем узел списка аргументов
    AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, peek_token(lexer, file).line);
    skip_EOL(lexer, file);
    Token current_token = peek_token(lexer, file);
    // Проверяем есть ли термы
    if (is_term(current_token)) {
        // Первый терм
        NodeType node_type_term = token_to_node(current_token);
        AstNode *leaf_node = create_leaf_node(current_token, node_type_term);
        if (leaf_node == NULL) {
            exit(INTERNAL_ERROR); // Ошибка аллокации
        }
        get_token(lexer, file); // consume parameter
        // Добавляем первый терм в список аргументов
        ast_node_add_child(arg_list, leaf_node);
        // Последующие термы через запятую
        while (peek_token(lexer, file).type == TOKEN_COMMA) {
            get_token(lexer, file); // consume ','   
            skip_EOL(lexer, file);
            Token current_token = peek_token(lexer, file);
            if (is_term(current_token) == false) {
                printf("Expected parameter identifier after ','.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            }
            NodeType node_type_term = token_to_node(current_token);
            AstNode *leaf_node = create_leaf_node(current_token, node_type_term);
            if (leaf_node == NULL) {
                return false; // Ошибка аллокации
            }
            get_token(lexer, file); // consume parameter identifier
            ast_node_add_child(arg_list, leaf_node);
        }
    }
    skip_EOL(lexer, file);
    return arg_list;
}

static void right_side_expression(Lexer *lexer, FILE *file, AstNode *assignment_node) {
    // Проверяем на вызов встроенной функции Ifj.что-то
    if (peek_token(lexer, file).type == TOKEN_KEYWORD &&
        strcmp(peek_token(lexer, file).data, "Ifj") == 0) {
        handle_builtin_call(lexer, file, assignment_node);
        return;
        }
    // Проверяем на вызов пользовательской функции id(...) или выражение, начинающееся с идентификатора
    if (peek_token(lexer, file).type == TOKEN_IDENTIFIER || 
        peek_token(lexer, file).type == TOKEN_GLOBAL_IDENTIFIER) {   
        if (peek_next_token(lexer, file).type == TOKEN_OPEN_PAREN) {
            handle_function_call(lexer, file, assignment_node);
            return;
        }  
        // выражение, которое начинается с идентификатора
        else  {
            handle_parser_expression(lexer, file, assignment_node);
            return;
        }
    }
    // обычное выражение
    else {
        handle_parser_expression(lexer, file, assignment_node);
        return;
    }
}

static void handle_blok_body(Lexer *lexer, FILE *file, AstNode *block_node) {
    switch (peek_token(lexer, file).type) {
    case TOKEN_GLOBAL_IDENTIFIER: {
        if (peek_next_token(lexer, file).type == TOKEN_EOL) {
            AstNode *global_id_node = ast_new_id_node(NODE_ID, peek_token(lexer, file).line, peek_token(lexer, file).data);
            ast_node_add_child(block_node, global_id_node);
            get_token(lexer, file); // consume global identifier
            get_token(lexer, file); // consume EOL
            break;
        }
        handle_identifier(lexer, file, block_node);
        break;
    }
    case TOKEN_IDENTIFIER: {
        handle_identifier(lexer, file, block_node);
        break;
    }
    case TOKEN_KEYWORD:
        if (strcmp(peek_token(lexer, file).data, "return") == 0) {
            // создаем узел return
            AstNode *return_node = ast_node_create(NODE_RETURN, peek_token(lexer, file).line);
            ast_node_add_child(block_node, return_node);
            get_token(lexer, file); // consume 'return' keyword

            //? Пустой return
            // if (peek_token(lexer, file).type == TOKEN_EOL) {
            //     get_token(lexer, file); // consume EOL
            //     break;
            // }

            // return expression
            handle_parser_expression(lexer, file, return_node);
            break;
        }
        else if (strcmp(peek_token(lexer, file).data, "if") == 0) {
            
            AstNode *if_node = ast_node_create(NODE_IF, peek_token(lexer, file).line);
            ast_node_add_child(block_node, if_node);
            
            get_token(lexer, file); // consume 'if' keyword
            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after 'if' keyword.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            }
            
            // скобки обрабатываются в parser_expression
            if (!parser_expression(lexer, file, if_node)) {
                printf("Invalid expression in 'if' condition.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            }
            
            // обработка if блока
            function_block(lexer, file, if_node);
            
            // обработка else блока (по заданию, но можно изменить)
            if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
                strcmp(peek_token(lexer, file).data, "else") != 0) {
                printf("Expected 'else' keyword after 'if' block.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            } else {
                get_token(lexer, file); // consume 'else' keyword
                
                // обработка else блока
                function_block(lexer, file, if_node);
                if (peek_token(lexer, file).type != TOKEN_EOL) {
                    printf("Expected end of line after 'else' block.\n");
                    get_token(lexer, file); // consume invalid token
                    exit(SYNTAX_ERROR);
                }
                get_token(lexer, file); // consume EOL
            }
            break;
        }
        else if (strcmp(peek_token(lexer, file).data, "while") == 0) {
            
            AstNode *while_node = ast_node_create(NODE_WHILE, peek_token(lexer, file).line);
            ast_node_add_child(block_node, while_node);
            
            get_token(lexer, file); // consume 'while' keyword
            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after 'while' keyword.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            }
            // скобки обрабатываются в parser_expression
            if (!parser_expression(lexer, file, while_node)) {
                printf("Invalid expression in 'while' condition.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            }
            
            // обработка while блока
            function_block(lexer, file, while_node);
            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after 'while' block.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            }
            get_token(lexer, file); // consume EOL
            break;
        }
        else if (strcmp(peek_token(lexer, file).data, "var") == 0) {
            get_token(lexer, file); // consume 'var' keyword

            // Проверяем чтобы была id
            if (peek_token(lexer, file).type != TOKEN_IDENTIFIER && 
                peek_token(lexer, file).type != TOKEN_GLOBAL_IDENTIFIER) {
                printf("Expected variable name after 'var' keyword.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            }

            
            AstNode *var_node = ast_new_id_node(NODE_VAR_DEF, peek_token(lexer, file).line, peek_token(lexer, file).data);
            ast_node_add_child(block_node, var_node);
            
            get_token(lexer, file); // consume variable name
            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after variable declaration.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            }
            get_token(lexer, file); // consume EOL
            break;
        }
        else {
            printf("Unexpected keyword in function body: %s\n", peek_token(lexer, file).data);
            get_token(lexer, file); // consume invalid token
            exit(SYNTAX_ERROR);
        }
        break;
    case TOKEN_EOL:
        get_token(lexer, file); // consume EOL
        break;
    case TOKEN_CLOSE_BRACE:
        return; // end of function body
    case TOKEN_OPEN_BRACE:
        // Начало вложенного блока
        function_block(lexer, file, block_node);
        // После вложенного блока должен быть EOL
        if (peek_token(lexer, file).type != TOKEN_EOL) {
            printf("Expected end of line after nested block.\n");
            get_token(lexer, file); // consume invalid token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // consume EOL
        break;
    case TOKEN_EOF:
        printf("Unexpected end of file inside function body.\n");
        exit(SYNTAX_ERROR);
    default:
        exit(SYNTAX_ERROR);
    }
}

static void function_block(Lexer *lexer, FILE *file, AstNode *func_node) {
    if (peek_token(lexer, file).type != TOKEN_OPEN_BRACE) {
        printf("Expected '{' to open function body.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // consume '{'
    //? расширение EOL после {
    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after '{' in function body.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    
    AstNode *block_node = ast_node_create(NODE_BLOCK, peek_token(lexer, file).line);
    get_token(lexer, file); // consume EOL
    
    // Добавляем block_node в дерево AST здесь
    ast_node_add_child(func_node, block_node);

    // Здесь обработка тела функции
    while (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        handle_blok_body(lexer, file, block_node);
    }
    // Обработка EOL перед '}' проверяется в handle_blok_body

    if (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        printf("Expected '}' to close function body.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // consume '}'
}

static void function_params(Lexer *lexer, FILE *file, AstNode *param_list) {
    // Условие задания, после скобки может быть EOL (они не проверяют, но пусть будет)
    skip_EOL(lexer, file);
    // Здесь можно добавить обработку параметров функции
    if (peek_token(lexer, file).type == TOKEN_IDENTIFIER) {
        
        AstNode *param_node = ast_new_id_node(NODE_PARAM, peek_token(lexer, file).line, peek_token(lexer, file).data);
        // Добавляем param_node в дерево AST здесь
        ast_node_add_child(param_list, param_node);
        
        get_token(lexer, file); // consume parameter identifier
        // Обработка дополнительных параметров
        while (peek_token(lexer, file).type == TOKEN_COMMA) {
            get_token(lexer, file); // consume ','
            if (peek_token(lexer, file).type == TOKEN_EOL) {
                get_token(lexer, file); // consume EOL
            }
            if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
                printf("Expected parameter identifier after ','.\n");
                get_token(lexer, file); // consume invalid token
                exit(SYNTAX_ERROR);
            }
            get_token(lexer, file); // consume parameter identifier

            AstNode *param_node = ast_new_id_node(NODE_PARAM, lexer->current_token->line, lexer->current_token->data);
            // Добавляем param_node в дерево AST здесь
            ast_node_add_child(param_list, param_node);
            
        }
    }
}

static void tail_function(Lexer *lexer, FILE *file, AstNode *func_node) {
    if (peek_token(lexer, file).type == TOKEN_OPEN_PAREN) {
        get_token(lexer, file); // consume '('

        AstNode *param_list = ast_node_create(NODE_PARAM_LIST, peek_token(lexer, file).line);
        // Добавляем param_list в дерево AST здесь
        ast_node_add_child(func_node, param_list);

        // Здесь можно добавить обработку параметров функции
        function_params(lexer, file, param_list);

        if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
            printf("Expected ')' after function parameters.\n");
            get_token(lexer, file); // consume invalid token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // consume ')'
    }
    if (peek_token(lexer, file).type == TOKEN_EOL) {
        printf("Unexpected end of line after function header.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    // Здесь переходим к обработке тела функции
    function_block(lexer, file, func_node);
}

static AstNode *name_function(Lexer *lexer, FILE *file) {
    get_token(lexer, file); // consume 'static' keyword

    // Проверяем чтобы была id
    if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
        printf("Expected function name identifier after 'function' keyword.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    AstNode *func_name_node;
    // Сохраняем имя функции
    Token func_name_token = peek_token(lexer, file);

    // Определяем тип функции по следующему токену
    // foo() - обычная функция
    if (peek_next_token(lexer, file).type == TOKEN_OPEN_PAREN) {
        func_name_node = ast_new_id_node(NODE_FUNCTION_DEF, func_name_token.line, func_name_token.data);
        ast_node_add_child(program, func_name_node);
    }
    // foo{} - геттер
    else if (peek_next_token(lexer, file).type == TOKEN_OPEN_BRACE) {
        func_name_node = ast_new_id_node(NODE_GETTER_DEF, func_name_token.line, func_name_token.data);
        ast_node_add_child(program, func_name_node);
    }
    // foo = ... - сеттер
    else if (peek_next_token(lexer, file).type == TOKEN_ASSIGN) {
        func_name_node = ast_new_id_node(NODE_SETTER_DEF, func_name_token.line, func_name_token.data);
        ast_node_add_child(program, func_name_node);
        get_token(lexer, file); // consume '='
    }
    // Ошибка, если не один из ожидаемых токенов
    else {
        printf("Expected '(' or '{' or '=' after function name.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // consume function name
    return func_name_node;
}

static void parser_function_definition(Lexer *lexer, FILE *file) {
    // Проверяем ввод имени функции и определяем тип функции, записывая его в узел AST
    AstNode *func_node = name_function(lexer, file);

    // Обрабатываем хвост функции (параметры и тело)
    tail_function(lexer, file, func_node);

    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after function definition.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
}

static void parser_function_list(Lexer *lexer, FILE *file) {
    switch (peek_token(lexer, file).type) {
    case TOKEN_CLOSE_BRACE:
        return; // Базовый случай: конец списка функций
    case TOKEN_KEYWORD:
        if (strcmp(peek_token(lexer, file).data, "static") == 0) {
            
            // Обработка функции
            parser_function_definition(lexer, file);
            return;
        }
        printf("Expected static or }\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    case TOKEN_EOL:
        get_token(lexer, file); // пропустить пустые строки
        parser_function_list(lexer, file);
        break;
    default:
        printf("Unexpected token in function list: %s\n",
               peek_token(lexer, file).data);
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
}

static void parser_prolog(Lexer *lexer, FILE *file) {
    if (peek_token(lexer, file).type == TOKEN_EOF) {
        printf("Empty file.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    skip_EOL(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(peek_token(lexer, file).data, "import") != 0) {
        printf("Expected 'import' keyword at the beginning.");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_STRING ||
        strcmp(peek_token(lexer, file).data, "ifj25") != 0) {
        printf("Expected string literal after 'import' keyword.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    //! Проверка на EOL сразу после строкового литерала посмотреть надо ли её вообще делать
    if (peek_token(lexer, file).type == TOKEN_EOL) {
        printf("Expected 'for' keyword after import string literal.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(peek_token(lexer, file).data, "for") != 0) {
        printf("Expected 'for' keyword after import string literal.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(peek_token(lexer, file).data, "Ifj") != 0) {
        printf("Expected identifier 'Ifj' after 'for' keyword.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);
}

static void parser_kostra(Lexer *lexer, FILE *file) {

    // Здесь будет дальнейшая реализация парсинга основной структуры программы
    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(peek_token(lexer, file).data, "class") != 0) {
        printf("Expected 'class' keyword to start class definition.\n"); 
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_IDENTIFIER ||
        strcmp(peek_token(lexer, file).data, "Program") != 0) {
        printf("Expected 'Program' identifier after 'class' keyword.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_OPEN_BRACE) {
        printf("Expected '{' to open class body.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after class body.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    // Парсим список функций внутри класса Program
    // Добавляем корневой узел программы в дерево AST здесь
    program = ast_node_create(NODE_PROGRAM, lexer->current_token->line);

    // Рекурсивно парсим функции до закрывающей скобки класса
    while (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        parser_function_list(lexer, file);
    }
    get_token(lexer, file);

    // Проверяем конец файла
    if (peek_token(lexer, file).type != TOKEN_EOF) {
        printf("Expected end of file after class definition.\n");
        get_token(lexer, file);
        exit(SYNTAX_ERROR);
    }
}

AstNode *parser_run(FILE *file) {

    Lexer *lexer = lexer_init();
    if (lexer == NULL) {
        fprintf(stderr, "Error initializing lexer.\n");
        fclose(file);
        exit(99);
    }

    // Для первой фразы import "ifj25" for Ifj
    parser_prolog(lexer, file);
    Token current = peek_token(lexer, file);
    if (current.type != TOKEN_EOL) {
        printf("Expected end of line after import statement.\n");
        get_token(lexer, file); // consume invalid token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // consume EOL
    // Основной парсер
    parser_kostra(lexer, file);
    lexer_free(lexer);

    return program;
}