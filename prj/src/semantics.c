/**
 * @file semantics.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Implementace sémantické analýzy a typové kontroly.
 *
 * @author
 *     - Mykhailo Tarnavskyi (272479)
 */

#include "symtable.h"
#include "ast.h"
#include "semantics.h"
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <math.h>


// Implementace zásobníku rozsahů

#define HIERARCHY_STACK_SIZE 100

/**
 * Struktura představující zásobník ukazatelů na tabulky symbolů.
 * Slouží k uchování hierarchie vnořených bloků během průchodu AST.
 */
typedef struct {
    Symtable *array[HIERARCHY_STACK_SIZE];
    int topIndex;
} ScopeStack;

/**
 * Inicializuje zásobník pro správu vnořených rozsahů.
 * @param stack Ukazatel na zásobník.
 */
static void H_Stack_Init(ScopeStack *stack) {
    stack->topIndex = -1;
}

/**
 * Přidá tabulku symbolů na vrchol zásobníku.
 * V případě překročení kapacity zásobníku dojde k ukončení překladu s interní chybou.
 * @param stack Ukazatel na zásobník.
 * @param table Ukazatel na vkládanou tabulku.
 * @return true při úspěšném vložení.
 */
static bool H_Stack_Push(ScopeStack *stack, Symtable *table) {
    if (stack->topIndex >= HIERARCHY_STACK_SIZE - 1) {
        fprintf(stderr, "Internal Error: Scope stack overflow (too many nested blocks).\n");
        exit(99);
    }
    stack->topIndex++;
    stack->array[stack->topIndex] = table;
    return true;
}

/**
 * Odebere aktuální tabulku symbolů z vrcholu zásobníku.
 * @param stack Ukazatel na zásobník.
 * @return Odebíraná tabulka symbolů nebo NULL, je-li zásobník prázdný.
 */
static Symtable *H_Stack_Pop(ScopeStack *stack) {
    if (stack->topIndex == -1) {
        return NULL;
    }
    Symtable *table = stack->array[stack->topIndex];
    stack->topIndex--;
    return table;
}

/**
 * Zpřístupní aktuální tabulku symbolů na vrcholu zásobníku bez jejího odebrání.
 * @param stack Ukazatel na zásobník.
 * @return Aktuální tabulka symbolů.
 */
static Symtable *H_Stack_Peek(ScopeStack *stack) {
    if (stack->topIndex == -1) {
        return NULL;
    }
    return stack->array[stack->topIndex];
}

/**
 * Vyhledá proměnnou v hierarchii tabulek symbolů.
 * Postupuje od aktuálního (vnitřního) rozsahu směrem k vnějším (rodičovským).
 * Je zajištěno správné překrývání proměnných.
 * @param stack Zásobník rozsahů.
 * @param name Název hledané proměnné.
 * @return Nalezený záznam nebo záznam z globální tabulky, pokud existuje.
 */
static TableEntry *H_Stack_Find_Var(ScopeStack *stack, const char *name) {
    for (int i = stack->topIndex; i >= 0; i--) {
        TableEntry *entry = symtable_lookup(stack->array[i], name);
        if (entry != NULL) {
            return entry; 
        }
    }
    return symtable_lookup(&global_table, name);
}

static bool register_builtin_function(const char *name, int arity, DataType return_type);
static bool process_function_declaration(AstNode *func_node);
static bool analyze_function_body(AstNode *func_node);
static bool analyze_statement(AstNode *node, ScopeStack *stack, int *block_cnt, int current_scope_id);
static bool analyze_expression(AstNode *node, ScopeStack *stack, DataType *result_type);
static bool is_whole_number(double value);

Symtable global_table; 

/**
 * Spouští hlavní proces sémantické analýzy nad abstraktním syntaktickým stromem.
 * Řídí inicializaci globální tabulky, registraci vestavěných funkcí, průchod deklaracemi
 * uživatelských funkcí a následnou kontrolu jejich těl. Na závěr se ověřuje existence funkce main.
 * @param root Kořen AST stromu.
 * @return true při úspěšném dokončení analýzy.
 */
