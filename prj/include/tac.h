/**
 * @file tac.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Hlavní hlavičkový soubor pro generování a reprezentaci třiadresného kódu (TAC).
 *
 * @author
 *     - Serhij Čepil (253038)
 */

#ifndef TAC_H
#define TAC_H

#include "symtable.h"
#include "ast.h"

/* ======================================*/
/* ===== Enumerace =====*/
/* ======================================*/

/**
 * @brief Enumerace pro kódy operací trojadresního kódu.
 * 
 * Tato enumerace definuje všechny možné operace
 * v trojadresním kódu.
 */
typedef enum{
    /* Operace pro řízení toku */
    OP_JUMP, // Nepodmíněný skok například: JUMP L1
    OP_JUMP_IF_FALSE, // Podmíněný skok například: JUMP_IF_FALSE arg1 L1
    OP_LABEL, // Návěští (adresát pro JUMP) například: LABEL L1
    
    /* Aritmetické operace */
    OP_ADD, // Sčítání například: ADD arg1 arg2
    OP_SUBTRACT, // Odčítání například: SUBTRACT arg1 arg2
    OP_MULTIPLY, // Násobení například: MULTIPLY arg1 arg2
    OP_DIVIDE, // Dělení například: DIVIDE arg1 arg2

    
    /* Operace konkatenace řetězců */
    OP_CONCAT, // Konkatenace řetězců například: CONCAT arg1 arg2
    OP_MULTIPLY_STRING, // Násobení řetězce například: MULTIPLY_STRING arg1 arg2
    
    /* Operace porovnání */
    OP_LESS, // Menší například: OP_LESS arg1 arg2
    OP_GREATER, // Větší například: OP_GREATER arg1 arg2
    OP_LESS_EQUAL, // Menší nebo rovno například: OP_LESS_EQUAL arg1 arg2
    OP_GREATER_EQUAL, // Větší nebo rovno například: OP_GREATER_EQUAL arg1 arg2
    OP_EQUAL, // Rovno například: OP_EQUAL arg1 arg2
    OP_NOT_EQUAL, // Nerovno například: OP_NOT_EQUAL arg1 arg2
    
    /* Logické operace */
    OP_IS,  // Operátor 'is' například: IS arg1 arg2

    /* Přiřazení */
    OP_ASSIGN, // Přiřazení například: ASSIGN arg1 result
    OP_DECLARE, // Deklarace proměnné například: DECLARE result

    /* Operace práce s funkcemi */
    OP_CALL, // Volání funkce například: CALL arg1 (kde arg1 - jméno funkce)
    OP_RETURN, // Návrat z funkce například: RETURN arg1
    OP_PARAM, // Předání parametru funkci například: PARAM arg1
    OP_FUNCTION_BEGIN, // Začátek funkce například: FUNCTION_BEGIN arg1 (kde arg1 - jméno funkce)
    OP_FUNCTION_END, // Konec funkce například: FUNCTION_END arg1 (kde arg1 - jméno funkce)

} TacOperationCode;

/**
 * @brief Typy operandů v trojadresním kódu.
 * 
 * @note Každý operand může být jednoho z těchto typů.
 * @note Například to může být symbol, konstanta nebo návěští.
 * @note Pokud operand není používán, jeho typ bude OPERAND_TYPE_EMPTY.
 * 
 */
typedef enum{
    OPERAND_TYPE_EMPTY,    // Prázdný operand (není používán)
    OPERAND_TYPE_SYMBOL,   // Symbol z symtable ('a', 'b')
    OPERAND_TYPE_CONSTANT, // Konstanta (10, "hello")
    OPERAND_TYPE_LABEL,    // Návěští ('L1', 'L_ELSE')
    OPERAND_TYPE_TEMP      // Dočasná proměnná ('$t1', '$t2')
} OperandType;


/* ======================================*/
/* ===== Struktury =====*/
/* ======================================*/

/**
 * @brief Struktura pro reprezentaci konstanty v trojadresním kódu.
 * 
 * V závislosti na typu konstanty,
 * hodnota je uložena v odpovídajícím poli union.
 * 
 * @note Pole 'type' ukazuje na typ dat konstanty.
 * @note Například pokud je konstanta používána, tato struktura může ukazovat na typ INT, FLOAT atd.
 *  
 */
typedef struct {
    DataType type;
    union {
        int int_value;
        float float_value;
        char *str_value;
    } value;
} TacConstant;

/**
 * @brief Struktura pro reprezentaci operandu v trojadresním kódu.
 * 
 * V závislosti na typu operandu,
 * odpovídající pole union obsahuje data.
 * 
 * @note Pokud je to prostě symbol, pak je uložen ukazatel na záznam tabulky symbolů.
 * @note Pokud je to konstanta, pak je uložena struktura TacConstant.
 * @note Pokud je to návěští, pak je uložen řetězec se jménem návěští.
 * @note Pokud je to dočasná proměnná, pak je uložen unikátní identifikátor.
 */
