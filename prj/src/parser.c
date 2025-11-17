#include "parser.h"
#include "expression.h"
#include "ast.h"
#include <stdio.h>
#include <string.h>

static AstNode *program = NULL;


static void parameters_function(Lexer *lexer, FILE *file, AstNode *param_list);
static void function_block(Lexer *lexer, FILE *file, AstNode *func_node);
static void tail_function(Lexer *lexer, FILE *file, AstNode *func_node);
static AstNode *name_function(Lexer *lexer, FILE *file);
static void parser_function_definition(Lexer *lexer, FILE *file);
static void parser_function_list(Lexer *lexer, FILE *file);
static void right_side_expression(Lexer *lexer, FILE *file, AstNode *parent_node);
static void operations_function(Lexer *lexer, FILE *file, AstNode *block_node);
static AstNode *list_of_tersms(Lexer *lexer, FILE *file);

// Проверяет, является ли токен терминальным (term)
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


// Парсит список term функции
AstNode *list_of_tersms(Lexer *lexer, FILE *file) {
    AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, lexer->current_token->line);
    if (peek_token(lexer, file).type == TOKEN_EOL) {
        get_token(lexer, file); // consume EOL
    }
    if (is_term(peek_token(lexer, file))) {
        get_token(lexer, file); // consume parameter
        ast_node_add_child(arg_list, ast_new_id_node(NODE_ID, lexer->current_token->line, lexer->current_token->data));
        if (peek_token(lexer, file).type == TOKEN_EOL) {
            get_token(lexer, file); // consume EOL
        }
        while (peek_token(lexer, file).type == TOKEN_COMMA) {
            get_token(lexer, file); // consume ','   
            if (peek_token(lexer, file).type == TOKEN_EOL) {
                get_token(lexer, file); // consume EOL
            }
            if (is_term(peek_token(lexer, file)) == false) {
                printf("Expected parameter identifier after ','.\n");
                exit(25);
            }
            get_token(lexer, file); // consume parameter identifier
            ast_node_add_child(arg_list, ast_new_id_node(NODE_ID, lexer->current_token->line, lexer->current_token->data));
        }
    }
    return arg_list;
}

// Проверяет, является ли имя встроенной функцией Ifj
bool is_builtin(const char *name) {
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
    int num_builtins = sizeof(builtins) / sizeof(builtins[0]);
    for (int i = 0; i < num_builtins; i++) {
        if (strcmp(name, builtins[i]) == 0) {
            return true;
        }
    }
    return false;
}

void right_side_expression(Lexer *lexer, FILE *file, AstNode *assignment_node) {
    // Проверяем на вызов встроенной функции Ifj.что-то
    if (peek_token(lexer, file).type == TOKEN_KEYWORD &&
        strcmp(lexer->current_token->data, "Ifj") == 0) {
        get_token(lexer, file); // consume 'Ifj' keyword
        if (peek_token(lexer, file).type == TOKEN_DOT) {
            get_token(lexer, file); // consume '.'
            if (peek_token(lexer, file).type == TOKEN_EOL) {
                get_token(lexer, file); // consume EOL
            }
            get_token(lexer, file); // consume method name
            // Проверяем, что это действительно встроенная функция
            if (!is_builtin(lexer->current_token->data)) {
                printf("Unknown method name: %s\n", lexer->current_token->data);
                exit(25);
            }
            //! Исправить эту хрень
            // Формируем полное имя встроенной функции, для красивой записи в AST
            char full_function_name[100];
            snprintf(full_function_name,
                sizeof(full_function_name),
                "Ifj.%s",
                lexer->current_token->data);

            AstNode *builtin_func_name = ast_new_id_node(NODE_ID, lexer->current_token->line, full_function_name);

            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after method name.\n");
                exit(25);
            }
            get_token(lexer, file); // consume '('

            // Здесь обработка параметров метода
            AstNode *list_args = list_of_tersms(lexer, file);



            if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
                printf("Expected ')' after method parameters.\n");
                exit(25);
            }
            get_token(lexer, file); // consume ')'
            // Создаем узлы для вызова встроенной функции
            ast_node_add_child(assignment_node, builtin_func_name);
            ast_node_add_child(assignment_node, list_args);

            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after method call.\n");
                exit(25);
            }
            get_token(lexer, file); // consume EOL
            return;
        }
    }
    if (peek_token(lexer, file).type == TOKEN_IDENTIFIER || peek_token(lexer, file).type == TOKEN_GLOBAL_IDENTIFIER) {
        // Token identifier = get_token(lexer, file); // consume identifier
        if (peek_next_token(lexer, file).type == TOKEN_OPEN_PAREN) {
            get_token(lexer, file); // consume identifier
            AstNode *builtin_func_name = ast_new_id_node(NODE_ID, lexer->current_token->line, lexer->current_token->data);
            // Здесь можно добавить обработку параметров функции
            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after function name.\n");
                exit(25);
            }
            get_token(lexer, file); // consume '('
            AstNode *list_args = list_of_tersms(lexer, file);

            if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
                printf("Expected ')' after function parameters.\n");
                exit(25);
            }
            get_token(lexer, file); // consume ')'
            ast_node_add_child(assignment_node, builtin_func_name);
            ast_node_add_child(assignment_node, list_args);
            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after function call.\n");
                exit(25);
            }
            get_token(lexer, file); // consume EOL
            return;
        }
        // выражение, которое начинается с идентификатора
        else {
            if (!parser_expression(lexer, file, assignment_node)) {
                printf("Invalid expression on right side of assignment.\n");
                exit(25);
            }
            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after expression.\n");
                exit(25);
            }
            get_token(lexer, file); // consume EOL
        }
    }
    // обычное выражение
    else {
        if (!parser_expression(lexer, file, assignment_node)) {
            printf("Invalid expression on right side of assignment.\n");
            exit(25);
        }
        if (peek_token(lexer, file).type != TOKEN_EOL) {
            printf("Expected end of line after expression.\n");
            exit(25);
        }
        get_token(lexer, file); // consume EOL
    }
}