bool analyze_semantics(AstNode *root) {

    if (!symtable_init(&global_table)) {
        
        fprintf(stderr, "Internal Compiler Error: Failed to init global_table.\n");
        exit(99);
    }

    // Krok 1: Registrace vestavěných funkcí

    // static Ifj.read_str() -> String | Null
    if (!register_builtin_function("Ifj.read_str", 0, TYPE_STR)) exit(99);
    // static Ifj.read_num() -> Num | Null
    if (!register_builtin_function("Ifj.read_num", 0, TYPE_NUM)) exit(99);
    // static Ifj.write(term) -> Null
    if (!register_builtin_function("Ifj.write", 1, TYPE_NIL)) exit(99);
    // static Ifj.floor(term: Num) -> Num
    if (!register_builtin_function("Ifj.floor", 1, TYPE_NUM)) exit(99);
    // static Ifj.substring(s: String, i: Num, j: Num) -> String | Null
    if (!register_builtin_function("Ifj.substring", 3, TYPE_STR)) exit(99);
    // static Ifj.strcmp(s1: String, s2: String) -> Num
    if (!register_builtin_function("Ifj.strcmp", 2, TYPE_NUM)) exit(99);
    // static Ifj.ord(s: String, i: Num) -> Num
    if (!register_builtin_function("Ifj.ord", 2, TYPE_NUM)) exit(99);
    // static Ifj.chr(i: Num) -> String
    if (!register_builtin_function("Ifj.chr", 1, TYPE_STR)) exit(99);
    // static Ifj.str(term) -> String
    if (!register_builtin_function("Ifj.str", 1, TYPE_STR)) exit(99);
    // static Ifj.length(s: String) -> Num
    if (!register_builtin_function("Ifj.length", 1, TYPE_NUM)) exit(99);


    // Krok 2: Sběr deklarací uživatelských funkcí

    if (root == NULL || root->type != NODE_PROGRAM) {
        fprintf(stderr, "Internal Error: AST root is not NODE_PROGRAM.\n");
        symtable_free(&global_table);
        exit(99);
    }

    for (AstNode *node = root->child; node != NULL; node = node->sibling) {
        if (node->type == NODE_FUNCTION_DEF ||
            node->type == NODE_GETTER_DEF ||
            node->type == NODE_SETTER_DEF)
        {
            // Volání funkce pro registraci funkce
            if (!process_function_declaration(node)) {
                symtable_free(&global_table);
                exit(4);
            }
        }
    }

    // Krok 3: Analýza těl funkcí

    for (AstNode *node = root->child; node != NULL; node = node->sibling) {
        if (node->type == NODE_FUNCTION_DEF ||
            node->type == NODE_GETTER_DEF ||
            node->type == NODE_SETTER_DEF)
        {
            // Volání funkce pro zpracování těla funkce
            if (!analyze_function_body(node)) {
                symtable_free(&global_table);
                exit(10);
            }
        }
    }

    // Krok 4: Kontrola existence funkce main

    TableEntry *main_entry = symtable_lookup(&global_table, "main$0");

    if (main_entry == NULL) {
        fprintf(stderr, "Semantic Error: Function 'main()' is not defined.\n");
        symtable_free(&global_table);
        exit(3);
    }

    // Kontroluje se, zda byla funkce main nejen deklarována, ale i definována
    if (main_entry->data->is_defined == false) {
        fprintf(stderr, "Semantic Error: Function 'main()' is declared but not defined.\n");
        symtable_free(&global_table);
        exit(3);
    }

    return true;
}

/**
 * Ověřuje, zda hodnota s plovoucí řádovou čárkou představuje celé číslo.
 * @param value Hodnota k ověření.
 * @return true, pokud hodnota odpovídá celému číslu.
 */
static bool is_whole_number(double value) {
    const double EPSILON = 1e-9;
    double truncated_value = (double)((long long)value);
    double difference = value - truncated_value;
    return (difference < EPSILON) && (difference > -EPSILON);
}

/**
 * Generuje se unikátní interní název identifikátoru pro konkrétní rozsah platnosti.
 * Tato funkce řeší problematiku překrývání proměnných tím, že k původnímu
 * názvu připojí identifikátor bloku (např. "promenna$1").
 * @param original_name Původní název proměnné v AST.
 * @param scope_id Číselný identifikátor aktuálního bloku.
 * @return Ukazatel na nově alokovaný řetězec s unikátním názvem.
 */
static char *create_unique_name(const char *original_name, int scope_id) {
    size_t len = strlen(original_name) + 1 + 10 + 1;
    char *new_name = (char *)malloc(len);
    if (new_name) {
        sprintf(new_name, "%s$%d", original_name, scope_id);
    }
    return new_name;
}

/**
 * Zajišťuje se registrace vestavěné funkce jazyka do globální tabulky symbolů.
 * @param name Název vestavěné funkce.
 * @param arity Počet parametrů funkce.
 * @param return_type Návratový typ funkce.
 * @return true, pokud vložení do tabulky proběhlo úspěšně.
 */
static bool register_builtin_function(const char *name, int arity, DataType return_type) {
    char mangled_name[256];
    sprintf(mangled_name, "%s$%d", name, arity);


    SymbolData data;
    data.kind = KIND_FUNC;
    data.data_type = return_type; 
    data.is_defined = true;      
    data.unique_name = NULL;     

    if (!symtable_insert(&global_table, mangled_name, &data)) {
        fprintf(stderr, "Internal Error: Failed to insert builtin '%s'\n", mangled_name);
        exit(99);
    }

    TableEntry *entry = symtable_lookup(&global_table, mangled_name);
    if (entry) {
        entry->local_table = NULL;
    }

    return true;
}

/**
 * Zjišťuje se počet parametrů funkce z její definice v AST.
 * Prochází se seznam parametrů (NODE_PARAM_LIST). Pro speciální typy funkcí
 * (settery a gettery) se vrací fixní hodnoty dle specifikace jazyka.
 * @param func_node Uzel AST reprezentující definici funkce.
 * @return Celkový počet parametrů.
 */