typedef struct{
    OperandType type;
    union {
        TableEntry *symbol_entry; // Pokud typ SYMBOL
        TacConstant constant;     // Pokud typ CONSTANT
        char *label_name;         // Pokud typ LABEL
        int temp_id;              // Pokud typ TEMP
    } data;
} Operand;

/**
 * @brief Struktura pro reprezentaci instrukce trojadresního kódu.
 * 
 * Každá instrukce obsahuje kód operace a až tři operandy.
 * Výsledek operace je také uložen v samostatném operandu.
 * 
 * @note Pokud operand není používán, bude NULL.
 */
typedef struct{
    TacOperationCode operation_code;
    Operand *arg1;
    Operand *arg2;
    Operand *result;
} TacInstruction;


/* ====================================== */
/* ===== Struktura TAC_DLL =====*/
/* ====================================== */



typedef struct TACDLLElement {
	TacInstruction *tac_instruction;
	struct TACDLLElement *prev_element;
	struct TACDLLElement *next_element;
} *TACDLLElementPtr;

typedef struct{
  TACDLLElementPtr first_element;
  TACDLLElementPtr last_element;
  TACDLLElementPtr active_element;
} TACDLList;

/* ======================================*/
/* ===== Prototypy funkcí TACDLL =====*/
/* ======================================*/

/**
 * @brief Externí funkce pro čištění dat uložených v seznamu.
 * Měla by být definována někde jinde (např. v tac_playground.c)
 * Pouze ji zde deklarujeme, aby ji common.c mohl používat.
 */
void free_tac_instruction(TacInstruction *tac_instruction);


// ======================================*/
// ===== Veřejné funkce pro práci s DLL =====*/
// ======================================*/

/**
 * @brief Inicializuje DLL
 * 
 * @param list Ukazatel kam inicializovat seznam
 */
void TACDLL_Init(TACDLList *list);


/**
 * @brief Uvolňuje paměť všech prvků v seznamu a také sám seznam
 * 
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 */
void TACDLL_Dispose(TACDLList *list);

/**
 * Vkládá nový prvek na začátek seznamu list.
 * V případě, že není dostatek paměti pro nový prvek při operaci malloc,
 * volá funkci TACDLL_Error().
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 * @param tac_intructions Hodnota pro vložení na začátek seznamu
 */
