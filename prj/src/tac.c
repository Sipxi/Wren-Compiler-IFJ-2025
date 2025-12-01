/**
 * @file tac.c
 *
 * @brief Implementace Tříadresného kódu (Three-Address Code - TAC) 
 *
 * Author:
 *     - Serhij Čepil (253038)
 *
 */

#include "tac.h"
#include "utils.h"
#include "error_codes.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 /* ======================================*/
 /* ===== Globální proměnné =====*/
 /* ======================================*/

int global_temp_counter = 0;
int global_label_counter = 0;

/* ======================================*/
/* ===== Prototypy privátních funkcí =====*/
/* ======================================*/

/**
 * @brief Kontroluje, zda řetězec obsahuje název getteru.
 * 
 * @param name Název funkce pro kontrolu.
 * @return true pokud název odpovídá getteru, false jinak.
 */
static bool has_getter(const char *name);

/**
 * @brief Hlavní rekurzivní funkce. Prochází AST a generuje TAC.
 *
 *
 * @param node Uzel AST, pro který se generuje TAC.
 * @param tac_list Seznam TAC instrukcí pro přidání nových instrukcí.
 * @param symtable Tabulka symbolů (většinou pro 'read-only' přístup).
 *
 * @return Vrací 'Operand*', ve kterém je uložen výsledek výrazu ($t1 nebo
konstanta 5).
 * @note Pro stavy (if, var def, block) vrací NULL, protože nevytvářejí "hodnoty", ale pouze generují instrukce do 'tac_list'.
 *
 * @example
 * .// Výchozí kód:
 * static main() {
 *   if (a < 10) {
 *     b = a + 1
 *   }
 * }
 *
 * // Sgenereovaný 3AC:
 * // (OPCODE, RESULT, ARG1, ARG2)
 * LABEL,          ____,      main,   ____
 * FUNC_BEGIN,      ____,      main,   ____
 * // 1. tac_gen_recursive(NODE_OP_LT) vrací '$t0'
 * LESS,           $t0,         a,     10
 * // 2. tac_gen_recursive(NODE_IF) generuje JUMP a štítky
 * JUMP_IF_FALSE,   ____,       $t0,    L_ENDIF_0
 * // 3. tac_gen_recursive(NODE_OP_PLUS) vrací '$t1'
 * ADD,             $t1,         a,     1
 * // 4. tac_gen_recursive(NODE_ASSIGNMENT) používá '$t1'
 * ASSIGN,           b,         $t1,   ____
 * // 5. Konec NODE_IF
 * LABEL,          ____,      L_ENDIF_0, ____
 * FUNC_END,        ____,      main,   ____
 *  */
static Operand *tac_gen_recursive(AstNode *node, TACDLList *tac_list,
    Symtable *symtable);

/**
 * @brief Vytváří operandu pro symbol (proměnnou) z tabulky symbolů.
 *
 * Toto je tovární funkce, která vytváří a alokuje paměť pro operand
 *
 * @note Nastavuje typ OPERAND_TYPE_SYMBOL
 * @note Nastavuje data.symbol_entry na poskytnutý záznam tabulky symbolů.
 * @param entry Záznam tabulky symbolů.
 * @return Ukazatel na vytvořený operand.
 */
static Operand *create_symbol_operand(TableEntry *entry);

/**
 * @brief Vytváří prázdný operand daného typu.
 *
 * @param type Typ vytvářeného operandu.
 * @return Ukazatel na vytvořený operand.
 * @note Pomocná funkce.
 */
static Operand *create_operand(OperandType type);

/**
 * @brief Vytváří nový dočasný operand. Například $t1, $t2 atd.
 *
 * @param tac_list Seznam TAC instrukcí, do kterého bude přidána instrukce
 *
 * @note Globální počítadlo dočasných proměnných se při každém volání zvyšuje.
 * @note Vytváří instrukci DECLARE pro novou dočasnou proměnnou.
 *
 * @return Ukazatel na vytvořený dočasný operand.
 */
static Operand *create_temp_operand(TACDLList *tac_list);

/**
 * Vytváří operand pro návěstí se zadaným jménem.
 *
 * @param label_name Jméno návěstí (například "L_FUNCTION").
 * @return Ukazatel na vytvořený operand.
 */
static Operand *create_label_operand(const char *label_name);

/**
 * Rekurzivně generuje TAC pro všechny děti uzlu.
 *
 * @param node Uzel AST, jehož děti je třeba zpracovat.
 * @param tac_list Seznam TAC instrukcí pro přidání nových instrukcí.
 * @param symtable Tabulka symbolů.
 * @return void
 */
static void tac_gen_children_list(AstNode *node, TACDLList *tac_list,
    Symtable *symtable);

/**
 * Vytvoření instrukce a přidání na konec seznamu.
 *
 * @param list Seznam instrukcí.
 * @param op Kód operace.
 * @param res Výsledkový operand.
 * @param arg1 První argument.
 * @param arg2 Druhý argument.
 * @return void
 */
static void generate_instruction(TACDLList *list, TacOperationCode op,
    Operand *res, Operand *arg1, Operand *arg2);


    
/**
 * @brief Vypíše "pěknou" chybu 3AC generátoru a ukončí program.
 * @param message Popis chyby (např. "Unimplemented NodeType")
 * @param error_code Kód chyby (pro exit())
 */
void raise_tac_error(const char *message, ErrorCode error_code);