static int count_parameters(AstNode *func_node) {
    if (func_node->type == NODE_SETTER_DEF) {
        return 1; // Setter má vždy jeden parametr
    }
    if (func_node->type == NODE_GETTER_DEF) {
        return 0; // Getter má vždy nula parametru
    }

    AstNode *param_list = func_node->child;
    if (param_list == NULL || param_list->type != NODE_PARAM_LIST) {
        return 0;
    }

    int arity = 0;
    for (AstNode *param = param_list->child; param != NULL; param = param->sibling) {
        if (param->type == NODE_PARAM) {
            arity++;
        }
    }
    return arity;
}

/**
 * Zpracovává se deklarace funkce v prvním průchodu.
 * Generuje se unikátní klíč funkce, kontroluje se duplicita názvů (redefinice) v globální tabulce
 * a inicializuje se lokální tabulka symbolů pro danou funkci.
 * @param func_node Uzel AST definice funkce.
 * @return true při úspěšném zpracování.
 */
static bool process_function_declaration(AstNode *func_node) {

    // Získává se identifikátor funkce z AST uzlu
    const char *name = func_node->data.identifier;
    if (name == NULL) {
        fprintf(stderr, "Internal Error: Function node has no name.\n");
        exit(99);
    }

    // Vypočítává se arita a generuje se unikátní klíč
    int arity = count_parameters(func_node);
    char mangled_name[256];
    if (func_node->type == NODE_SETTER_DEF) {
        sprintf(mangled_name, "%s$setter", name);
    }
    else if (func_node->type == NODE_GETTER_DEF) {
        sprintf(mangled_name, "%s$getter", name);
    }
    else {
        sprintf(mangled_name, "%s$%d", name, arity);
    }

    // Kontrola redefinice
    if (symtable_lookup(&global_table, mangled_name) != NULL) {
        fprintf(stderr, "Semantic Error (Line %d): Redefinition of function '%s'.\n",
            func_node->line_number, name);
        exit(4);
    }

    SymbolData data;
    data.kind = KIND_FUNC;
    data.data_type = TYPE_UNKNOWN; 
    data.is_defined = false;   
    data.unique_name = NULL; 

    // Vkládá se záznam do globální tabulky symbolů
    if (!symtable_insert(&global_table, mangled_name, &data)) {
        fprintf(stderr, "Internal Error: Failed to insert function '%s' into global_table.\n", name);
        exit(99);
    }


    // Zpětně se vyhledává záznam v tabulce pro získání stabilního ukazatele.
    TableEntry *func_entry = symtable_lookup(&global_table, mangled_name);
    if (func_entry == NULL) {
        fprintf(stderr, "Internal Error: Failed to re-lookup function '%s'.\n", name);
        exit(99);
    }

    // Alokuje a inicializuje se lokální tabulka symbolů (Level 1), která bude obsahovat parametry a lokální proměnné této funkce.
    func_entry->local_table = (Symtable *)malloc(sizeof(Symtable));
    if (func_entry->local_table == NULL) {
        fprintf(stderr, "Internal Error: Failed to malloc local_table for '%s'.\n", name);
        exit(99);
    }

    if (!symtable_init(func_entry->local_table)) {
        fprintf(stderr, "Internal Error: Failed to init local_table for '%s'.\n", name);
        free(func_entry->local_table);
        func_entry->local_table = NULL;
        exit(99);
    }

    // Propojuje se AST uzel s vytvořeným záznamem v tabulce symbolů
    func_node->table_entry = func_entry;

    return true;
}

/**
 * Provádí se sémantická analýza těla jedné funkce 
 * Připravuje se lokální kontext (zásobník rozsahů), zpracovávají se parametry
 * a následně se kontrolují jednotlivé příkazy v těle funkce.
 * @param func_node Uzel AST reprezentující definici funkce.
 * @return true, pokud analýza proběhne bez chyb.
 */