void operations_function(Lexer *lexer, FILE *file, AstNode *block_node) {
    switch (peek_token(lexer, file).type) {
        // вместо 
    case TOKEN_IDENTIFIER:
    case TOKEN_GLOBAL_IDENTIFIER: {
        Token identifier = get_token(lexer, file); // consume identifier
        if (peek_token(lexer, file).type == TOKEN_ASSIGN) {
            // создаем узел присваивания
            AstNode *assignment_node = ast_node_create(NODE_ASSIGNMENT, lexer->current_token->line);
            ast_node_add_child(block_node, assignment_node);
            // создаем узел идентификатора для левой части
            AstNode *id_node = ast_new_id_node(NODE_ID, identifier.line, identifier.data);
            ast_node_add_child(assignment_node, id_node);

            get_token(lexer, file); // consume '='

            // обрабатываем правую часть выражения
            right_side_expression(lexer, file, assignment_node);

        }
        else {
            printf("Expected '=' after identifier in assignment.\n");
            exit(25);
        }
        break;
    }
    case TOKEN_KEYWORD:
        if (strcmp(lexer->current_token->data, "return") == 0) {
            get_token(lexer, file); // consume 'return' keyword

            AstNode *return_node = ast_node_create(NODE_RETURN, lexer->current_token->line);
            ast_node_add_child(block_node, return_node);

            // Пустой return
            if (peek_token(lexer, file).type == TOKEN_EOL) {
                get_token(lexer, file); // consume EOL
                break;
            }


            // return null
            if (peek_token(lexer, file).type == TOKEN_KEYWORD &&
                strcmp(lexer->current_token->data, "null") == 0) {
                get_token(lexer, file); // consume 'null' keyword

                // создаем узел литерала null
                AstNode *null_node = ast_node_create(NODE_LITERAL_NULL, lexer->current_token->line);
                ast_node_add_child(return_node, null_node);

                if (peek_token(lexer, file).type != TOKEN_EOL) {
                    printf("Expected end of line after 'return null'.\n");
                    exit(25);
                }
                get_token(lexer, file); // consume EOL
                return;
            }
            // return expression
            if (parser_expression(lexer, file, return_node)) {
                // Успешно разобрали выражение
            }
            else {
                printf("Invalid expression after 'return' keyword.\n");
                exit(25);
            }
            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after return expression.\n");
                exit(25);
            }
            get_token(lexer, file); // consume EOL
            break;
        }
        if (strcmp(lexer->current_token->data, "if") == 0) {
            get_token(lexer, file); // consume 'if' keyword

            AstNode *if_node = ast_node_create(NODE_IF, lexer->current_token->line);
            ast_node_add_child(block_node, if_node);

            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after 'if' keyword.\n");
                exit(25);
            }

            // скобки обрабатываются в parser_expression
            if (!parser_expression(lexer, file, if_node)) {
                printf("Invalid expression in 'if' condition.\n");
                exit(25);
            }


            function_block(lexer, file, if_node);

            // обработка else блока (по заданию, но можно изменить)
            if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
                strcmp(lexer->current_token->data, "else") != 0) {
                printf("Expected 'else' keyword after 'if' block.\n");
                exit(25);
            }
            else {
                get_token(lexer, file); // consume 'else' keyword

                function_block(lexer, file, if_node);
            }
            break;
        }
        if (strcmp(lexer->current_token->data, "while") == 0) {
            get_token(lexer, file); // consume 'while' keyword

            AstNode *while_node = ast_node_create(NODE_WHILE, lexer->current_token->line);
            ast_node_add_child(block_node, while_node);

            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after 'while' keyword.\n");
                exit(25);
            }
            // скобки обрабатываются в parser_expression
            if (!parser_expression(lexer, file, while_node)) {
                printf("Invalid expression in 'while' condition.\n");
                exit(25);
            }

            function_block(lexer, file, while_node);
            break;
        }
        if (strcmp(lexer->current_token->data, "var") == 0) {
            get_token(lexer, file); // consume 'var' keyword

            // Проверяем чтобы была id
            if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
                printf("Expected variable name after 'var' keyword.\n");
                exit(25);
            }

            get_token(lexer, file); // consume variable name

            AstNode *var_node = ast_new_id_node(NODE_VAR_DEF, lexer->current_token->line, lexer->current_token->data);
            ast_node_add_child(block_node, var_node);

            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after variable declaration.\n");
                exit(25);
            }
            get_token(lexer, file); // consume EOL
            break;
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
        break;
    case TOKEN_EOF:
        printf("Unexpected end of file inside function body.\n");
        exit(25);
    default:
        return;
    }
}