/* ======================================*/
/* ===== Implementace soukromých funkcí TACDLList =====*/
/* ======================================*/

void TACDLL_Error(void) {
    fprintf(stderr, "TACDLList: Chyba alokace paměti\n");
    exit(INTERNAL_ERROR);
}

/* ======================================*/
/* ===== Implementace veřejných funkcí TACDLList =====*/
/* ======================================*/

void TACDLL_Init(TACDLList *list) {
    list->first_element = NULL;
    list->active_element = NULL;
    list->last_element = NULL;
}

void TACDLL_Dispose(TACDLList *list) {

    TACDLLElementPtr tmp = list->first_element;
    // Pomocný příznak pro procházení seznamu
    TACDLLElementPtr next;
    // Uvolňuje paměť všech prvků
    while (tmp != NULL) {
        free(tmp->tac_instruction);
        next = tmp->next_element;
        free(tmp);
        tmp = next;
    }
    // Znovu inicializuje seznam
    TACDLL_Init(list);
}

void TACDLL_InsertFirst(TACDLList *list, TacInstruction *tac_instruction) {
    TACDLLElementPtr newElement = (TACDLLElementPtr)malloc(sizeof(struct TACDLLElement));
    if (newElement == NULL) {
        TACDLL_Error();
    }

    // Začneme vyplňovat nový prvek
    newElement->tac_instruction = tac_instruction;
    // Nový prvek bude první, nemá předchůdce
    newElement->prev_element = NULL;
    // Nový prvek bude ukazovat na bývalý první prvek
    newElement->next_element = list->first_element;
    // Pokud seznam nebyl prázdný, nastavujeme předchůdce bývalého prvního prvku na nový prvek
    if (list->first_element != NULL)
        list->first_element->prev_element = newElement;
    // Pokud byl seznam prázdný, nastavujeme také ukazatel na poslední prvek
    else
        list->last_element = newElement;
    list->first_element = newElement;
}

void TACDLL_InsertLast(TACDLList *list, TacInstruction *tac_intruction) {
    TACDLLElementPtr newElement = (TACDLLElementPtr)malloc(sizeof(struct TACDLLElement));
    if (newElement == NULL) {
        TACDLL_Error();
    }
    newElement->tac_instruction = tac_intruction;
    newElement->next_element = NULL;
    newElement->prev_element = list->last_element;
    // Pokud seznam nebyl prázdný, nastavujeme ukazatel na následující prvek posledního prvku na nový prvek
    if (list->last_element != NULL)
        list->last_element->next_element = newElement;
    // Pokud byl seznam prázdný, nastavujeme také ukazatel na první prvek
    else
        list->first_element = newElement;
    list->last_element = newElement;
}

void TACDLL_First(TACDLList *list) {
    list->active_element = list->first_element;
}

void TACDLL_Last(TACDLList *list) {
    list->active_element = list->last_element;
}

void TACDLL_GetFirst(TACDLList *list, TacInstruction **tac_instruction_ptr) {
    if (list->first_element != NULL)
        *tac_instruction_ptr = list->first_element->tac_instruction;
    else
        TACDLL_Error();
}

void TACDLL_GetLast(TACDLList *list, TacInstruction **tac_instruction_ptr) {
    if (list->last_element != NULL)
        *tac_instruction_ptr = list->last_element->tac_instruction;
    else
        TACDLL_Error();
}

void TACDLL_DeleteFirst(TACDLList *list) {
    if (list->first_element == NULL) return; // Seznam je prázdný, nic neděláme
    // Pokud byl první prvek aktivní, deaktivujeme seznam
    if (list->active_element == list->first_element)
        list->active_element = NULL;

    // Pomocná proměnná pro uvolnění prvku
    TACDLLElementPtr tmp = list->first_element;
    // Posouváme ukazatel na první prvek na následující prvek
    list->first_element = tmp->next_element;
    // Pokud nový první prvek existuje, nastavujeme jeho předchozí prvek na NULL
    if (list->first_element != NULL)
        list->first_element->prev_element = NULL;
    // Pokud seznam sestával z jednoho prvku, také nastavujeme ukazatel na poslední prvek na NULL
    else
        list->last_element = NULL;
    free(tmp->tac_instruction);
    // Uvolňujeme paměť původního prvního prvku
    free(tmp);
}

void TACDLL_DeleteLast(TACDLList *list) {
    // Seznam je prázdný, nic neděláme
    if (list->last_element == NULL) return;
    // Pokud byl poslední prvek aktivní, deaktivujeme seznam
    if (list->active_element == list->last_element)
        list->active_element = NULL;

    // Pomocná proměnná pro uvolnění prvku
    TACDLLElementPtr tmp = list->last_element;
    list->last_element = tmp->prev_element;
    // Pokud nový poslední prvek existuje, nastavujeme jeho následující prvek na NULL
    if (list->last_element != NULL)
        list->last_element->next_element = NULL;
    // Pokud seznam sestával z jednoho prvku, také nastavujeme ukazatel na první prvek na NULL
    else
        list->first_element = NULL;
    free(tmp->tac_instruction);
    free(tmp);
}