static bool analyze_function_body(AstNode *func_node)
{

   // Získává se odkaz na záznam funkce v globální tabulce symbolů
    TableEntry *func_entry = func_node->table_entry;
    if (func_entry == NULL) {
        fprintf(stderr, "Internal Error: func_node->table_entry is NULL for '%s'.\n", func_node->data.identifier);
        exit(99);
    }

    // Načítá se lokální tabulka symbolů, která je přiřazena této funkci.
    Symtable *func_local_table = func_entry->local_table;
    if (func_local_table == NULL) {
        fprintf(stderr, "Internal Error: func_local_table is NULL for '%s'.\n", func_node->data.identifier);
        exit(99);
    }

    // Inicializuje se zásobník rozsahů platnosti (Scope Stack).
    ScopeStack stack;
    H_Stack_Init(&stack);

    // Na vrchol zásobníku se vkládá lokální tabulka funkce.
    if (!H_Stack_Push(&stack, func_local_table)) {
        exit(99);
    }

    int local_block_cnt = 0; 
    int current_scope_id = 0; 

    // Určují se uzly pro seznam parametrů a tělo funkce
    AstNode *param_iter = NULL;
    AstNode *body_node = NULL;

    if (func_node->type == NODE_FUNCTION_DEF || func_node->type == NODE_SETTER_DEF) {
        AstNode* param_list = func_node->child;
        
        if (param_list != NULL && param_list->type == NODE_PARAM_LIST) {
            param_iter = param_list->child;   
            body_node = param_list->sibling;  
        } else {
            fprintf(stderr, "Internal Error: Function/Setter '%s' missing param list.\n", func_node->data.identifier);
            H_Stack_Pop(&stack);
            exit(99);
        }
    } 
    else { 
        // Getter nemá parametry
        param_iter = NULL;
        body_node = func_node->child;
    }

    // Zpracovávají se parametry funkce
    for (AstNode* param = param_iter; param != NULL; param = param->sibling) {
        
        if (param->type != NODE_PARAM) continue;

        const char* param_name = param->data.identifier;
        // Kontroluje se duplicita názvů parametrů
        if (symtable_lookup(func_local_table, param_name) != NULL) {
            fprintf(stderr, "Semantic Error (Line %d): Duplicate parameter name '%s'.\n",
                param->line_number, param_name);
            H_Stack_Pop(&stack);
            exit(4);
        }

        SymbolData data;
        data.kind = KIND_VAR;
        data.data_type = TYPE_UNKNOWN;;
        data.is_defined = true;

        data.unique_name = create_unique_name(param_name, 0); 

        if (!symtable_insert(func_local_table, param_name, &data)) {
            H_Stack_Pop(&stack);
            fprintf(stderr, "Internal Error: Failed to insert param '%s'.\n", param_name);
            exit(99);
        }

        // Propojuje se uzel AST s vytvořeným záznamem v tabulce.
        TableEntry *entry = symtable_lookup(func_local_table, param_name);
        if (entry == NULL) {
            H_Stack_Pop(&stack);
            fprintf(stderr, "Internal Error: Failed to re-lookup param '%s'.\n", param_name);
            exit(99);
        }
        entry->local_table = NULL;

        param->table_entry = entry;
    }

    // Zahajuje se analýza těla funkce.

    if (body_node == NULL || body_node->type != NODE_BLOCK) {
        fprintf(stderr, "Internal Error: Function '%s' has no NODE_BLOCK body.\n", func_node->data.identifier);
        H_Stack_Pop(&stack);
        exit(2);
    }
    // Prochází se jednotlivé příkazy v těle funkce.
    for (AstNode *stmt = body_node->child; stmt != NULL; stmt = stmt->sibling) {

        if (!analyze_statement(stmt, &stack, &local_block_cnt, current_scope_id)) {
            H_Stack_Pop(&stack);
            exit(10);
        }
    }

    // Funkce se označuje jako definovaná.
    func_entry->data->is_defined = true;

    H_Stack_Pop(&stack);

    return true;
}

// Interní čítač pro generování unikátních názvů bloků. (např. "__block_0", "__block_1")
static int block_counter = 0;

/**
 * Rekurzivně se analyzuje jeden příkaz (statement) v rámci těla funkce.
 * Funkce zpracovává vnořené bloky (vytváří nové tabulky symbolů), definice proměnných, přiřazení a řídicí konstrukce.
 * @param node Uzel AST reprezentující příkaz.
 * @param stack Zásobník rozsahů platnosti.
 * @param block_cnt Ukazatel na sdílený čítač bloků v rámci funkce.
 * @param current_scope_id Identifikátor aktuálního bloku (pro generování unikátních názvů proměnných).
 * @return true, pokud je příkaz sémanticky validní.
 */