void TACDLL_InsertFirst(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Vkládá nový prvek na konec seznamu list (symetrická operace k TACDLL_InsertFirst).
 * V případě, že není dostatek paměti pro nový prvek při operaci malloc,
 * volá funkci TACDLL_Error().
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 * @param tac_intructions Hodnota pro vložení na konec seznamu
 */
void TACDLL_InsertLast(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Nastaví první prvek seznamu list jako aktivní.
 * Implementujte funkci jako jeden příkaz, netestujte,
 * zda je seznam list prázdný.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 */
void TACDLL_First(TACDLList *list);

/**
 * Nastaví poslední prvek seznamu list jako aktivní.
 * Implementujte funkci jako jeden příkaz, netestujte,
 * zda je seznam list prázdný.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 */
void TACDLL_Last(TACDLList *list);

/**
 * Prostřednictvím parametru tac_intructions vrací hodnotu prvního prvku seznamu list.
 * Pokud je seznam list prázdný, volá funkci TACDLL_Error().
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 * @param tac_intructions Ukazatel na cílovou proměnnou
 */
void TACDLL_GetFirst(TACDLList *list, TacInstruction **tac_intructions);

/**
 * Prostřednictvím parametru tac_intructions vrací hodnotu posledního prvku seznamu list.
 * Pokud je seznam list prázdný, volá funkci TACDLL_Error().
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 * @param tac_intructions Ukazatel na cílovou proměnnou
 */
void TACDLL_GetLast(TACDLList *list, TacInstruction **tac_intructions);

/**
 * Odstraní první prvek seznamu list.
 * Pokud byl první prvek aktivní, aktivita se ztratí.
 * Pokud byl seznam list prázdný, nic se nestane.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 */
void TACDLL_DeleteFirst(TACDLList *list);

/**
 * Odstraní poslední prvek seznamu list.
 * Pokud byl poslední prvek aktivní, aktivita seznamu se ztratí.
 * Pokud byl seznam list prázdný, nic se nestane.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 */
void TACDLL_DeleteLast(TACDLList *list);

/**
 * Odstraní prvek seznamu list za aktivním prvkem.
 * Pokud seznam list není aktivní nebo pokud je aktivní prvek
 * posledním prvkem seznamu, nic se nestane.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 */
void TACDLL_DeleteAfter(TACDLList *list);

/**
 * Odstraní prvek před aktivním prvkem seznamu list.
 * Pokud seznam list není aktivní nebo pokud je aktivní prvek
 * prvním prvkem seznamu, nic se nestane.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 */
void TACDLL_DeleteBefore(TACDLList *list);

/**
 * Vkládá prvek za aktivní prvek seznamu list.
 * Pokud seznam list nebyl aktivní, nic se nestane.
 * V případě, že není dostatek paměti pro nový prvek při operaci malloc,
 * volá funkci TACDLL_Error().
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 * @param tac_intructions Hodnota pro vložení do seznamu za současný aktivní prvek
 */
void TACDLL_InsertAfter(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Vkládá prvek před aktivní prvek seznamu list.
 * Pokud seznam list nebyl aktivní, nic se nestane.
 * V případě, že není dostatek paměti pro nový prvek při operaci malloc,
 * volá funkci TACDLL_Error().
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 * @param tac_intructions Hodnota pro vložení do seznamu před současný aktivní prvek
 */
void TACDLL_InsertBefore(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Prostřednictvím parametru tac_intructions vrací hodnotu aktivního prvku seznamu list.
 * Pokud seznam list není aktivní, volá funkci TACDLL_Error().
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 * @param tac_intructions Ukazatel na cílovou proměnnou
 */
void TACDLL_GetValue(TACDLList *list, TacInstruction **tac_intructions);

/**
 * Přepisuje obsah aktivního prvku seznamu list.
 * Pokud seznam list není aktivní, nic nedělá.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 * @param tac_intructions Nová hodnota současného aktivního prvku
 */
void TACDLL_SetValue(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Přesouvá aktivitu na následující prvek seznamu list.
 * Pokud seznam není aktivní, nic nedělá.
 * Všimněte si, že při aktivitě na posledním prvku se seznam stává neaktivním.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 */
void TACDLL_Next(TACDLList *list);

/**
 * Přesouvá aktivitu na předchozí prvek seznamu list.
 * Pokud seznam není aktivní, nic nedělá.
 * Všimněte si, že při aktivitě na prvním prvku se seznam stává neaktivním.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 */
void TACDLL_Previous(TACDLList *list);

/**
 * Pokud je seznam list aktivní, vrací nenulovou hodnotu, jinak vrací 0.
 * Funkci je účelné implementovat jedním příkazem return.
 *
 * @param list Ukazatel na inicializovanou strukturu obousměrného seznamu
 *
 * @returns Nenulová hodnota v případě aktivity prvku seznamu, jinak nula
 */
bool TACDLL_IsActive(TACDLList *list);

// ======================================*/
// ===== Veřejné funkce TAC =====*/
// ======================================*/

/**
 * @brief Vytváří unikátní jméno návěští se zadaným prefixem (L_IF1, L_ELSE2
 * atd.).
 *
 * Přidává globální počítadlo k prefixu, aby zaručilo unikátnost.
 * Globální počítadlo se zvyšuje při každém volání.
 *
 * @param prefix Prefix pro jméno návěští (například "L_IF", "L_ELSE").
 * @return Unikátní jméno návěští v dynamicky alokované paměti.
 * @note Volající je povinen uvolnit paměť.
 */
char *create_unique_label(const char *prefix);

/**
 * @brief Vytváří operand-konstantu.
 * @attention Pokud je konstanta TYPE_STR, tato funkce
 * vytváří kopii řetězce na haldě. Tato kopie bude
 * uvolněna, když 'free_tac_instruction' bude
 * čistit 'Operand'.
 *
 * @param constant Struktura TacConstant (může být z AST).
 * @return Ukazatel na nový Operand (alokován na haldě).
 */
Operand *create_constant_operand(TacConstant);


/**
 * @brief Pomocná funkce pro čištění operandu.
 * @param op Ukazatel na operand pro čištění.
 * @note Uvolňuje paměť zabranou operandem a jeho vnitřními daty.
 */
void free_operand(Operand *op);


/**
 * @brief Hlavní funkce pro generování trojadresního kódu (3AC) z AST.
 * 
 * Tato veřejná funkce spouští rekurzivní generování 3AC.
 * 
 * @param ast_node Kořenový uzel AST.
 * @param tac_list Seznam pro uložení vygenerovaných instrukcí 3AC.
 * @param global_table Globální tabulka symbolů.
 * 
 * @note Tato funkce inicializuje globální počítadla pro dočasné proměnné a návěští.
 * Uděláno pro podporu opakovaných volání.
 */
void generate_tac(AstNode *ast_node, TACDLList *tac_list, Symtable *global_table);

/**
 * @brief Uvolňuje paměť zabranou seznamem trojadresního kódu.
 * 
 * @param tac_list Seznam instrukcí pro uvolnění.
 */
void free_tac(TACDLList *tac_list);

/**
 * @brief Tiskne obsah seznamu 3AC v čitelné podobě.
 * @param tac_list Vyplněný seznam instrukcí.
 */
void print_tac_list(TACDLList *tac_list);

#endif // TAC_H
