/**
 * @file parser.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Implementace pro parser.h
 *
 * @author
 *     - Veronika Turbaievska (273123)
 */

#include "parser.h"
#include "expression.h"
#include "ast.h"
#include "error_codes.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

// Globální proměnná pro kořen AST
static AstNode *program = NULL;

#define MAX_BUFFER_SIZE 100

/**
 * @brief Zpracovává prolog programu (kontroluje přítomnost 'import "Ifj25" for Ifj')
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 */
static void parser_prolog(Lexer *lexer, FILE *file);

/**
 * @brief Zpracovává kostru programu (kontroluje přítomnost 'class Program { EOL ... }')
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 */
static void parser_kostra(Lexer *lexer, FILE *file);

/**
 * @brief Zpracovává seznam funkcí uvnitř třídy Program
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 */
static void parser_function_list(Lexer *lexer, FILE *file);

/**
 * @brief Zpracovává definici jedné funkce
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 */
static void parser_function_definition(Lexer *lexer, FILE *file);

/**
 * @brief Zpracovává jméno funkce (static id ...)
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @return AstNode* Ukazatel na uzel funkce
 */
static AstNode *name_function(Lexer *lexer, FILE *file);

/**
 * @brief Zpracovává "ocas" funkce (... ) { ... }
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @param func_node Ukazatel na uzel funkce
 */
static void tail_function(Lexer *lexer, FILE *file, AstNode *func_node);

/**
 * @brief Zpracovává parametry funkce (id, id, ...)
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @param param_list Ukazatel na uzel seznamu parametrů
 */
static void function_params(Lexer *lexer, FILE *file, AstNode *param_list);

/**
 * @brief Zpracovává tělo funkce { ... }
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @param func_node Ukazatel na uzel funkce
 */
static void function_block(Lexer *lexer, FILE *file, AstNode *func_node);

/**
 * @brief Zpracovává blok kódu { vše co je uvnitř různé operace ... }
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @param block_node Ukazatel na uzel bloku
 */
static void handle_blok_body(Lexer *lexer, FILE *file, AstNode *block_node);

/**
 * @brief Zpracovává jedno výraz v pravé části přiřazení (id = ... )
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @param block_node Ukazatel na uzel bloku
 */
static void right_side_expression(Lexer *lexer, FILE *file, AstNode *parent_node);

/**
 * @brief Zpracovává seznam termů ( term, term, ... )
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @return AstNode* Ukazatel na uzel seznamu argumentů
 */
static AstNode *handle_fun_call_params(Lexer *lexer, FILE *file);

/**
 * @brief Kontroluje, zda je funkce vestavěná Ifj
 * 
 * @param name Jméno funkce
 * @return true Pokud je funkce vestavěná
 */
static bool is_builtin(const char *name);

/**
 * @brief Najde odpovídající typ uzlu AST pro daný token
 * 
 * @param token Vstupní token
 */
static NodeType token_to_node(Token token);

/**
 * @brief Kontroluje, zda je token termínem (term)
 * 
 * @param token Vstupní token
 */
static bool is_term(Token token);

/**
 * @brief Přeskočí token EOL, pokud je
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 */
static void skip_EOL(Lexer *lexer, FILE *file);

/**
 * @brief Formátuje úplný název vestavěné funkce Ifj.něco
 * 
 * @param identifier Token identifikátoru funkce
 * @param output_buffer Buffer pro zápis úplného názvu
 * @param buffer_size Velikost bufferu
 */
static void format_full_function_name(Token identifier, char *output_buffer, size_t buffer_size);

/**
 * @brief Zpracovává identifikátor v přiřazení
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @param block_node Uzlů bloku, kam bude přidáno přiřazení
 */
static void handle_identifier (Lexer *lexer, FILE *file, AstNode *block_node);

/**
 * @brief Zpracovává volání vestavěné funkce Ifj.něco
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @param assignment_node Uzlů přiřazení, kam bude přidáno volání funkce
 */
static void handle_builtin_call(Lexer *lexer, FILE *file, AstNode *assignment_node);