void TACDLL_DeleteAfter(TACDLList *list) {
    // Pomocná proměnná pro zkrácení zápisu
    TACDLLElementPtr active_element = list->active_element;

    // Pokud seznam není aktivní nebo aktivní prvek je poslední, nic neděláme
    if (active_element == NULL || active_element->next_element == NULL)
        return;

    // Prvek za aktivním prvkem
    TACDLLElementPtr afterActiveElememt = active_element->next_element;
    // Přeskakujeme prvek za aktivním
    active_element->next_element = afterActiveElememt->next_element;
    // Pokud prvek za aktivním není poslední, nastavujeme jeho předchozí prvek na aktivní prvek
    if (active_element->next_element != NULL)
        active_element->next_element->prev_element = active_element;
    // Pokud byl prvek za aktivním poslední, aktualizujeme ukazatel na poslední prvek
    else
        list->last_element = active_element;
    free(afterActiveElememt->tac_instruction);
    free(afterActiveElememt);
}

void TACDLL_DeleteBefore(TACDLList *list) {
    // Pomocná proměnná pro zkrácení zápisu
    TACDLLElementPtr active_element = list->active_element;

    if (active_element == NULL || active_element->prev_element == NULL)
        return;

    // Prvek před aktivním prvkem
    TACDLLElementPtr beforeActiveElememt = active_element->prev_element;
    // Přeskakujeme prvek před aktivním
    active_element->prev_element = beforeActiveElememt->prev_element;
    // Pokud prvek před aktivním není první, nastavujeme jeho následující prvek na aktivní prvek
    if (active_element->prev_element != NULL)
        active_element->prev_element->next_element = active_element;
    // Pokud byl prvek před aktivním první, aktualizujeme ukazatel na první prvek
    else
        list->first_element = active_element;
    free(beforeActiveElememt->tac_instruction);
    free(beforeActiveElememt);
}

void TACDLL_InsertAfter(TACDLList *list, TacInstruction *tac_instruction) {
    // Pomocná proměnná pro zkrácení zápisu
    TACDLLElementPtr active_element = list->active_element;

    if (active_element == NULL) return;
    // Ukazatel na nový prvek
    TACDLLElementPtr newElement = (TACDLLElementPtr)malloc(sizeof(struct TACDLLElement));
    if (newElement == NULL) {
        TACDLL_Error();
    }
    // Hodnota nového prvku
    newElement->tac_instruction = tac_instruction;
    // Nový prvek bude ukazovat na aktivní prvek jako na předchůdce
    newElement->prev_element = active_element;
    // Nový prvek bude ukazovat na následující prvek aktivního prvku
    newElement->next_element = active_element->next_element;
    // Pokud je aktivní prvek poslední, aktualizujeme ukazatel na poslední prvek
    if (active_element->next_element == NULL)
        list->last_element = newElement;
    // Pokud aktivní prvek není poslední, nastavujeme předchůdce následujícího prvku na nový prvek
    else
        active_element->next_element->prev_element = newElement;
    active_element->next_element = newElement; // Aktivní prvek bude ukazovat na nový prvek jako na následující
}

void TACDLL_InsertBefore(TACDLList *list, TacInstruction *tac_instruction) {
    TACDLLElementPtr active_element = list->active_element;

    if (active_element == NULL) return;

    TACDLLElementPtr newElement = (TACDLLElementPtr)malloc(sizeof(struct TACDLLElement));
    if (newElement == NULL) {
        TACDLL_Error();
    }

    newElement->tac_instruction = tac_instruction;
    newElement->next_element = active_element;
    newElement->prev_element = active_element->prev_element;
    if (active_element->prev_element == NULL)
        list->first_element = newElement;
    else
        active_element->prev_element->next_element = newElement;
    active_element->prev_element = newElement;
}

void TACDLL_GetValue(TACDLList *list, TacInstruction **tac_instruction_ptr) {
    if (list->active_element == NULL) {
        TACDLL_Error();
    }
    *tac_instruction_ptr = list->active_element->tac_instruction;
}

void TACDLL_SetValue(TACDLList *list, TacInstruction *tac_instruction) {
    if (list->active_element == NULL) return;
    list->active_element->tac_instruction = tac_instruction;
}

void TACDLL_Next(TACDLList *list) {
    if (list->active_element == NULL) return;
    list->active_element = list->active_element->next_element;
}

void TACDLL_Previous(TACDLList *list) {
    if (list->active_element == NULL) return;
    list->active_element = list->active_element->prev_element;
}

bool TACDLL_IsActive(TACDLList *list) {
    return (list->active_element != NULL);
}

/* ======================================*/
/* ===== Implementace soukromých funkcí =====*/
/* ======================================*/

static bool has_getter(const char *name) {
   return strstr(name, "getter") != NULL;
}

static void tac_gen_children_list(AstNode *node, TACDLList *tac_list,
    Symtable *symtable) {
    // Procházíme všechny děti uzlu
    AstNode *current = node->child;
    while (current != NULL) {
        tac_gen_recursive(current, tac_list, symtable);
        // Přecházíme k dalšímu sourozenci
        current = current->sibling;
    }
}

static void generate_instruction(TACDLList *list, TacOperationCode op,
    Operand *res, Operand *arg1, Operand *arg2) {
    // Alokujeme paměť pro instrukci
    TacInstruction *instruction =
        (TacInstruction *)malloc(sizeof(TacInstruction));
    // Vyplňujeme pole instrukce
    instruction->operation_code = op;
    instruction->result = res;
    instruction->arg1 = arg1;
    instruction->arg2 = arg2;
    // Vkládáme instrukci na konec seznamu
    TACDLL_InsertLast(list, instruction);
}

