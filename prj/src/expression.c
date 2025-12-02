/**
 * @file expression.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Implementace precedenční analýzy výrazů a tvorby AST
 *
 * @author
 *     - Veronika Turbaievska (273123)
 */

#include "expression.h"
#include "stack_precedence.h"
#include "precedence.h"
#include "ast.h"
#include "token.h"
#include "utils.h"
#include "error_codes.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>


/**
 * @brief Převádí typ tokenu na typ uzlu AST
 * 
 * @param token Vstupní token
 * @return NodeType Typ uzlu AST
 */
static NodeType token_type_to_node(Token token);

/**
 * @brief Převádí gramatický symbol na typ uzlu AST
 * 
 * @param symbol Vstupní gramatický symbol
 * @return NodeType Typ uzlu AST
 */
static NodeType grammar_symbol_to_node_type(GrammarSymbol symbol);

/**
 * @brief Skládá rukojeť na zásobníku do jednoho uzlu AST
 * 
 * @param stack Ukazatel na zásobník
 * @return true Redukce byla úspěšná
 * @return false Chyba při redukci
 */
static bool handle_reduce(PStack *stack);

/**
 * @brief Kontroluje, zda je třeba ignorovat EOL po určitých tokenech
 * 
 * @param token Vstupní token
 * @return true Pokud je třeba ignorovat EOL
 * @return false Jinak
 */
static bool ignore_eol (Token token);

static bool ignore_eol (Token token) {
    switch (token.type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MULTIPLY:
        case TOKEN_DIVISION:
        case TOKEN_LESS:
        case TOKEN_GREATER:
        case TOKEN_EQUAL_LESS:
        case TOKEN_EQUAL_GREATER:
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
            return true;
        case TOKEN_KEYWORD:
            if (strcmp(token.data, "is") == 0) return true;
            return false;
        default:
            return false;
    }
}

bool char_to_double(const char *str, double *out_value) {
    if (!str || !out_value) return false;

    errno = 0;
    char *endptr = NULL;
    double val = strtod(str, &endptr);

    // Kontrola chyb
    if (errno != 0) return false;              // chyba konverze
    if (endptr == str) return false;           // nic nerozpoznáno
    while (*endptr == ' ' || *endptr == '\t')  // přeskočit mezery
        endptr++;
    if (*endptr != '\0') return false;         // neplatné znaky na konci

    *out_value = val;
    return true;
}

static NodeType token_type_to_node(Token token) {
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
            if (strcmp(token.data, "Num") == 0 || 
                strcmp(token.data, "String") == 0 || 
                strcmp(token.data, "Null") == 0) {
                return NODE_TYPE_NAME; 
            }
            if (strcmp(token.data, "null") == 0) {
                return NODE_LITERAL_NULL;
            }
            exit(SYNTAX_ERROR); // Není operátor
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

static NodeType grammar_symbol_to_node_type(GrammarSymbol symbol) {
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
        default:        exit(SYNTAX_ERROR); // Chyba
    }
}

AstNode *create_leaf_node (Token token, NodeType node_type_term) {
    // Vytváří listový uzel AST v závislosti na typu tokenu
    AstNode *leaf_node;

    // Vytváří odpovídající uzel AST
    if (node_type_term == NODE_ID) {
        leaf_node = ast_new_id_node(node_type_term, token.line, token.data);
    } else if (node_type_term == NODE_LITERAL_NUM) {
        double num_value;
        if (!char_to_double(token.data, &num_value)) {
            return NULL; // Chyba konverze
        }
        leaf_node = ast_new_num_node(num_value, token.line);
    } else if (node_type_term == NODE_LITERAL_STRING) {
        leaf_node = ast_new_string_node(token.data, token.line);
    } else if (node_type_term == NODE_LITERAL_NULL) {
        leaf_node = ast_new_null_node(token.line);
    } else {
        return NULL; // Neznámý typ termu
    }
    return leaf_node;
}