/**
 * @brief Zpracovává volání uživatelské funkce id(...)
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @param assignment_node Uzlů přiřazení, kam bude přidáno volání funkce
 */
static void handle_function_call(Lexer *lexer, FILE *file, AstNode *assignment_node);

/**
 * @brief Zpracovává výraz v pravé části přiřazení
 * 
 * @param lexer Ukazatel na lexér
 * @param file Ukazatel na soubor
 * @param assignment_node Uzlů přiřazení, kam bude přidáno výraz
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
            return NODE_ID; // není operátor, ale term
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_EXP:
        case TOKEN_HEX:
            return NODE_LITERAL_NUM;
        case TOKEN_STRING:
            return NODE_LITERAL_STRING;
        default:
            exit(SYNTAX_ERROR); // Není operátor
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
    // Kontrolujeme přítomnost jména v seznamu vestavěných funkcí
    // Hledáme jméno v poli builtins
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
        get_token(lexer, file); // přečteme EOL
    }
}

static void format_full_function_name(Token identifier, char *output_buffer, size_t buffer_size) {
    snprintf(output_buffer, buffer_size, "Ifj.%s", identifier.data);
}

static void handle_identifier (Lexer *lexer, FILE *file, AstNode *block_node) {
    Token identifier = peek_token(lexer, file);
    if (peek_next_token(lexer, file).type == TOKEN_ASSIGN) {
        // vytvoříme uzel přiřazení
        AstNode *assignment_node = ast_node_create(NODE_ASSIGNMENT, peek_next_token(lexer, file).line);
        ast_node_add_child(block_node, assignment_node);
        // vytvoříme uzel identifikátoru pro levou část
        AstNode *id_node = ast_new_id_node(NODE_ID, identifier.line, identifier.data);
        ast_node_add_child(assignment_node, id_node);
        
        get_token(lexer, file); // přečteme identifikátor
        get_token(lexer, file); // přečteme '='

        // zpracováváme pravou část výrazu
        right_side_expression(lexer, file, assignment_node);

    } else {
        printf("Expected '=' after identifier in assignment.\n");
        get_token(lexer, file); // přečteme identifikátor
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
}

static void handle_builtin_call(Lexer *lexer, FILE *file, AstNode *assignment_node) {
    get_token(lexer, file); // přečteme klíčové slovo 'Ifj'
    if (peek_token(lexer, file).type == TOKEN_DOT) {
        get_token(lexer, file); // přečteme '.'

        skip_EOL(lexer, file);
        Token current_token = peek_token(lexer, file);
        // Kontrolujeme, že se jedná o vestavěnou funkci
        if (!is_builtin(current_token.data)) {
            printf("Unknown method name: %s\n", current_token.data);
            exit(SYNTAX_ERROR);
        }

        // Formátujeme úplný název vestavěné funkce pro hezčí zápis v AST
        char full_function_name[MAX_BUFFER_SIZE];
        format_full_function_name(current_token, full_function_name, sizeof(full_function_name));
        
        AstNode *node_statement = ast_node_create(NODE_CALL_STATEMENT, current_token.line);
        
        AstNode *builtin_func_name = ast_new_id_node(NODE_ID, current_token.line, full_function_name);

        get_token(lexer, file); // přečteme název metody

        if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
            printf("Expected '(' after method name.\n");
            get_token(lexer, file); // přečteme neplatný token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // přečteme '('

        // Zde zpracování parametrů metody
        AstNode *list_args = handle_fun_call_params(lexer, file);

        if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
            printf("Expected ')' after method parameters.\n");
            get_token(lexer, file); // přečteme neplatný token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // přečteme ')'

        // Vytváříme uzly pro volání vestavěné funkce
        ast_node_add_child(node_statement, builtin_func_name);
        ast_node_add_child(node_statement, list_args);
        ast_node_add_child(assignment_node, node_statement);

        if (peek_token(lexer, file).type != TOKEN_EOL) {
            printf("Expected end of line after method call.\n");
            get_token(lexer, file); // přečteme neplatný token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // přečteme EOL
        return;
    }
    else {
        printf("Expected '.' after 'Ifj'.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
}

static void handle_function_call(Lexer *lexer, FILE *file, AstNode *assignment_node) {
    Token identifier = peek_token(lexer, file);
    AstNode *node_statement = ast_node_create(NODE_CALL_STATEMENT, identifier.line);
    AstNode *node_id = ast_new_id_node(NODE_ID, identifier.line, identifier.data);
    get_token(lexer, file); // přečteme identifikátor
    
    // Zde můžeme přidat zpracování parametrů funkce
    if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
        printf("Expected '(' after function name.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // přečteme '('
    AstNode *list_args = handle_fun_call_params(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
        printf("Expected ')' after function parameters.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // přečteme ')'
    
    // Vytváříme uzly pro volání funkce 
    ast_node_add_child(node_statement, node_id);
    ast_node_add_child(node_statement, list_args);
    ast_node_add_child(assignment_node, node_statement);


    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after function call.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // přečteme EOL
    return;
}

static void handle_parser_expression(Lexer *lexer, FILE *file, AstNode *assignment_node) {
    if (!parser_expression(lexer, file, assignment_node)) {
        printf("Invalid expression on right side of assignment.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after expression.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // přečteme EOL
}

static AstNode *handle_fun_call_params(Lexer *lexer, FILE *file) {
    // Vytváříme uzel seznamu argumentů
    AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, peek_token(lexer, file).line);
    skip_EOL(lexer, file);
    Token current_token = peek_token(lexer, file);
    // Kontrolujeme, zda je token termín
    if (is_term(current_token)) {
        // První term
        NodeType node_type_term = token_to_node(current_token);
        AstNode *leaf_node = create_leaf_node(current_token, node_type_term);
        if (leaf_node == NULL) {
            exit(INTERNAL_ERROR); // Chyba alokace
        }
        get_token(lexer, file); // přečteme parametr
        // Přidáváme první term do seznamu argumentů
        ast_node_add_child(arg_list, leaf_node);
        // Následující termy oddělené čárkou
        while (peek_token(lexer, file).type == TOKEN_COMMA) {
            get_token(lexer, file); // přečteme ','   
            skip_EOL(lexer, file);
            Token current_token = peek_token(lexer, file);
            if (is_term(current_token) == false) {
                printf("Expected parameter identifier after ','.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            }
            NodeType node_type_term = token_to_node(current_token);
            AstNode *leaf_node = create_leaf_node(current_token, node_type_term);
            if (leaf_node == NULL) {
                return false; // Chyba alokace
            }
            get_token(lexer, file); // přečteme identifikátor parametru
            ast_node_add_child(arg_list, leaf_node);
        }
    }
    skip_EOL(lexer, file);
    return arg_list;
}

static void right_side_expression(Lexer *lexer, FILE *file, AstNode *assignment_node) {
    // Kontrolujeme volání vestavěné funkce Ifj.něco
    if (peek_token(lexer, file).type == TOKEN_KEYWORD &&
        strcmp(peek_token(lexer, file).data, "Ifj") == 0) {
        handle_builtin_call(lexer, file, assignment_node);
        return;
        }
    // Kontrolujeme volání uživatelské funkce id(...) nebo výraz začínající identifikátorem
    if (peek_token(lexer, file).type == TOKEN_IDENTIFIER || 
        peek_token(lexer, file).type == TOKEN_GLOBAL_IDENTIFIER) {   
        if (peek_next_token(lexer, file).type == TOKEN_OPEN_PAREN) {
            handle_function_call(lexer, file, assignment_node);
            return;
        }  
        // výraz, který začíná identifikátorem
        else  {
            handle_parser_expression(lexer, file, assignment_node);
            return;
        }
    }
    // běžný výraz
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
            get_token(lexer, file); // přečteme globální identifikátor
            get_token(lexer, file); // přečteme EOL
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
            // zpracování return
            AstNode *return_node = ast_node_create(NODE_RETURN, peek_token(lexer, file).line);
            ast_node_add_child(block_node, return_node);
            get_token(lexer, file); // přečteme klíčové slovo 'return'

            //? Prázdný return
            // if (peek_token(lexer, file).type == TOKEN_EOL) {
            //     get_token(lexer, file); // přečteme EOL
            //     break;
            // }

            // return výraz
            handle_parser_expression(lexer, file, return_node);
            break;
        }
        else if (strcmp(peek_token(lexer, file).data, "if") == 0) {
            
            AstNode *if_node = ast_node_create(NODE_IF, peek_token(lexer, file).line);
            ast_node_add_child(block_node, if_node);
            
            get_token(lexer, file); // přečteme klíčové slovo 'if'
            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after 'if' keyword.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            }
            
            // závorky jsou zpracovány v parser_expression
            if (!parser_expression(lexer, file, if_node)) {
                printf("Invalid expression in 'if' condition.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            }
            
            // zpracování if bloku
            function_block(lexer, file, if_node);
            
            // zpracování else bloku (podle zadání, ale lze změnit)
            if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
                strcmp(peek_token(lexer, file).data, "else") != 0) {
                printf("Expected 'else' keyword after 'if' block.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            } else {
                get_token(lexer, file); // přečteme klíčové slovo 'else'
                
                // zpracování else bloku
                function_block(lexer, file, if_node);
                if (peek_token(lexer, file).type != TOKEN_EOL) {
                    printf("Expected end of line after 'else' block.\n");
                    get_token(lexer, file); // přečteme neplatný token
                    exit(SYNTAX_ERROR);
                }
                get_token(lexer, file); // přečteme EOL
            }
            break;
        }
        else if (strcmp(peek_token(lexer, file).data, "while") == 0) {
            
            AstNode *while_node = ast_node_create(NODE_WHILE, peek_token(lexer, file).line);
            ast_node_add_child(block_node, while_node);
            
            get_token(lexer, file); // přečteme klíčové slovo 'while'
            if (peek_token(lexer, file).type != TOKEN_OPEN_PAREN) {
                printf("Expected '(' after 'while' keyword.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            }
            // závorky jsou zpracovány v parser_expression
            if (!parser_expression(lexer, file, while_node)) {
                printf("Invalid expression in 'while' condition.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            }
            
            // zpracování while bloku
            function_block(lexer, file, while_node);
            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after 'while' block.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            }
            get_token(lexer, file); // přečteme EOL
            break;
        }
        else if (strcmp(peek_token(lexer, file).data, "var") == 0) {
            get_token(lexer, file); // přečteme klíčové slovo 'var'

            // Kontrola následujícího tokenu - musí být identifikátor
            if (peek_token(lexer, file).type != TOKEN_IDENTIFIER && 
                peek_token(lexer, file).type != TOKEN_GLOBAL_IDENTIFIER) {
                printf("Expected variable name after 'var' keyword.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            }

            
            AstNode *var_node = ast_new_id_node(NODE_VAR_DEF, peek_token(lexer, file).line, peek_token(lexer, file).data);
            ast_node_add_child(block_node, var_node);
            
            get_token(lexer, file); // přečteme název proměnné
            if (peek_token(lexer, file).type != TOKEN_EOL) {
                printf("Expected end of line after variable declaration.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            }
            get_token(lexer, file); // přečteme EOL
            break;
        }
        else {
            printf("Unexpected keyword in function body: %s\n", peek_token(lexer, file).data);
            get_token(lexer, file); // přečteme neplatný token
            exit(SYNTAX_ERROR);
        }
        break;
    case TOKEN_EOL:
        get_token(lexer, file); // přečteme EOL
        break;
    case TOKEN_CLOSE_BRACE:
        return; // end of function body
    case TOKEN_OPEN_BRACE:
        // Začátek vnořeného bloku
        function_block(lexer, file, block_node);
        // Po vnořeném bloku musí být EOL
        if (peek_token(lexer, file).type != TOKEN_EOL) {
            printf("Expected end of line after nested block.\n");
            get_token(lexer, file); // přečteme neplatný token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // přečteme EOL
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
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // přečteme '{'
    //? расширение EOL после {
    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after '{' in function body.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    
    AstNode *block_node = ast_node_create(NODE_BLOCK, peek_token(lexer, file).line);
    get_token(lexer, file); // přečteme EOL
    
    // Přidáme block_node do stromu AST zde
    ast_node_add_child(func_node, block_node);

    // Zde zpracujeme tělo funkce
    while (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        handle_blok_body(lexer, file, block_node);
    }
    // Zpracování EOL před '}' je kontrolováno v handle_blok_body

    if (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        printf("Expected '}' to close function body.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // přečteme '}'
}

static void function_params(Lexer *lexer, FILE *file, AstNode *param_list) {
    // Podmínka zadání: za závorkou může být EOL (není kontrolováno, ale nechme to tak)
    skip_EOL(lexer, file);
    // Zde můžeme přidat zpracování parametrů funkce
    if (peek_token(lexer, file).type == TOKEN_IDENTIFIER) {
        get_token(lexer, file); // přečteme identifikátor parametru

        AstNode *param_node = ast_new_id_node(NODE_PARAM, peek_token(lexer, file).line, peek_token(lexer, file).data);
        // Přidáme param_node do stromu AST zde
        ast_node_add_child(param_list, param_node);
        
        // Zpracování dalších parametrů
        while (peek_token(lexer, file).type == TOKEN_COMMA) {
            get_token(lexer, file); // přečteme ','
            if (peek_token(lexer, file).type == TOKEN_EOL) {
                get_token(lexer, file); // přečteme EOL
            }
            if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
                printf("Expected parameter identifier after ','.\n");
                get_token(lexer, file); // přečteme neplatný token
                exit(SYNTAX_ERROR);
            }
            get_token(lexer, file); // přečteme identifikátor parametru

            AstNode *param_node = ast_new_id_node(NODE_PARAM, lexer->current_token->line, lexer->current_token->data);
            // Přidáme param_node do stromu AST zde
            ast_node_add_child(param_list, param_node);
        }
    }
}

static void tail_function(Lexer *lexer, FILE *file, AstNode *func_node) {
    if (peek_token(lexer, file).type == TOKEN_OPEN_PAREN) {
        get_token(lexer, file); // přečteme '('

        AstNode *param_list = ast_node_create(NODE_PARAM_LIST, peek_token(lexer, file).line);
        // Přidáme param_list do stromu AST zde
        ast_node_add_child(func_node, param_list);

        // Zde můžeme přidat zpracování parametrů funkce
        function_params(lexer, file, param_list);

        if (peek_token(lexer, file).type != TOKEN_CLOSE_PAREN) {
            printf("Expected ')' after function parameters.\n");
            get_token(lexer, file); // přečteme neplatný token
            exit(SYNTAX_ERROR);
        }
        get_token(lexer, file); // přečteme ')'
    }
    if (peek_token(lexer, file).type == TOKEN_EOL) {
        printf("Unexpected end of line after function header.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    // Zde přecházíme k zpracování těla funkce
    function_block(lexer, file, func_node);
}

static AstNode *name_function(Lexer *lexer, FILE *file) {
    get_token(lexer, file); // přečteme klíčové slovo 'static'

    // Kontrola, zda je následující token identifikátor
    if (peek_token(lexer, file).type != TOKEN_IDENTIFIER) {
        printf("Expected function name identifier after 'function' keyword.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    AstNode *func_name_node;
    // Uložíme jméno funkce
    Token func_name_token = peek_token(lexer, file);

    // Určíme typ funkce podle následujícího tokenu
    // foo() - běžná funkce
    if (peek_next_token(lexer, file).type == TOKEN_OPEN_PAREN) {
        func_name_node = ast_new_id_node(NODE_FUNCTION_DEF, func_name_token.line, func_name_token.data);
        ast_node_add_child(program, func_name_node);
    }
    // foo{} - getter
    else if (peek_next_token(lexer, file).type == TOKEN_OPEN_BRACE) {
        func_name_node = ast_new_id_node(NODE_GETTER_DEF, func_name_token.line, func_name_token.data);
        ast_node_add_child(program, func_name_node);
    }
    // foo = ... - setter
    else if (peek_next_token(lexer, file).type == TOKEN_ASSIGN) {
        func_name_node = ast_new_id_node(NODE_SETTER_DEF, func_name_token.line, func_name_token.data);
        ast_node_add_child(program, func_name_node);
        get_token(lexer, file); // přečteme '='
    }
    // Chyba, pokud není jeden z očekávaných tokenů
    else {
        printf("Expected '(' or '{' or '=' after function name.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // přečteme jméno funkce
    return func_name_node;
}

static void parser_function_definition(Lexer *lexer, FILE *file) {
    // Kontrola zadání jména funkce a určení typu funkce, zapisujeme do uzlu AST
    AstNode *func_node = name_function(lexer, file);

    // Zpracování "ocasu" funkce (parametry a tělo funkce)
    tail_function(lexer, file, func_node);

    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after function definition.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
}

static void parser_function_list(Lexer *lexer, FILE *file) {
    switch (peek_token(lexer, file).type) {
    case TOKEN_CLOSE_BRACE:
        return; // Základní případ: konec seznamu funkcí
    case TOKEN_KEYWORD:
        if (strcmp(peek_token(lexer, file).data, "static") == 0) {
            
            // Zpracování funkce
            parser_function_definition(lexer, file);
            return;
        }
        printf("Expected static or }\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    case TOKEN_EOL:
        get_token(lexer, file); // přeskočíme prázdné řádky
        parser_function_list(lexer, file);
        break;
    default:
        printf("Unexpected token in function list: %s\n",
               peek_token(lexer, file).data);
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
}

static void parser_prolog(Lexer *lexer, FILE *file) {
    if (peek_token(lexer, file).type == TOKEN_EOF) {
        printf("Empty file.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    skip_EOL(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(peek_token(lexer, file).data, "import") != 0) {
        printf("Expected 'import' keyword at the beginning.");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_STRING ||
        strcmp(peek_token(lexer, file).data, "ifj25") != 0) {
        printf("Expected string literal after 'import' keyword.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type == TOKEN_EOL) {
        printf("Expected 'for' keyword after import string literal.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(peek_token(lexer, file).data, "for") != 0) {
        printf("Expected 'for' keyword after import string literal.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(peek_token(lexer, file).data, "Ifj") != 0) {
        printf("Expected identifier 'Ifj' after 'for' keyword.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);
}

static void parser_kostra(Lexer *lexer, FILE *file) {

    // Zde bude uložen kořenový uzel AST
    // a bude pokračovat implementace parsování hlavní struktury programu.
    if (peek_token(lexer, file).type != TOKEN_KEYWORD ||
        strcmp(peek_token(lexer, file).data, "class") != 0) {
        printf("Expected 'class' keyword to start class definition.\n"); 
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_IDENTIFIER ||
        strcmp(peek_token(lexer, file).data, "Program") != 0) {
        printf("Expected 'Program' identifier after 'class' keyword.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_OPEN_BRACE) {
        printf("Expected '{' to open class body.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    if (peek_token(lexer, file).type != TOKEN_EOL) {
        printf("Expected end of line after class body.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file);

    // Parsujeme seznam funkcí uvnitř třídy Program
    // Přidáváme kořenový uzel programu do stromu AST zde
    program = ast_node_create(NODE_PROGRAM, lexer->current_token->line);

    // Rekurzivně parsujeme funkce až do uzavírací závorky třídy
    while (peek_token(lexer, file).type != TOKEN_CLOSE_BRACE) {
        parser_function_list(lexer, file);
    }
    get_token(lexer, file);

    // Kontrola konce souboru po definici třídy
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

    // Pro první frázi import "ifj25" for Ifj
    parser_prolog(lexer, file);
    Token current = peek_token(lexer, file);
    if (current.type != TOKEN_EOL) {
        printf("Expected end of line after import statement.\n");
        get_token(lexer, file); // přečteme neplatný token
        exit(SYNTAX_ERROR);
    }
    get_token(lexer, file); // přečteme EOL
    // Hlavní parser
    parser_kostra(lexer, file);
    lexer_free(lexer);

    return program;
}