static Operand *create_operand(OperandType type) {
    // Alokujeme paměť pro operand
    Operand *op = (Operand *)malloc(sizeof(Operand));
    if (!op) {
        raise_tac_error("Memory allocation failed for operand",
            INTERNAL_ERROR);
    }
    op->type = type;
    return op;
}

static Operand *create_label_operand(const char *label_name) {
    // Vytváříme operand typu LABEL
    Operand *op = create_operand(OPERAND_TYPE_LABEL);
    // Kopírujeme jméno návěstí
    op->data.label_name = malloc(strlen(label_name) + 1);
    if (!op->data.label_name) {
        raise_tac_error("Memory allocation failed for label operand",
            INTERNAL_ERROR);
        return NULL;
    }
    // Kopírujeme řetězec
    strcpy(op->data.label_name, label_name);
    return op;
}

static Operand *create_temp_operand(TACDLList *tac_list) {
    Operand *op = create_operand(OPERAND_TYPE_TEMP);
    // Přiřazujeme unikátní ID
    op->data.temp_id = global_temp_counter++;
    generate_instruction(tac_list, OP_DECLARE, op, NULL, NULL);
    return op;
}

static Operand *create_symbol_operand(TableEntry *entry) {
    Operand *op = create_operand(OPERAND_TYPE_SYMBOL);
    op->data.symbol_entry = entry;
    return op;
}

