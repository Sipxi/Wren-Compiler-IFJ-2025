/**
 * @file ast.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Definice struktury a funkcí pro abstraktní syntaktický strom (AST).
 *
 * @author
 *     - Mykhailo Tarnavskyi (272479)
 */

#ifndef AST_H
#define AST_H

#include "symtable.h"
#include "utils.h"

/**
 * Typy uzlů v AST.
 * Rozlišují se strukturální uzly (definice funkcí), příkazy a výrazy
 */
typedef enum {
    //* 1. Strukturální uzly

    /** Kořenový uzel celého stromu.
     * child -> První definice funkce (NODE_FUNCTION_DEF) */
    NODE_PROGRAM,

    /** Definice uživatelské funkce: static id (...) { ... }
     * child -> NODE_PARAM_LIST (seznam parametrů)
     * child->sibling -> NODE_BLOCK (tělo funkce) */
    NODE_FUNCTION_DEF,

    /** Definice setteru: static id = (id) { ... }
     * child -> NODE_PARAM (jeden parametr)
     * child->sibling -> NODE_BLOCK (tělo funkce) */
    NODE_SETTER_DEF,

    /** Definice getteru: static id { ... }
     * child -> NODE_BLOCK (tělo funkce) */
    NODE_GETTER_DEF,

    /** Kontejner pro seznam parametrů funkce.
     * child -> První parametr (NODE_PARAM) */
    NODE_PARAM_LIST,

    /** Jeden parametr funkce.
     * Ukládá identifikátor parametru.
     * sibling -> Další parametr (NODE_PARAM) */
    NODE_PARAM,

    /** Blok kódu uzavřený ve složených závorkách { ... }.
     * child -> První příkaz v bloku */
    NODE_BLOCK,

    //* 2. Uzly příkazů (Statements)

    /** Podmíněný příkaz if-else.
     * child -> Podmínka (výraz)
     * child->sibling -> Větev if (NODE_BLOCK)
     * child->sibling->sibling -> Větev else (NODE_BLOCK, volitelně NULL) */
    NODE_IF,

    /** Cyklus while.
     * child -> Podmínka (výraz)
     * child->sibling -> Tělo cyklu (NODE_BLOCK) */
    NODE_WHILE,

    /** Definice lokální proměnné (var id).
     * Ukládá identifikátor proměnné. */
    NODE_VAR_DEF,

    /** Return z funkce.
     * child -> Návratová hodnota */
    NODE_RETURN,

    /** Přiřazení hodnoty (id = ...).
     * child -> Levá strana (NODE_ID)
     * child->sibling -> Pravá strana (výraz) */
    NODE_ASSIGNMENT,

    /** Samostatné volání funkce jako příkaz.
     * child -> Název funkce (NODE_ID)
     * child->sibling -> Seznam argumentů (NODE_ARGUMENT_LIST) */
    NODE_CALL_STATEMENT,

    /** Kontejner pro seznam argumentů při volání funkce.
     * child -> První argument (výraz) */
    NODE_ARGUMENT_LIST,

    //* 3. Uzly výrazů (Expressions)

    /** Binární operátory:
     * child -> Levý operand
     * child->sibling -> Pravý operand */

    // Aritmetické
    NODE_OP_PLUS,
    NODE_OP_MINUS,
    NODE_OP_MUL,
    NODE_OP_DIV,

    // Relační
    NODE_OP_LT,
    NODE_OP_GT,
    NODE_OP_LTE,
    NODE_OP_GTE,

    // Rovnost
    NODE_OP_EQ,
    NODE_OP_NEQ,

    // Kontrola typu
    NODE_OP_IS, // is()

    //* 4. Listové uzly (Termy/Literály)

    /** Identifikátor (proměnná nebo funkce)
     * Ukládá název jako řetězec. */
    NODE_ID,

    /** Číselný literál 
     * Ukládá hodnotu jako double. */
    NODE_LITERAL_NUM,

    /** Řetězcový literál
     * Ukládá hodnotu jako řetězec. */
    NODE_LITERAL_STRING,

    // Literál null.
    NODE_LITERAL_NULL,

    /** Název datového typu (např. Num, String).
     * Používá se v operátoru 'is'. */
    NODE_TYPE_NAME
} NodeType;


/**
 * Struktura reprezentující obecný uzel AST.
 */
typedef struct AstNode {
    NodeType type;  // Typ uzlu
    int line_number; // Číslo řádku ve zdrojovém kódu

    struct AstNode *child; // Ukazatel na prvního potomka
    struct AstNode *sibling; // Ukazatel na následujícího sourozence

    // Specifická data pro listové uzly a definice.
    union {
        char *identifier; // pro: NODE_ID, NODE_VAR_DEF, NODE_PARAM, NODE_FUNCTION_DEF...
        double literal_num; // Pro: NODE_LITERAL_NUM
        char *literal_string; // Pro: NODE_LITERAL_STRING
    } data;

    // Datový typ výsledku výrazu
    DataType data_type;
    // Odkaz na záznam v tabulce symbolů
    TableEntry *table_entry;
} AstNode;


/**
 * Vytváří se nový uzel AST.
 * @param type Typ vytvářeného uzlu.
 * @param line_number Číslo řádku pro ladění.
 * @return Ukazatel na nový uzel.
 */
AstNode *ast_node_create(NodeType type, int line_number);

/**
 * Rekurzivně se uvolňuje paměť celého podstromu.
 * Uvolňuje se uzel, jeho potomci i sourozenci. Zároveň se uvolňují alokovaná data uvnitř uzlů (identifikátory, řetězce).
 * @param node Kořen podstromu k uvolnění.
 */
void ast_node_free_recursive(AstNode *node);

/**
 * Přidává se potomek k rodičovskému uzlu.
 * Pokud rodič nemá žádné potomky, stává se nový uzel prvním potomkem (child).
 * Pokud již potomky má, je nový uzel přidán na konec seznamu sourozenců (sibling).
 * @param parent Rodičovský uzel.
 * @param new_child Přidávaný potomek.
 */
void ast_node_add_child(AstNode *parent, AstNode *new_child);


/**
 * Vypisuje se struktura AST pro ladicí účely.
 * @param node Kořenový uzel pro výpis.
 */
void ast_print_debug(AstNode *node);



// Pomocné funkce pro vytváření uzlů (API pro parser)
AstNode* ast_new_id_node(NodeType type, int line, const char* id);

AstNode* ast_new_num_node(double value, int line);

AstNode* ast_new_string_node(const char* value, int line);

AstNode* ast_new_null_node(int line);

AstNode* ast_new_bin_op(NodeType type, int line, AstNode* left, AstNode* right);
#endif