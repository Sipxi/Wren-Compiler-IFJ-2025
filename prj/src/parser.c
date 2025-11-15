#include "parser.h"
#include "expression.h"
#include "ast.h"
#include <stdio.h>
#include <string.h>

static void parameters_function(Lexer *lexer, FILE *file);
static void function_block(Lexer *lexer, FILE *file);
static void tail_function(Lexer *lexer, FILE *file);
static void name_function(Lexer *lexer, FILE *file);
static void parser_function_definition(Lexer *lexer, FILE *file);
static void parser_function_list(Lexer *lexer, FILE *file);
static void right_side_expression(Lexer *lexer, FILE *file);
static void operations_function(Lexer *lexer, FILE *file);

void list_of_tersms(Lexer *lexer, FILE *file) {
    if (is_term(peek_token(lexer, file))) {
        get_token(lexer, file); // consume parameter
        while (peek_token(lexer, file).type == TOKEN_COMMA) {
            get_token(lexer, file); // consume ','
            if (is_term(peek_token(lexer, file))== false) {
                printf("Expected parameter identifier after ','.\n");
                return;
            }
            get_token(lexer, file); // consume parameter identifier
        }
    }
}

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

void right_side_expression(Lexer *lexer, FILE *file) {
    if (peek_token(lexer, file).type == TOKEN_IDENTIFIER || peek_token(lexer, file).type == TOKEN_GLOBAL_IDENTIFIER) {
        Token identifier = get_token(lexer, file); // consume identifier
        if (peek_token(lexer, file).type == TOKEN_OPEN_PAREN) {
            get_token(lexer, file); // consume '('
            // Здесь можно добавить обработку параметров функции
            parameters_function(lexer, file);

            if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
                printf("Expected ')' after function parameters.\n");
                return;
            }
            get_token(lexer, file); // consume ')'
            return;
        }  
        else  {
            if (!parser_expression(lexer, file)) {
            printf("Invalid expression on right side of assignment.\n");
            return;
            }
        }
    }
    if (peek_token(lexer, file).type == TOKEN_KEYWORD &&
        strcmp(lexer->current_token->data, "Ifj") == 0) {
        get_token(lexer, file); // consume 'Ifj' keyword
        if (peek_token(lexer, file).type == TOKEN_DOT) {
            get_token(lexer, file); // consume '.'
            // if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
            //     printf("Expected method name after '.'.\n");
            //     return;
            // }
            get_token(lexer, file); // consume method name
            if (!is_builtin(lexer->current_token->data)) {
                printf("Unknown method name: %s\n", lexer->current_token->data);
                return;
            }
            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after method name.\n");
                return;
            }
            get_token(lexer, file); // consume '('
            // Здесь можно добавить обработку параметров метода
            list_of_tersms(lexer, file);

            if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
                printf("Expected ')' after method parameters.\n");
                return;
            }
            get_token(lexer, file); // consume ')'
            return;
        }
    }
    else {
        if (!parser_expression(lexer, file)) {
            printf("Invalid expression on right side of assignment.\n");
            return;
        }
    }
}