static Operand *tac_gen_recursive(AstNode *node, TACDLList *tac_list,
    Symtable *symtable) {
    // Základní případ rekurze
    // Pokud je uzel prázdný, vracíme NULL
    if (node == NULL) {
        return NULL;
    }
    /* Zpracování uzlu AST a generování odpovídající instrukce TAC */

    switch (node->type) {

    /* ===== Zpracování strukturních uzlů ===== */
    // Tyto uzly procházejí všechny děti a generují instrukce pro
        // každý z nich

    case NODE_PROGRAM:
    case NODE_BLOCK:
    case NODE_PARAM_LIST: {

        // Ukládáme současnou hodnotu globálního počítadla dočasných
        // ! později předělat
        // int temp = global_temp_counter;
        // global_temp_counter = 0;

        // Zpracování kořenového uzlu bloku
        // Jednoduše zpracováváme všechny děti
        tac_gen_children_list(node, tac_list, symtable);
        // Obnovujeme globální počítadlo dočasných
        // ! později předělat
        // global_temp_counter = temp;

        // Nevrací hodnoty
        return NULL;
    }

    /* ===== Zpracování uzlů funkcí ===== */

    case NODE_FUNCTION_DEF: {
        // Parametry funkce NODE_PARAM_LIST
        AstNode *params = node->child;
        // Tělo funkce NODE_BLOCK
        AstNode *body = node->child->sibling;

        // Získáváme jméno funkce ze záznamu tabulky symbolů
        TableEntry *func_entry = node->table_entry;
        // Generujeme návěstí funkce
        // Jméno funkce jako návěstí
        char *func_label = func_entry->key;

        Operand *label_op = create_label_operand(func_label);
        generate_instruction(tac_list, OP_LABEL, NULL, label_op, NULL);

        // Generujeme instrukci začátku funkce, FRAME atd.
        Operand *func_entry_op = create_symbol_operand(func_entry);
        generate_instruction(tac_list, OP_FUNCTION_BEGIN, NULL,
            func_entry_op, NULL);

        // Generujeme kód pro parametry funkce (NODE_PARAM_LIST)
        tac_gen_recursive(params, tac_list, symtable);

        // Generujeme kód pro tělo funkce (NODE_BLOCK)
        tac_gen_recursive(body, tac_list, symtable);

        // Vytváříme nový operand pro konec funkce
        Operand *func_entry_op_end = create_label_operand(func_entry->key);
        // Generujeme instrukci konce funkce
        generate_instruction(tac_list, OP_FUNCTION_END, NULL,
            func_entry_op_end, NULL);
        // Definice funkce nevrací hodnoty
        return NULL;
    }

    /* ===== Zpracování uzlů parametrů funkcí ===== */

    case NODE_PARAM: {
        // Parametr funkce
        // Získáváme záznam tabulky symbolů parametru
        TableEntry *param_entry = node->table_entry;
        // Generujeme instrukci předání parametru
        Operand *param_op = create_symbol_operand(param_entry);
        generate_instruction(tac_list, OP_PARAM, param_op, NULL, NULL);

        // Parametr nevrací hodnoty
        return NULL;
    }

    /* ===== Zpracování uzlů setterů ===== */

    case NODE_SETTER_DEF: {
        // Zpracování uzlu definice setteru

        // NODE_PARAM
        AstNode *setter_param = node->child;
        // NODE_BLOCK
        AstNode *setter_body = node->child->sibling;

        // Získáváme jméno setteru ze záznamu tabulky symbolů
        TableEntry *setter_entry = node->table_entry;
        // Generujeme návěští začátku setteru
        char *setter_label = setter_entry->key;  // Jméno setteru jako návěští

        // Návěští setteru 
        Operand *setter_label_op = create_label_operand(setter_label);
        generate_instruction(tac_list, OP_LABEL, NULL, setter_label_op,
            NULL);

        // Generujeme instrukci začátku setteru
        Operand *setter_entry_op = create_symbol_operand(setter_entry);
        generate_instruction(tac_list, OP_FUNCTION_BEGIN, NULL,
            setter_entry_op, NULL);

        // Generujeme kód pro parametr setteru (NODE_PARAM)
        tac_gen_recursive(setter_param, tac_list, symtable);

        // Generujeme kód pro tělo setteru (NODE_BLOCK)
        tac_gen_recursive(setter_body, tac_list, symtable);

        Operand *setter_entry_op_end = create_symbol_operand(setter_entry);
        // Generujeme instrukci konce setteru
        generate_instruction(tac_list, OP_FUNCTION_END, NULL,
            setter_entry_op_end, NULL);
        return NULL;  // Definice setteru nevrací hodnoty
    }

    /* ===== Zpracování uzlů getterů ===== */

    case NODE_GETTER_DEF: {
        // Zpracování uzlu definice getteru
        // NODE_BLOCK
        AstNode *getter_body = node->child;
        // Získáváme jméno getteru ze záznamu tabulky symbolů
        TableEntry *getter_entry = node->table_entry;

        // Generujeme návěští začátku getteru
        char *getter_label = getter_entry->key;  // Jméno getteru jako návěští
        Operand *getter_label_op = create_label_operand(getter_label);
        generate_instruction(tac_list, OP_LABEL, NULL, getter_label_op,
            NULL);

        // Generujeme instrukci začátku getteru
        Operand *getter_entry_op = create_symbol_operand(getter_entry);
        generate_instruction(tac_list, OP_FUNCTION_BEGIN, NULL,
            getter_entry_op, NULL);
        // Generujeme kód pro tělo getteru (NODE_BLOCK)
        tac_gen_recursive(getter_body, tac_list, symtable);

        Operand *getter_entry_op_end = create_symbol_operand(getter_entry);
        // Generujeme instrukci konce getteru
        generate_instruction(tac_list, OP_FUNCTION_END, NULL,
            getter_entry_op_end, NULL);
        return NULL;  // Definice getteru nevrací hodnoty
    }

    /* ===== Zpracování uzlů podmíněných příkazů ===== */

    case NODE_IF: {
        // Zpracování uzlu if
        // child -> Podmínka (<expression>)
        AstNode *condition = node->child;
        // child->sibling -> Blok if (NODE_BLOCK)
        AstNode *if_block = node->child->sibling;
        // child->sibling->sibling -> Blok else (NODE_BLOCK) (může být
        // NULL)
        AstNode *else_block = node->child->sibling->sibling;

        // Vytváříme unikátní návěští pro větve if a else
        char *else_label = create_unique_label("L_ELSE");
        char *end_if_label = create_unique_label("L_ENDIF");

        // Generujeme kód pro podmínku
        // Vrací výsledek v podobě operandu (např. $t1), kde
        // je uloženo true/false
        Operand *condition_result =
            tac_gen_recursive(condition, tac_list, symtable);

        // Generujeme instrukci přechodu, pokud je podmínka nepravdivá
        // Pokud je podmínka nepravdivá, skáčeme k návěští else_label, ale jen když else_block
        // existuje
        char *jump_label = (else_block == NULL) ? end_if_label : else_label;

        // Vytváříme operand pro návěští skoku v případě lži
        Operand *jump_if_false = create_label_operand(jump_label);
        generate_instruction(tac_list, OP_JUMP_IF_FALSE, NULL, condition_result,
            jump_if_false);

        // Generujeme kód pro blok if
        tac_gen_recursive(if_block, tac_list, symtable);

        // Zpracování bloku else, pokud existuje
        if (else_block != NULL) {
            // Generujeme JUMP na konec, abychom přeskočili 'else' pokud if
            // byl proveden
            Operand *jump_if_done = create_label_operand(end_if_label);
            generate_instruction(tac_list, OP_JUMP, NULL, NULL,
                jump_if_done);

            // Generujeme návěští, že začal else blok:
            Operand *else_label_op = create_label_operand(else_label);
            generate_instruction(tac_list, OP_LABEL, NULL, else_label_op,
                NULL);

            // Generujeme kód pro blok else
            tac_gen_recursive(else_block, tac_list, symtable);
        }
        // Pokud else nebylo, pak jednoduše generujeme návěští end_if_label:
        Operand *end_if_label_op = create_label_operand(end_if_label);
        generate_instruction(tac_list, OP_LABEL, NULL, end_if_label_op,
            NULL);

        // Čistíme dočasná návěští
        free(else_label);
        free(end_if_label);

        return NULL;  // Uzel if nevrací hodnoty
    }

    /*===== Zpracování uzlu while =====*/

    case NODE_WHILE: {
        // Zpracování uzlu while
        // child -> Podmínka (<expression>)
        AstNode *condition = node->child;
        // child->sibling -> Tělo cyklu (NODE_BLOCK)
        AstNode *while_body = node->child->sibling;

        // Vytváříme unikátní návěští pro začátek podmínky a konec cyklu
        char *condition_label = create_unique_label("L_WHILE_CONDITION");
        char *endwhile_label = create_unique_label("L_ENDWHILE");
        // Generujeme návěští začátku podmínky
        Operand *condition_label_op = create_label_operand(condition_label);
        generate_instruction(tac_list, OP_LABEL, NULL, condition_label_op,
            NULL);

        // Generujeme kód pro podmínku
        Operand *condition_result =
            tac_gen_recursive(condition, tac_list, symtable);

        // Generujeme instrukci přechodu, pokud je podmínka nepravdivá

        Operand *jump_if_false = create_label_operand(endwhile_label);
        generate_instruction(tac_list, OP_JUMP_IF_FALSE, NULL, condition_result,
            jump_if_false);
        // Generujeme kód pro tělo cyklu
        tac_gen_recursive(while_body, tac_list, symtable);
        // Generujeme skok zpět na začátek podmínky
        Operand *jump_at_beginning = create_label_operand(condition_label);
        generate_instruction(tac_list, OP_JUMP, NULL, NULL, jump_at_beginning);
        // Generujeme návěští konce cyklu
        Operand *end_while_op = create_label_operand(endwhile_label);
        generate_instruction(tac_list, OP_LABEL, NULL, end_while_op, NULL);
        // Čistíme dočasná návěští
        free(condition_label);
        free(endwhile_label);

        return NULL;  // Uzel while nevrací hodnoty
    }

    /* ===== Zpracování uzlu definice proměnné ===== */

    case NODE_VAR_DEF: {
        // NODE_VAR_DEF (pro var id) ->  id = nil.
        // NODE_ASSIGNMENT (pro id = 10) ->  id = 10 (a to přepíše
        // nil).

        //  Získáváme 'id' proměnné z symtable
        TableEntry *var_entry = node->table_entry;

        // Vytváříme operand-symbol pro 'id'
        // Toto bude 'result' (LHS - levá strana)
        Operand *lhs_op = create_symbol_operand(var_entry);

        generate_instruction(tac_list, OP_DECLARE, lhs_op, NULL, NULL);

        return NULL;  // 'var' - to je statement, nevrací hodnoty
    }

    /* ===== Zpracování uzlu return ===== */

    case NODE_RETURN: {
        // Operátor return
        AstNode *return_expr = node->child;  // Výraz pro návrat

        Operand *ret_op;
        if (return_expr != NULL) {
            // Generujeme kód pro výraz návratu
            ret_op = tac_gen_recursive(return_expr, tac_list, symtable);
        }
        else {
            // Prázdný operand pro 'return;' bez výrazu
            ret_op = NULL;
        }

        // Generujeme instrukci návratu   
        // Bude to buď výsledek výrazu, nebo nil.
        generate_instruction(tac_list, OP_RETURN, ret_op, NULL, NULL);
        return NULL;  // Operátor return nevrací hodnoty
    }

    /* ===== Zpracování uzlů výrazů ===== */

    case NODE_ASSIGNMENT: {
        // 'id = expression'
        // child -> Výraz (LHS)
        AstNode *lhs_node = node->child;
        // child->sibling -> Výraz (RHS)
        AstNode *expr_node = node->child->sibling;

        // Získáváme 'id' proměnné z symtable
        TableEntry *var_entry = lhs_node->table_entry;

        // Generujeme kód pro výraz (RHS)
        Operand *rhs_op = tac_gen_recursive(expr_node, tac_list, symtable);

        // Vytváříme operand-symbol pro 'id' (LHS)
        Operand *lhs_op = create_symbol_operand(var_entry);
        // Generujeme instrukci přiřazení
        generate_instruction(tac_list, OP_ASSIGN, lhs_op, rhs_op, NULL);
        return NULL;  // Operátor přiřazení nevrací hodnoty
    }

    /* ===== Zpracování uzlu volání funkce ===== */

    case NODE_CALL_STATEMENT: {
        // Získáváme děti 
        // Jméno funkce
        AstNode *func_name_node = node->child;
        // Seznam argumentů (NODE_ARGUMENT_LIST)
        AstNode *arg_list_node =
            node->child->sibling;

        TableEntry *func_entry = func_name_node->table_entry;

        // Generujeme instrukci volání funkce
        Operand *func_op = create_label_operand(func_entry->key);
        generate_instruction(tac_list, OP_CALL, NULL, func_op, NULL);

        if (arg_list_node != NULL) {
            // Generujeme kód pro seznam argumentů
            tac_gen_recursive(arg_list_node, tac_list, symtable);
        }

        return func_op;  // Vracíme operand funkce (může být potřebný)
    }

    /* ===== Zpracování uzlu seznamu argumentů ===== */

    case NODE_ARGUMENT_LIST: {
        // Bereme PRVNÍ argument (první dítě NODE_ARGUMENT_LIST)
        AstNode *current_arg_expr = node->child;

        // Jdeme po seznamu sourozeneců (sibling)
        while (current_arg_expr != NULL) {
            // Generujeme kód pro výraz ('a + b' -> $t0)
            Operand *arg_op =
                tac_gen_recursive(current_arg_expr, tac_list, symtable);

            // Generujeme OP_PARAM, abychom napušili výsledek
            // (Předpokládáme, že parametr jde do arg1)
            generate_instruction(tac_list, OP_PARAM, NULL, arg_op, NULL);

            // Přecházíme k dalšímu argumentu
            current_arg_expr = current_arg_expr->sibling;
        }

        return NULL;  // Seznam argumentů nevrací hodnoty
    }

    /* ===== Zpracování uzlů binárních operací ===== */
    // Tyto uzly vrací výsledek výrazu v podobě dočasného operandu
    // Například pro 'a + b' se vrátí '$t1', ve kterém bude uložen
    // výsledek sčítání.
    // Pro plus to může být buď ADD (čísla), nebo CONCAT (řetězce)

    case NODE_OP_PLUS: {
        // Získáváme děti
        AstNode *left_node = node->child;
        AstNode *right_node = node->child->sibling;

        // Rekurzivně generujeme kód
        Operand *left_op = tac_gen_recursive(left_node, tac_list, symtable);
        Operand *right_op =
            tac_gen_recursive(right_node, tac_list, symtable);

        // Vytváříme dočasný výsledek
        Operand *result_op = create_temp_operand(tac_list);

        // Koukneme na typ, který nám dala sémantika (Pass 2)
        TacOperationCode op_code;
        if (node->data_type == TYPE_STR) {
            op_code = OP_CONCAT;
        }
        else {
            // (Pokud je to TYPE_NUM nebo něco jiného, považujeme to za ADD)
            op_code = OP_ADD;
        }



        // Generujeme instrukci (to samé)
        generate_instruction(tac_list, op_code, result_op, left_op,
            right_op);
        return result_op;
    }

    case NODE_OP_MUL: {
        // Získáváme děti
        AstNode *left_node = node->child;
        AstNode *right_node = node->child->sibling;

        // Rekurzivně generujeme kód
        Operand *left_op = tac_gen_recursive(left_node, tac_list, symtable);
        Operand *right_op =
            tac_gen_recursive(right_node, tac_list, symtable);
        // Vytváříme dočasný operand pro výsledek
        Operand *result_op = create_temp_operand(tac_list);

        // Určujeme kód operace
        TacOperationCode op_code;
        if (node->data_type == TYPE_STR) {
            op_code = OP_MULTIPLY_STRING;

        }
        else {
            op_code = OP_MULTIPLY;
        }

        // Generujeme instrukci TAC
        generate_instruction(tac_list, op_code, result_op, left_op,
            right_op);

        // Vracíme výsledek ($tN)
        return result_op;
    }

    /* ===== Zpracování uzlů operací ===== */
    // Tyto uzly vrací výsledek výrazu v podobě dočasného operandu
    // Jelikož jsou všechny podobné, zpracováváme je v jednom case
    case NODE_OP_MINUS:
    case NODE_OP_DIV:
    case NODE_OP_LT:
    case NODE_OP_GT:
    case NODE_OP_LTE:
    case NODE_OP_GTE:
    case NODE_OP_EQ:
    case NODE_OP_NEQ: {
        // Získáváme děti
        AstNode *left_node = node->child;
        AstNode *right_node = node->child->sibling;

        // Rekurzivně generujeme kód
        Operand *left_op = tac_gen_recursive(left_node, tac_list, symtable);
        Operand *right_op =
            tac_gen_recursive(right_node, tac_list, symtable);

        // Vytváříme dočasný operand pro výsledek
        Operand *result_op = create_temp_operand(tac_list);

        // Určujeme kód operace
        TacOperationCode op_code;
        switch (node->type) {
            // Aritmetika
        case NODE_OP_MINUS: op_code = OP_SUBTRACT; break;
        case NODE_OP_MUL: op_code = OP_MULTIPLY; break;
        case NODE_OP_DIV: op_code = OP_DIVIDE; break;
            // Porovnání
        case NODE_OP_LT: op_code = OP_LESS; break;
        case NODE_OP_GT: op_code = OP_GREATER; break;
        case NODE_OP_LTE: op_code = OP_LESS_EQUAL; break;
        case NODE_OP_GTE: op_code = OP_GREATER_EQUAL; break;
        case NODE_OP_EQ: op_code = OP_EQUAL; break;
        case NODE_OP_NEQ: op_code = OP_NOT_EQUAL; break;
        default: raise_tac_error("Unexpected node type", INTERNAL_ERROR); return NULL;  // Sem se nemělo dostat
        }

        // Generujeme instrukci TAC
        generate_instruction(tac_list, op_code, result_op, left_op,
            right_op);

        // Vracíme výsledek ($tN)
        return result_op;
    }

    /* ===== Zpracování uzlu operátoru "is" ===== */

    case NODE_OP_IS: {
        // Operátor "is" pro kontrolu typu
        AstNode *expr_node = node->child;           // Výraz vlevo
        AstNode *type_node = node->child->sibling;  // Typ vpravo

        // Generujeme kód pro výraz
        Operand *expr_op = tac_gen_recursive(expr_node, tac_list, symtable);

        // Generujeme kód pro typ
        // Vytváříme operand pro typ (RHS)
        //    Bereme 'data.identifier' z uzlu NODE_TYPE_NAME
        //    a měníme ho na *řetězcovou konstantu*.

        Operand *type_op = tac_gen_recursive(type_node, tac_list, symtable);

        Operand *result_op =
            create_temp_operand(tac_list);  // Výsledek kontroly typu $t1

        // Generujeme instrukci
        // OP_IS, $t1, $t0, "Num"
        generate_instruction(tac_list, OP_IS, result_op, expr_op, type_op);

        // Vracíme výsledek ($tN)
        return result_op;
    }

    /* ===== Zpracování uzlů operandů ===== */

    case NODE_ID: {
        // Proměnná (identifikátor)
        TableEntry *var_entry = node->table_entry;
        if (var_entry == NULL) {
            // Chyba: proměnná nenalezena v tabulce symbolů
            return NULL;
        }
        if (has_getter(var_entry->key)) {
            // Vytváříme dočasný operand pro výsledek getteru
            Operand *result_op = create_temp_operand(tac_list);
            
            // Volání getteru pro vlastnost
            Operand *getter_label_op = create_label_operand(var_entry->key);
            generate_instruction(tac_list, OP_CALL, NULL, getter_label_op,
                NULL);
            // Generujeme instrukci získání výsledku getteru
            generate_instruction(tac_list, OP_ASSIGN, result_op, getter_label_op,
                NULL);
            return result_op;
        }
        // Vytváříme operand-symbol pro proměnnou
        return create_symbol_operand(var_entry);
    }

    case NODE_LITERAL_NUM: {
        // Literál čísla
        TacConstant num_const;
        // Určujeme typ čísla (int nebo float)
        if (node->data_type == TYPE_FLOAT) {
            num_const.type = TYPE_FLOAT;
            num_const.value.float_value = node->data.literal_num;
        }
        else {
            num_const.type = TYPE_NUM;
            num_const.value.int_value = (int) node->data.literal_num;
        }
        return create_constant_operand(num_const);
    }

    case NODE_LITERAL_NULL: {
        // Literál null
        TacConstant nil_const;
        nil_const.type = TYPE_NIL;
        return create_constant_operand(nil_const);
    }

    case NODE_LITERAL_STRING: {
        // Literál řetězce
        TacConstant str_const;
        str_const.type = TYPE_STR;
        // Normalizujeme řetězec, odstraňujeme uvozovky
        str_const.value.str_value = node->data.literal_string;
        return create_constant_operand(str_const);
    }

    case NODE_TYPE_NAME: {
        // Uzel typu (například v operátoru "is")
        // Vytváříme operand-konstantu pro typ
        TacConstant type_const;
        type_const.type = TYPE_STR;
        type_const.value.str_value = node->data.identifier;  // e.g., "Num"
        return create_constant_operand(type_const);
    }

    default:
        // Pokud jsme se sem dostali, znamená to, že jsme zapomněli
        // implementovat nějaký NodeType v tomto switchi.
        raise_tac_error("Unimplemented AST node type in TAC generation", INTERNAL_ERROR);


    }  // --- Konec 'switch' ---

    // Tento řádek se NIKDY neprovede, pokud default dělá exit().
    raise_tac_error("Reached unreachable code in TAC generation", INTERNAL_ERROR);
    return NULL;
}