static bool handle_reduce(PStack *stack) {
    PStackItem handle[MAX_RULE_LENGTH];
    int count = 0;
    // Vytahujeme 3 prvky (protože maximální délka pravidla je 3)
    for (int iter = 0; iter < MAX_RULE_LENGTH-1; iter++) {
        if (PSTACK_is_empty(stack)) {
            // Chyba: dosáhli jsme $
            return false; 
        }
        PStackItem item = PSTACK_pop(stack);
        // Zapamatujeme prvek ručky
        handle[count++] = item;
    }
    AstNode *new_ast_node = NULL;
    bool rule_found = false;

    for (int rule_pos = 0; rule_pos < NUM_GRAMMAR_RULES; rule_pos++) {
        // Porovnáváme ručku s pravidlem, přeskočíme pravidla E -> i, protože pro ně se uzly AST vytvářejí přímo
        if (grammar_rules[rule_pos][1] == GS_TERM) {
            continue; 
        }
        // Získáme délku pravidla
        const int* rule = grammar_rules[rule_pos];
        int rule_length = 0;
        // Zjistíme skutečnou délku pravidla
        for (int idx = 1; idx < MAX_RULE_LENGTH; idx++) {
            if (rule[idx] == GS_UNDEF) break;
            rule_length++;
        }
        // Kontrolujeme délku ručky
        if (rule_length != count) {
            continue;
        }
        bool match = true;
        // Porovnáváme ručku s pravidly (v obráceném pořadí)
        for (int index = 0; index < count; index++) {
            if ((int) handle[index].symbol != rule[rule_length - index]) {
                match = false;
                break;
            }
        }
        if (match) {
            rule_found = true;
            if (rule_pos >= GR_RULE_E_OP_E_START && rule_pos <= GR_RULE_E_OP_E_END) {
                // E -> E op E
                AstNode *right_node = handle[0].ast_node;
                AstNode *left_node = handle[2].ast_node;
                Token op_token = handle[1].token;
                NodeType node_type = grammar_symbol_to_node_type(handle[1].symbol);
                new_ast_node = ast_new_bin_op(node_type, op_token.line, left_node, right_node);
                
            }
            else if (rule_pos == GR_RULE_E_IS_K) {
                // E -> E is k
                AstNode *left_node = handle[2].ast_node;
                AstNode *type_node = handle[0].ast_node; // Token "Num", "String", "Null"
                Token is_token = handle[1].token;

                NodeType node_type = NODE_OP_IS;
                new_ast_node = ast_node_create(node_type, is_token.line);
                if (new_ast_node == NULL) {
                    exit(INTERNAL_ERROR); // Chyba alokace
                }
                ast_node_add_child(new_ast_node, left_node);
                
                ast_node_add_child(new_ast_node, type_node);
            }
            else if (rule_pos == GR_RULE_PAREN_E) {
                // E -> ( E )
                new_ast_node = handle[1].ast_node;
            }

            break; // Pravidlo nalezeno, ukončíme smyčku
        }
    }
    if (!rule_found) {
        // Chyba: ručka neodpovídá žádnému pravidlu
        return false;
    }
    PStackItem result_item = {0}; // Inicializujeme nulami
    result_item.symbol = GS_E; // Výsledek redukce je vždy GS_E
    result_item.ast_node = new_ast_node; // S novým AST uzlem
    
    PSTACK_push(stack, result_item);

    return true; // Zástupný návrat
}