static bool analyze_statement(AstNode *node, ScopeStack *stack, int *block_cnt, int current_scope_id)
{
    if (node == NULL) {
        return true;
    }

    switch (node->type) {
        
        // PŘÍPAD 1: Vnořený blok kódu: { ... }
    case NODE_BLOCK: {

        Symtable *parent_table = H_Stack_Peek(stack);
        if (parent_table == NULL) {
            fprintf(stderr, "Internal Error: Scope stack is empty inside NODE_BLOCK.\n");
            exit(99);
        }

        // Alokuje se a inicializuje nová tabulka symbolů pro vnořený blok (Level N+1)
        Symtable *new_block_table = (Symtable *)malloc(sizeof(Symtable));
        if (new_block_table == NULL) { exit(99); }

        if (!symtable_init(new_block_table)) {
            free(new_block_table);
            exit(99);
        }


        SymbolData data;
        data.kind = KIND_BLOCK;
        data.data_type = TYPE_NIL;
        data.is_defined = true;

        char block_key[256];
        sprintf(block_key, "__block_%d", block_counter++);

        if (!symtable_insert(parent_table, block_key, &data)) {
            symtable_free(new_block_table);
            free(new_block_table);
            exit(99);
        }

        // Propojuje se rodičovská tabulka s novou tabulkou bloku.
        TableEntry *block_entry = symtable_lookup(parent_table, block_key);
        block_entry->local_table = new_block_table;

        if (!H_Stack_Push(stack, new_block_table)) {
            exit(99);
        }

        (*block_cnt)++;
        int new_scope_id = *block_cnt;

        // Rekurzivně se analyzují všechny příkazy uvnitř tohoto bloku.
        bool result = true;
        for (AstNode *stmt = node->child; stmt != NULL; stmt = stmt->sibling) {
            if (!analyze_statement(stmt, stack, block_cnt, new_scope_id)) {
                result = false;
                break;
            }
        }

        H_Stack_Pop(stack);

        return result;
    }

        // PŘÍPAD 2: Definice lokální proměnné: (var id)
    case NODE_VAR_DEF: {

        const char *name = node->data.identifier;

        Symtable *current_table = H_Stack_Peek(stack);
        if (current_table == NULL) {
            exit(99);
        }

        // Kontroluje se redefinice proměnné v AKTUÁLNÍM rozsahu
        if (symtable_lookup(current_table, name) != NULL) {
            fprintf(stderr, "Semantic Error (Line %d): Redefinition of variable '%s' in the same scope.\n",
                node->line_number, name);
            exit(4);
        }

        SymbolData data;
        data.kind = KIND_VAR;
        data.data_type = TYPE_NIL;
        data.is_defined = true;
        data.unique_name = create_unique_name(name, current_scope_id); // generje se unikátní jméno (např. "x$2") pro potřeby generátoru kódu.

        if (!symtable_insert(current_table, name, &data)) {
            exit(99);
        }

        TableEntry *entry = symtable_lookup(current_table, name);
        entry->local_table = NULL; 
        node->table_entry = entry;

        return true;
    }

        // PŘÍPAD 3: Přiřazení hodnoty: (id = výraz)
    case NODE_ASSIGNMENT: {

        AstNode *id_node = node->child;
        AstNode *expr_node = node->child->sibling;

        // analyzuje výraz na pravé straně a zjišťuje se jeho typ.
        DataType expr_type;
        if (!analyze_expression(expr_node, stack, &expr_type)) {
            exit(10);
        }

        if (expr_type == TYPE_BOOL) {
            exit(6);
        }

        const char *name = id_node->data.identifier;
        TableEntry *entry = NULL;

        // Hledá se proměnná na levé straně. Prohledává se hierarchie zásobníků (lokální proměnné).
        entry = H_Stack_Find_Var(stack, name);

        if (entry != NULL) {
            // Nalezena lokální proměnná
            if (entry->data->kind == KIND_FUNC) {
                exit(10);
            }
            id_node->table_entry = entry;
            entry->data->data_type = expr_type;
            return true;
        }

        // Pokud nebyla nalezena lokálně, hledá se globálně

        // Setter
        char mangled_setter[256];
        sprintf(mangled_setter, "%s$setter", name);
        entry = symtable_lookup(&global_table, mangled_setter);
        if (entry != NULL) {
            id_node->table_entry = entry;
            return true;
        }

        // Globální proměnna
        if (strncmp(name, "__", 2) == 0) {
            entry = symtable_lookup(&global_table, name);
            // Pokud globální proměnná neexistuje, implicitně se vytvoří.
            if (entry == NULL) {
                SymbolData data;
                data.kind = KIND_VAR;
                data.data_type = expr_type;
                data.is_defined = true;
                data.unique_name = create_unique_name(name, 0);

                symtable_insert(&global_table, name, &data);
                entry = symtable_lookup(&global_table, name);
                entry->local_table = NULL;
            }
            else if (entry->data->kind == KIND_FUNC) {
                exit(10);
            }
            id_node->table_entry = entry;
            entry->data->data_type = expr_type;
            return true;
        }

        fprintf(stderr, "Semantic Error: Undefined variable '%s'.\n", name);
        exit(3);
    }

        // PŘÍPAD 4: Podmíněný příkaz: if (cond) {...} else {...}
    case NODE_IF: {
        AstNode *cond_node = node->child;
        AstNode *if_body = node->child->sibling;
        AstNode *else_body = node->child->sibling->sibling;

        // Analyzuje se výraz podmínky.
        DataType cond_type;
        if (!analyze_expression(cond_node, stack, &cond_type)) {
            exit(10);
        }

        // Rekurzivně se analyzují větve if 
        if (!analyze_statement(if_body, stack, block_cnt, current_scope_id)) {
            exit(10);
        }
        // a else
        if (!analyze_statement(else_body, stack, block_cnt, current_scope_id)) {
            exit(10);
        }

        return true;
    }

        // PŘÍPAD 5: Cyklus: (while)
    case NODE_WHILE: {
        AstNode *cond_node = node->child;
        AstNode *while_body = node->child->sibling;

        DataType cond_type;
        if (!analyze_expression(cond_node, stack, &cond_type)) {
            exit(10);
        }
        if (!analyze_statement(while_body, stack, block_cnt, current_scope_id)) {
            exit(10);
        }

        return true;
    }

        // PŘÍPAD 6: Návrat z funkce: (return)
    case NODE_RETURN: {
        AstNode *expr_node = node->child;

        DataType return_type;
        if (!analyze_expression(expr_node, stack, &return_type)) {
            exit(10);
        }

        return true;
    }

        // PŘÍPAD 7: Samostatné volání funkce
    case NODE_CALL_STATEMENT: {
        DataType return_type;
        if (!analyze_expression(node, stack, &return_type)) {
            exit(10);
        }

        return true;
    }
        
        // PŘÍPAD 8: Samostatný identifikátor nebo výraz
    case NODE_ID: {
        DataType expr_type;

        // Analyzuje se výraz pro kontrolu existence proměnných.
        if (!analyze_expression(node, stack, &expr_type)) {
            exit(10);
        }

        return true;
    }

    default:
        fprintf(stderr, "Internal Error (Line %d): Unexpected node type (%d) in statement list.\n", node->line_number, node->type);
        exit(99);
    }
}