/* ======================================*/
/* ===== Implementace veřejných funkcí =====*/
/* ======================================*/

char *create_unique_label(const char *prefix) {
    // Vytváříme buffer pro řetězec. 256 bajtů stačí s rezervou
    // (Například pro L_WHILE_CONDITION_12345)
    char buffer[256];

    // Formátujeme řetězec: "PREFIX_COUNTER"
    // sprintf funguje jako printf, ale píše do 'buffer'
    sprintf(buffer, "%s_%d", prefix, global_label_counter);

    global_label_counter++;

    // Kopírujeme řetězec z bufferu (stack) do haldy (heap)
    // strdup() = to je malloc() + strcpy()
    // Musíme vrátit řetězec z haldy, protože 'buffer' zemre
    // hned po odchodu z této funkce.
    return strdup_c99(buffer);
}

Operand *create_constant_operand(TacConstant constant) {
    // Alokujeme paměť pro samý operand
    Operand *op = (Operand *)malloc(sizeof(Operand));
    if (op == NULL) {
        // Chyba paměti
        raise_tac_error("Memory allocation failed for constant operand",
            INTERNAL_ERROR);
        return NULL;
    }

    op->type = OPERAND_TYPE_CONSTANT;

    // Kopírujeme celou strukturu constant
    op->data.constant = constant;

    // Pokud je to řetězec, musíme zkopírovat jeho obsah do nové paměti.
    if (constant.type == TYPE_STR) {
        // constant.value.str_value - to je ukazatel na řetězec v AST.
        // Nemůžeme ho jen přiřadit, protože AST bude
        // vyčištěn a ukazatel se stane "visícím".
        op->data.constant.value.str_value = strdup_c99(constant.value.str_value);

        if (op->data.constant.value.str_value == NULL) {
            // Chyba paměti při kopírování řetězce
            free(op);
            raise_tac_error("Memory allocation failed for constant string",
                INTERNAL_ERROR);
            return NULL;
        }
    }

    // Pro TYPE_NUM a TYPE_NIL nic více dělat nemusíme,
    // protože int_value nebo float_value se již zkopírovaly
    return op;
}