bool parser_expression(Lexer *lexer, FILE *file, AstNode *expr_node) {
    // Inicializace zásobníku
    PStack stack;
    PSTACK_init(&stack);

    // Vložíme $ na zásobník
    PStackItem dollar_item = { .symbol = GS_DOLLAR, .token.type = TOKEN_UNDEFINED, .ast_node = NULL };
    PSTACK_push(&stack, dollar_item);

    // Čteme první token
    Token current_token = peek_token(lexer, file);
    
    while (true) {
        // Získáme horní terminál zásobníku
        PStackItem *top_terminal_item = PSTACK_get_top_terminal(&stack);
        if (top_terminal_item == NULL) {
            return false; // Chyba
        }
        // Převádíme token na terminální index
        TermIndex stack_index = token_to_index(top_terminal_item->token);

        // Převádíme token na vstupní terminální index
        TermIndex input_index = token_to_index(current_token);


        // Podíváme se, co přišlo z tabulky precedencí
        char rule = get_precedence_rule(stack_index, input_index);

        if (stack_index == T_DOLLAR && input_index == T_DOLLAR) {
            break; // Úspěch!
        }

        if (rule == '<') {
            // Posun (Shift)
            PStackItem new_item;
            new_item.token = current_token;
            // Určujeme symbol gramatiky a vytváříme uzel AST, pokud je to potřeba
            if (input_index == T_TERM) {
                // Vytváříme AST uzel pro term
                // Určujeme typ AST uzlu
                NodeType node_type_term = token_type_to_node(current_token);
                AstNode *leaf_node = create_leaf_node(current_token, node_type_term);
                if (leaf_node == NULL) {
                    exit(INTERNAL_ERROR); // Chyba alokace
                }
                // Nastavujeme nový prvek zásobníku
                new_item.symbol = GS_E;
                new_item.ast_node = leaf_node;

            }
            // Typy (Num, String, Null)
            else if (input_index == T_TYPE) {
                // Typy nevytvářejí AST uzly v této fázi, ale potřebujeme je zaznamenat
                new_item.symbol = GS_TYPE;
                AstNode *type_node = ast_new_id_node(NODE_TYPE_NAME, current_token.line, current_token.data);
                if (type_node == NULL) {
                    exit(INTERNAL_ERROR); // Chyba alokace
                }
                new_item.ast_node = type_node;
            }
            else {
                // Operátory
                new_item.symbol = token_to_grammar_symbol(current_token);
                new_item.ast_node = NULL; // Operátory nevytvářejí AST uzly v této fázi
            }
            PSTACK_push(&stack, new_item);
            Token processed_token = current_token;
            current_token = peek_next_token(lexer, file);
            // Přeskočíme EOL po posunu, pokud je to potřeba (2 - EOL 4 - další token)
            if (ignore_eol(processed_token) && current_token.type == TOKEN_EOL) {
                get_token(lexer, file); // přečteme identifikátor
                get_token(lexer, file); // přečteme EOL
                current_token = peek_token(lexer, file);
            } else {
                get_token(lexer, file); // přečteme token
                current_token = peek_token(lexer, file);
            }
            
        }
        else if (rule == '=') {
            // Posun (Shift) pro závorky
            PStackItem new_item;
            new_item.token = current_token;
            // Závorky nevytvářejí AST uzly v této fázi
            new_item.symbol = token_to_grammar_symbol(current_token);
            new_item.ast_node = NULL;
            PSTACK_push(&stack, new_item);

            get_token(lexer, file); // přečteme token
            current_token = peek_token(lexer, file);
        }
        else if (rule == '>') {
            // Syntaktická redukce (Reduce)
            if (!handle_reduce(&stack)) {
                PSTACK_free(&stack);
                return false; // Chyba syntaxe
            }
        }
        else {
            PSTACK_free(&stack);
            return false; // Chyba syntaxe
        }
    }
    PStackItem final_item = PSTACK_pop(&stack); 
    if (final_item.symbol != GS_E) {
        // Pokud na vrcholu není GS_E, je to chyba
        PSTACK_free(&stack); 
        return false; 
    }

    PStackItem item = PSTACK_pop(&stack);

    // Kontrola 2: Je to opravdu $?
    if (item.symbol != GS_DOLLAR) {
        // Pokud pod GS_E nebylo $, je to chyba
        PSTACK_free(&stack);
        return false;
    }

    if (!PSTACK_is_empty(&stack)) {
        // Pokud pod $ něco ještě zůstalo, je to chyba
        PSTACK_free(&stack);
        return false;
    }
    // Úspěšný rozbor, přidáváme uzel do expr_node
    ast_node_add_child(expr_node, final_item.ast_node);
    PSTACK_free(&stack); 
    return true;
}