/**
 * Rekurzivně se analyzuje výraz (expression) a provádí se kontrola typů.
 * Funkce vyhodnocuje typ výsledku výrazu a ověřuje sémantická pravidla pro operace
 * Zároveň propojuje uzly AST s tabulkou symbolů.
 * @param node Uzel AST reprezentující výraz.
 * @param stack Zásobník rozsahů platnosti.
 * @param result_type Ukazatel, kam se uloží výsledný datový typ výrazu.
 * @return true, pokud je výraz sémanticky správný.
 */
static bool analyze_expression(AstNode *node, ScopeStack *stack, DataType *result_type)
{
    if (node == NULL) {
        *result_type = TYPE_NIL;
        return true;
    }

    switch (node->type)
    {
        // PŘÍPADY: Literály 
    case NODE_LITERAL_NUM: {
        if (is_whole_number(node->data.literal_num)) {
            node->data_type = TYPE_NUM;
            *result_type = TYPE_NUM;
        }
        else {
            node->data_type = TYPE_FLOAT;
            *result_type = TYPE_FLOAT;
        }
        return true;
    }

    case NODE_LITERAL_STRING: {
        node->data_type = TYPE_STR;
        *result_type = TYPE_STR;
        return true;
    }

    case NODE_LITERAL_NULL: {
        node->data_type = TYPE_NIL;
        *result_type = TYPE_NIL;
        return true;
    }

        // PŘÍPAD: Identifikátor (proměnná)
    case NODE_ID: {
        const char *name = node->data.identifier;

        // Vyhledává se proměnná v hierarchii lokálních rozsahů.
        TableEntry *entry = H_Stack_Find_Var(stack, name);

        // Pokud nebyla nalezena, zkouší se vyhledat jako getter.
        if (entry == NULL) {
            char mangled_name[256];
            sprintf(mangled_name, "%s$getter", name);
            entry = symtable_lookup(&global_table, mangled_name);
        }

        // Pokud nebyla nalezena, zkouší se vyhledat jako globální proměna.
        if (entry == NULL) {
            if (strncmp(name, "__", 2) == 0) {
                entry = symtable_lookup(&global_table, name);

                if (entry == NULL) {
                    SymbolData data = { 
                        .kind = KIND_VAR, 
                        .data_type = TYPE_NIL,
                        .is_defined = true,
                        .unique_name = create_unique_name(name, 0) 
                    };

                    if (!symtable_insert(&global_table, name, &data)) exit(99);

                    entry = symtable_lookup(&global_table, name);
                    entry->local_table = NULL;
                }
            }
        }

        if (entry == NULL) {
            fprintf(stderr, "Semantic Error (Line %d): Use of undefined variable or getter '%s'.\n",
                node->line_number, name);
            exit(3);
        }

        // Kontroluje se, zda není identifikátor funkce (ne getter) použit jako proměnná
        if (entry->data->kind == KIND_FUNC) {
            char getter_name[256];
            sprintf(getter_name, "%s$getter", name);
            if (strcmp(entry->key, getter_name) != 0) {
                fprintf(stderr, "Semantic Error (Line %d): Cannot use function or setter '%s' as a variable.\n", node->line_number, name);
                exit(10);
            }
        }

        // Uzel AST se propojuje se záznamem v tabulce
        node->table_entry = entry;
        node->data_type = entry->data->data_type;
        *result_type = entry->data->data_type;
        return true;
    }

        // PŘÍPADY: Aritmetické operátory (+, -, *, /)
    case NODE_OP_PLUS:
    case NODE_OP_MINUS:
    case NODE_OP_MUL:
    case NODE_OP_DIV: {
        // Rekurzivně se vyhodnocují levý a pravý operand
        DataType l, r;
        if (!analyze_expression(node->child, stack, &l)) exit(10);
        if (!analyze_expression(node->child->sibling, stack, &r)) exit(10);

        // Pokud je typ neznámý (např. parametr funkce), kontrola se přeskakuje
        if (l == TYPE_UNKNOWN || r == TYPE_UNKNOWN) {
            *result_type = TYPE_UNKNOWN;
            node->data_type = TYPE_UNKNOWN;
            return true;
        }

        if (l == TYPE_NIL || r == TYPE_NIL) {
            fprintf(stderr, "Semantic Error (Line %d): Cannot use 'null' in arithmetic.\n", node->line_number);
            exit(6);
        }

        bool l_is_num = (l == TYPE_NUM || l == TYPE_FLOAT);
        bool r_is_num = (r == TYPE_NUM || r == TYPE_FLOAT);


        switch (node->type)
        {
        case NODE_OP_PLUS:
            if (l_is_num && r_is_num) {
                *result_type = (l == TYPE_FLOAT || r == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_NUM;
            }
            else if (l == TYPE_STR && r == TYPE_STR) *result_type = TYPE_STR;
            else {
                fprintf(stderr, "Error 6: Invalid operands for '+' (Line %d).\n", node->line_number);
                exit(6);
            }
            break;

        case NODE_OP_MINUS:
            if (l_is_num && r_is_num) {
                *result_type = (l == TYPE_FLOAT || r == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_NUM;
            }
            else {
                fprintf(stderr, "Error 6: Invalid operands for '-' (Line %d).\n", node->line_number);
                exit(6);
            }
            break;

        case NODE_OP_MUL:
            if (l_is_num && r_is_num) {
                *result_type = (l == TYPE_FLOAT || r == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_NUM;
            }
            else if (l == TYPE_STR && r == TYPE_NUM) {
                *result_type = TYPE_STR;
            }
            else if (l == TYPE_STR && r == TYPE_FLOAT) {
                fprintf(stderr, "Error 6: String iteration requires INTEGER, got float (Line %d).\n", node->line_number);
                exit(6);
            }
            else {
                fprintf(stderr, "Error 6: Invalid operands for '*' (Line %d).\n", node->line_number);
                exit(6);
            }
            break;

        case NODE_OP_DIV:
            if (l_is_num && r_is_num) {
                *result_type = TYPE_FLOAT;

                // kontrola dělení nulou
                AstNode *r_node = node->child->sibling;
                if (r_node->type == NODE_LITERAL_NUM && r_node->data.literal_num == 0.0) {
                    fprintf(stderr, "Error 57: Div by zero (Line %d).\n", node->line_number);
                    exit(6);
                }
            }
            else {
                fprintf(stderr, "Error 6: Invalid operands for '/' (Line %d).\n", node->line_number);
                exit(6);
            }
            break;

        default: break;
        }

        node->data_type = *result_type;
        return true;
    }

        // PŘÍPADY: Relační operátory (<, >, <=, >=)
    case NODE_OP_LT:
    case NODE_OP_GT:
    case NODE_OP_LTE:
    case NODE_OP_GTE: {
        DataType l, r;
        if (!analyze_expression(node->child, stack, &l)) exit(10);
        if (!analyze_expression(node->child->sibling, stack, &r)) exit(10);

        if (l == TYPE_UNKNOWN || r == TYPE_UNKNOWN) {
            *result_type = TYPE_BOOL;
            node->data_type = TYPE_BOOL;
            return true;
        }

        bool l_is_num = (l == TYPE_NUM || l == TYPE_FLOAT);
        bool r_is_num = (r == TYPE_NUM || r == TYPE_FLOAT);

        // Relační operátory vyžadují číselné operandy.
        if (!l_is_num || !r_is_num) {
            fprintf(stderr, "Error 6: Relational ops require Numbers (got %d and %d) (Line %d).\n", l, r, node->line_number);
            exit(6);
        }
        node->data_type = TYPE_BOOL;
        *result_type = TYPE_BOOL;
        return true;
    }

        // PŘÍPADY: Operátory rovnosti (==, !=)
    case NODE_OP_EQ:
    case NODE_OP_NEQ: {
        DataType l, r;
        if (!analyze_expression(node->child, stack, &l)) exit(10);
        if (!analyze_expression(node->child->sibling, stack, &r)) exit(10);

        node->data_type = TYPE_BOOL;
        *result_type = TYPE_BOOL;
        return true;
    }

        // PŘÍPAD: Operátor typu (is)
    case NODE_OP_IS: {
        DataType l;
        if (!analyze_expression(node->child, stack, &l)) exit(10);

        AstNode *type_name_node = node->child->sibling;
        if (type_name_node->type != NODE_TYPE_NAME) {
            fprintf(stderr, "Error 6: Right side of 'is' must be Type (Line %d).\n", node->line_number);
            exit(6);
        }
        const char *t_name = type_name_node->data.identifier;
        if (strcmp(t_name, "Num") != 0 && strcmp(t_name, "String") != 0 && strcmp(t_name, "Null") != 0) {
            fprintf(stderr, "Error 6: Unknown type '%s' (Line %d).\n", t_name, node->line_number);
            exit(6);
        }
        node->data_type = TYPE_BOOL;
        *result_type = TYPE_BOOL;
        return true;
    }

    // PŘÍPAD: Volání funkce
    case NODE_CALL_STATEMENT: {
        AstNode *id_node = node->child;
        const char *name = id_node->data.identifier; 
        AstNode *arg_list = id_node->sibling;

        // Počítá se počet argumentů ve volání.
        int arity = 0;
        if (arg_list && arg_list->type == NODE_ARGUMENT_LIST) {
            for (AstNode *a = arg_list->child; a; a = a->sibling) arity++;
        }

        // Vyhledává se funkce v globální tabulce
        char mangled_name[256];
        sprintf(mangled_name, "%s$%d", name, arity);
        TableEntry *func_entry = symtable_lookup(&global_table, mangled_name);

        if (!func_entry) {
            bool found_with_other_arity = false;
            size_t name_len = strlen(name);

            // hledá se funkce se stejným jménem, ale jinou aritou
            for (size_t i = 0; i < global_table.capacity; i++) {
                TableEntry *func = &global_table.entries[i];
                if (func->status == SLOT_OCCUPIED && func->data->kind == KIND_FUNC) {
                    if (strncmp(func->key, name, name_len) == 0 && func->key[name_len] == '$') {
                        found_with_other_arity = true;
                        break;
                    }
                }
            }

            if (found_with_other_arity) {
                // Chyba arity
                fprintf(stderr, "Semantic Error (Line %d): Function '%s' called with wrong number of arguments (expected another, got %d).\n", node->line_number, name, arity);
                exit(5); 
            } else {
                // Nedefinovaná funkce
                fprintf(stderr, "Semantic Error (Line %d): Undefined function '%s'.\n", node->line_number, name);
                exit(3); 
            }
        }

        id_node->table_entry = func_entry;

        // Analyzují se typy argumentů.
        DataType arg_types[10];
        int idx = 0;

        if (arg_list) {
            for (AstNode *arg = arg_list->child; arg; arg = arg->sibling) {

                if (arg->type == NODE_LITERAL_NUM) arg_types[idx] = TYPE_NUM;
                else if (arg->type == NODE_LITERAL_STRING) arg_types[idx] = TYPE_STR;
                else if (arg->type == NODE_LITERAL_NULL) arg_types[idx] = TYPE_NIL;
                else if (arg->type == NODE_ID) {
                    DataType id_type;
                    // Rekurzivní analýza proměnné použité jako argument.
                    if (!analyze_expression(arg, stack, &id_type)) exit(10);
                    arg_types[idx] = id_type;
                }
                else {
                    fprintf(stderr, "Error 6: Expression in argument not allowed (Line %d).\n", arg->line_number);
                    exit(6);
                }
                idx++;
            }
        }

        // Statická kontrola typů argumentů pro vestavěné funkce
        if (strncmp(name, "Ifj.", 4) == 0) {
            bool ok = true;

            if (strcmp(name, "Ifj.floor") == 0) {
                if (arg_types[0] != TYPE_NUM && arg_types[0] != TYPE_FLOAT && arg_types[0] != TYPE_UNKNOWN) ok = false;
            }
            else if (strcmp(name, "Ifj.length") == 0) {
                if (arg_types[0] != TYPE_STR && arg_types[0] != TYPE_UNKNOWN) ok = false;
            }
            else if (strcmp(name, "Ifj.substring") == 0) {
                bool arg1_bad = (arg_types[0] != TYPE_STR && arg_types[0] != TYPE_UNKNOWN);
                bool arg2_bad = (arg_types[1] != TYPE_NUM && arg_types[1] != TYPE_UNKNOWN);
                bool arg3_bad = (arg_types[2] != TYPE_NUM && arg_types[2] != TYPE_UNKNOWN);

                if (arg1_bad || arg2_bad || arg3_bad) ok = false;
            }
            else if (strcmp(name, "Ifj.strcmp") == 0) {
                bool arg1_bad = (arg_types[0] != TYPE_STR && arg_types[0] != TYPE_UNKNOWN);
                bool arg2_bad = (arg_types[1] != TYPE_STR && arg_types[1] != TYPE_UNKNOWN);

                if (arg1_bad || arg2_bad) ok = false;
            }
            else if (strcmp(name, "Ifj.ord") == 0) {
                bool arg1_bad = (arg_types[0] != TYPE_STR && arg_types[0] != TYPE_UNKNOWN);
                bool arg2_bad = (arg_types[1] != TYPE_NUM && arg_types[1] != TYPE_UNKNOWN);

                if (arg1_bad || arg2_bad) ok = false;
            }
            else if (strcmp(name, "Ifj.chr") == 0) {
                if (arg_types[0] != TYPE_NUM && arg_types[0] != TYPE_UNKNOWN) ok = false;
            }

            if (!ok) {
                fprintf(stderr, "Error 5: Invalid arg type for '%s' (Line %d).\n", name, node->line_number);
                exit(5);
            }
        }

        node->data_type = func_entry->data->data_type;
        *result_type = func_entry->data->data_type;
        return true;
    }

    default:
        fprintf(stderr, "Internal Error: Invalid expression node %d\n", node->type);
        exit(99);
    }
}