void generate_tac(AstNode *ast_root, TACDLList *tac_list, Symtable *global_table) {
    // Vynulujeme počítadla v případě opakovaného volání
    global_temp_counter = 0;
    global_label_counter = 0;

    // Spouštíme rekurzi
    tac_gen_recursive(ast_root, tac_list, global_table);
}

void raise_tac_error(const char *message,
    ErrorCode error_code) {
    // Tiskáme hlavičku chyby (Tučné, Červené)
    fprintf(stderr, "%s%s--- TAC GENERATION ERROR ---%s\n\n", ANSI_STYLE_BOLD,
        ANSI_COLOR_RED, ANSI_COLOR_RESET);

    // Tiskáme detaily (Pouze Červené)
    fprintf(stderr, "%s[!] Fatal Error (Code: %d)%s\n", ANSI_COLOR_RED,
        error_code, ANSI_COLOR_RESET);
    fprintf(stderr, "    Během 3-Adresní Kód-generace:\n\n");

    // Tiskáme samé hlášení o chybě (Normální)
    fprintf(stderr, "    %s\n\n", message);
    // Hlášení o odchodu (Ztlumené)
    fprintf(stderr, "%s    Ukončujeme program.%s\n", ANSI_STYLE_DIM,
        ANSI_COLOR_RESET);

    exit(error_code);  // Ukončujeme program s chybou
}