void function_block(Lexer *lexer, FILE *file, AstNode *func_node) {
    if (peek_token(lexer, file).type != TOKEN_OPEN_BRACE) {
        printf("Expected '{' to open function body.\n");
        exit(25);
    }
    get_token(lexer, file); // consume '{'
    //! расширение EOL после {
    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after '{' in function body.\n");
        exit(25);
    }
    get_token(lexer, file); // consume EOL

    AstNode *block_node = ast_node_create(NODE_BLOCK, lexer->current_token->line);
    // Добавляем block_node в дерево AST здесь
    ast_node_add_child(func_node, block_node);

    // Здесь обработка тела функции
    while (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        operations_function(lexer, file, block_node);
    }
    // Обработка EOL перед '}' проверяется в operations_function

    if (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        printf("Expected '}' to close function body.\n");
        exit(25);
    }
    get_token(lexer, file); // consume '}'
}

void parameters_function(Lexer *lexer, FILE *file, AstNode *param_list) {
    // Условие задания, после скобки может быть EOL (они не проверяют, но пусть будет)
    if (peek_token(lexer, file).type == TOKEN_EOL) {
        get_token(lexer, file); // consume EOL
    }
    // Здесь можно добавить обработку параметров функции
    if (peek_token(lexer, file).type == TOKEN_IDENTIFIER) {
        get_token(lexer, file); // consume parameter identifier

        AstNode *param_node = ast_new_id_node(NODE_PARAM, lexer->current_token->line, lexer->current_token->data);
        // Добавляем param_node в дерево AST здесь
        ast_node_add_child(param_list, param_node);

        // Обработка дополнительных параметров
        while (peek_token(lexer, file).type == TOKEN_COMMA) {
            get_token(lexer, file); // consume ','
            if (peek_token(lexer, file).type == TOKEN_EOL) {
                get_token(lexer, file); // consume EOL
            }
            if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
                printf("Expected parameter identifier after ','.\n");
                exit(25);
            }
            get_token(lexer, file); // consume parameter identifier

            AstNode *param_node = ast_new_id_node(NODE_PARAM, lexer->current_token->line, lexer->current_token->data);
            // Добавляем param_node в дерево AST здесь
            ast_node_add_child(param_list, param_node);

        }
    }
}

void tail_function(Lexer *lexer, FILE *file, AstNode *func_node) {
    if (peek_token(lexer, file).type == TOKEN_OPEN_PAREN) {
        get_token(lexer, file); // consume '('

        AstNode *param_list = ast_node_create(NODE_PARAM_LIST, lexer->current_token->line);
        // Добавляем param_list в дерево AST здесь
        ast_node_add_child(func_node, param_list);

        // Здесь можно добавить обработку параметров функции
        parameters_function(lexer, file, param_list);

        if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
            printf("Expected ')' after function parameters.\n");
            exit(25);
        }
        get_token(lexer, file); // consume ')'
    }
    if (peek_token(lexer, file).type == TOKEN_EOL) {
        printf("Unexpected end of line after function header.\n");
        exit(25);
    }
    // Здесь переходим к обработке тела функции
    function_block(lexer, file, func_node);
}