void operations_function(Lexer *lexer, FILE *file) {
    switch (peek_token(lexer, file).type) {
        // вместо или
    case TOKEN_IDENTIFIER:
    case TOKEN_GLOBAL_IDENTIFIER: 
        get_token(lexer, file); // consume identifier
        if (peek_token(lexer, file).type == TOKEN_ASSIGN) {
            get_token(lexer, file); // consume '='

            right_side_expression(lexer, file);

        } else {
            printf("Expected '=' after identifier in assignment.\n");
            return;
        }
        break;
    case TOKEN_KEYWORD:
        if (strcmp(lexer->current_token->data, "return") == 0) {
            get_token(lexer, file); // consume 'return' keyword
            if (peek_token(lexer, file).type == TOKEN_EOL) {
                // Пустой return
                return;
            }
            if (peek_token(lexer, file).type == TOKEN_KEYWORD &&
                strcmp(lexer->current_token->data, "null") == 0) {
                // Пустой return
                get_token(lexer, file); // consume 'null' keyword
                return;
            }
            if (parser_expression(lexer, file)) {
                // Успешно разобрали выражение
            } else {
                printf("Invalid expression after 'return' keyword.\n");
                return;
            };
            break;
        }
        if (strcmp(lexer->current_token->data, "if") == 0) {
            get_token(lexer, file); // consume 'if' keyword
            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after 'if' keyword.\n");
                return;
            }
            //get_token(lexer, file); // consume '('
            if (!parser_expression(lexer, file)) {
                printf("Invalid expression in 'if' condition.\n");
                return;
            }
            // if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
            //     printf("Expected ')' after 'if' condition.\n");
            //     return;
            // }
            //get_token(lexer, file); // consume ')'
            function_block(lexer, file);
            
            if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
                strcmp(lexer->current_token->data, "else") != 0) {
                printf("Expected 'else' keyword after 'if' block.\n");
            } else {
                get_token(lexer, file); // consume 'else' keyword
                function_block(lexer, file);
            }
            break;
        }
        if (strcmp(lexer->current_token->data, "while") == 0) {
            get_token(lexer, file); // consume 'while' keyword
            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after 'while' keyword.\n");
                return;
            }
            // get_token(lexer, file); // consume '('
            if (!parser_expression(lexer, file)) {
                printf("Invalid expression in 'while' condition.\n");
                return;
            }
            // if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
            //     printf("Expected ')' after 'while' condition.\n");
            //     return;
            // }
            // get_token(lexer, file); // consume ')'
            function_block(lexer, file);
            break;
        }
        if (strcmp(lexer->current_token->data, "var") == 0) {
            get_token(lexer, file); // consume 'var' keyword
            if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
                printf("Expected variable name after 'var' keyword.\n");
                return;
            }
            get_token(lexer, file); // consume variable name

            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after variable declaration.\n");
                return;
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
        function_block(lexer, file);
        break;
    case TOKEN_EOF:
        printf("Unexpected end of file inside function body.\n");
        return;
    default:
        return;
    }
}

void function_block(Lexer *lexer, FILE *file) {
    if (peek_token(lexer, file).type != TOKEN_OPEN_BRACE) {
        printf("Expected '{' to open function body.\n");
        return;
    }
    get_token(lexer, file); // consume '{'

    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after '{' in function body.\n");
        return;
    }
    get_token(lexer, file); // consume EOL

    // Здесь можно добавить обработку тела функции
    while (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        operations_function(lexer, file);
    }

    if (peek_token(lexer, file).type == TOKEN_EOL) {
        get_token(lexer, file); // consume EOL
    }

    if (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        printf("Expected '}' to close function body.\n");
        return;
    }
    get_token(lexer, file); // consume '}'
}

void parameters_function(Lexer *lexer, FILE *file) {
    // Здесь можно добавить обработку параметров функции
    if (peek_token(lexer, file).type == TOKEN_IDENTIFIER) {
        get_token(lexer, file); // consume parameter identifier
        while (peek_token(lexer, file).type == TOKEN_COMMA) {
            get_token(lexer, file); // consume ','
            if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
                printf("Expected parameter identifier after ','.\n");
                return;
            }
            get_token(lexer, file); // consume parameter identifier
        }
    }
}

void tail_function(Lexer *lexer, FILE *file) {
    if (peek_token(lexer, file).type == TOKEN_OPEN_PAREN) {
        get_token(lexer, file); // consume '('
        // Здесь можно добавить обработку параметров функции
        parameters_function(lexer, file);

        if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
            printf("Expected ')' after function parameters.\n");
            return;
        }
        get_token(lexer, file); // consume ')'
    }

    function_block(lexer, file);
}

void name_function(Lexer *lexer, FILE *file) {
    get_token(lexer, file); // consume 'static' keyword
    if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
        printf("Expected function name identifier after 'function' keyword.\n");
        return;
    }
    get_token(lexer, file); // consume function name

    if (peek_token(lexer, file).type == TOKEN_ASSIGN) {
        get_token(lexer, file); // consume '='
    }
}

void parser_function_definition(Lexer *lexer, FILE *file) {
    name_function(lexer, file);

    tail_function(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after function definition.\n");
        return;
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
        return;
    case TOKEN_EOL:
        get_token(lexer, file); // пропустить пустые строки
        parser_function_list(lexer, file);
        break;
    default:
        printf("Unexpected token in function list: %s\n",
               lexer->current_token->data);
        return;
    }
}


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


    // Парсим список функций внутри класса



    while (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        parser_function_list(lexer, file);
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