AstNode *name_function(Lexer *lexer, FILE *file) {
    get_token(lexer, file); // consume 'static' keyword

    // Проверяем чтобы была id
    if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
        printf("Expected function name identifier after 'function' keyword.\n");
        exit(25);
    }
    AstNode *func_name_node;
    //! Проверка на malloc
    Token func_name_token = get_token(lexer, file); // consume function name    

    // Определяем тип функции по следующему токену
    if (peek_token(lexer, file).type == TOKEN_OPEN_PAREN) {
        func_name_node = ast_new_id_node(NODE_FUNCTION_DEF, lexer->current_token->line, func_name_token.data);
        ast_node_add_child(program, func_name_node);
    }
    else if (peek_token(lexer, file).type == TOKEN_OPEN_BRACE) {
        func_name_node = ast_new_id_node(NODE_GETTER_DEF, lexer->current_token->line, func_name_token.data);
        ast_node_add_child(program, func_name_node);
    }
    else if (peek_token(lexer, file).type == TOKEN_ASSIGN) {
        func_name_node = ast_new_id_node(NODE_SETTER_DEF, lexer->current_token->line, func_name_token.data);
        ast_node_add_child(program, func_name_node);
        get_token(lexer, file); // consume '='
    }
    // Ошибка, если не один из ожидаемых токенов
    else {
        printf("Expected '(' or '{' or '=' after function name.\n");
        exit(25);
    }
    return func_name_node;
}

void parser_function_definition(Lexer *lexer, FILE *file) {
    // Проверяем ввод имени функции и определяем тип функции, записывая его в узел AST
    AstNode *func_node = name_function(lexer, file);

    // Обрабатываем хвост функции (параметры и тело)
    tail_function(lexer, file, func_node);

    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after function definition.\n");
        exit(25);
    }
}

void parser_function_list(Lexer *lexer, FILE *file) {
    switch (peek_token(lexer, file).type) {
    case TOKEN_CLOSE_BRACE:
        return; // Базовый случай: конец списка функций
    case TOKEN_KEYWORD:
        if (strcmp(lexer->current_token->data, "static") == 0) {

            // Обработка статической функции
            parser_function_definition(lexer, file);
            return;
        }
        printf("Expected static or }\n");
        exit(25);
    case TOKEN_EOL:
        get_token(lexer, file); // пропустить пустые строки
        parser_function_list(lexer, file);
        break;
    default:
        printf("Unexpected token in function list: %s\n",
            lexer->current_token->data);
        exit(25);
    }
}


void parser_prolog(Lexer *lexer, FILE *file) {
    if (peek_token(lexer, file).type == TOKEN_EOF) {
        printf("Empty file.\n");
        exit(25);
    }
    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(lexer->current_token->data, "import") != 0) {
        printf("Expected 'import' keyword at the beginning.");
        exit(25);
    }

    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_STRING ||
        strcmp(lexer->current_token->data, "\"ifj25\"") != 0) {
        printf("Expected string literal after 'import' keyword.\n");
        exit(25);
    }

    get_token(lexer, file);

    if (peek_token(lexer, file).type == TOKEN_EOL) {
        printf("Expected 'for' keyword after import string literal.\n");
        exit(25);
    }

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(lexer->current_token->data, "for") != 0) {
        printf("Expected 'for' keyword after import string literal.\n");
        exit(25);
    }

    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(lexer->current_token->data, "Ifj") != 0) {
        printf("Expected identifier 'Ifj' after 'for' keyword.\n");
        exit(25);
    }

    get_token(lexer, file);
}

void parser_kostra(Lexer *lexer, FILE *file) {

    // Здесь будет дальнейшая реализация парсинга основной структуры программы
    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(lexer->current_token->data, "class") != 0) {
        printf("Expected 'class' keyword to start class definition.\n");
        exit(25);
    }

    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_IDENTIFIER ||
        strcmp(lexer->current_token->data, "Program") != 0) {
        printf("Expected 'Program' identifier after 'class' keyword.\n");
        exit(25);
    }

    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_OPEN_BRACE) {
        printf("Expected '{' to open class body.\n");
        exit(25);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after class body.\n");
        exit(25);
    }
    get_token(lexer, file);


    // Парсим список функций внутри класса

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
        exit(25);
    }
    //! исправить ошибку с EOF, без этого не работает, оштбка в free_lexer
    get_token(lexer, file);
}

void parser_run(FILE *file) {

    Lexer *lexer = lexer_init();
    if (lexer == NULL) {
        fprintf(stderr, "Error initializing lexer.\n");
        return;
    }

    // Для первой фразы import "ifj25" for Ifj
    parser_prolog(lexer, file);
    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after import statement.\n");
        exit(25);
    }

    get_token(lexer, file); // consume EOL

    // Основной парсер
    parser_kostra(lexer, file);

    ast_print_debug(program);
    ast_node_free_recursive(program);

    lexer_free(lexer);

    // ❗ don't fclose(stdin)
    return;
}
