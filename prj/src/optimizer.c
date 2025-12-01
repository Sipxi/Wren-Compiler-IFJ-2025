/**
 * @file optimizer.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Implementace optimalizátoru tříadresného kódu (TAC).
 *
 * @author
 *     - Serhij Čepil (253038)
 */

#include "optimizer.h"
#include "symtable.h"

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>


 /* ======================================*/
 /* ===== Globální proměnné ========*/
 /* ======================================*/

bool optimization_performed;

/* ======================================*/
/* ===== Prototypy privátních funkcí =====*/
/* ======================================*/

/* ========================================*/
/* ===== Techniky optimalizace =====*/
/* ========================================*/

 /**
  * @brief Provádí optimalizaci "Constant Folding" na seznamu TAC instrukcí.
  *
  * Tato funkce hledá aritmetické operace s konstantními operandy
  * a vypočítává je během kompilace, nahrazuje instrukci
  * přiřazením výsledku.
  * @note Nastavuje globální proměnnou 'optimization_performed' na true
  * pokud byla provedena alespoň jedna optimalizace.
  * 
  * Například :
  * t1 = 2 + 3  ->  t1 = 5
  *
  * @param tac_list Seznam TAC instrukcí pro optimalizaci.
  */
void constant_folding(TACDLList *tac_list);

/**
 * @brief Odstraňuje nedostupný kód ze seznamu TAC instrukcí.
 *
 * Tato funkce hledá instrukce, které nikdy nebudou provedeny
 * (například instrukce po bezpodmínečném skoku nebo návratu z funkce)
 * a odstraňuje je ze seznamu.
 * 
 * @note Nastavuje globální proměnnou 'optimization_performed' na true
 * pokud byla provedena alespoň jedna optimalizace.
 *
 * @param tac_list Seznam TAC instrukcí pro optimalizaci.
 */
void unreachable_code(TACDLList *tac_list);


/* ======================================*/
/* ===== Pomocné funkce pro Constant Folding ===== */
/* ======================================*/

/**
 * @brief Kontroluje, zda je operace aritmetická.
 *
 * @param op_code Kód operace TAC.
 * @return true pokud je operace aritmetická, jinak false.
 */
bool can_be_folded(TacOperationCode op_code);

/**
 * @brief Kontroluje, zda jsou oba argumenty instrukce konstanty.
 *
 * @param instr Ukazatel na TAC instrukci.
 * @return true pokud jsou oba argumenty konstanty, jinak false.
 */
bool are_args_constant(TacInstruction *instr);

/**
 * @brief Kontroluje, zda je číslo celé a nastavuje vhodný typ v TacConstant.
 *
 * @param result_const Ukazatel na TacConstant pro nastavení typu a hodnoty.
 * @param result_value Vypočítaná hodnota.
 */
void set_num_constant_value(TacConstant *result_const, float result_value);

/**
 * @brief Spojuje dvě řetězcové konstanty z TAC instrukce.
 *
 * @param instr Ukazatel na TAC instrukci s operací spojování.
 * @return Ukazatel na nový řetězec obsahující výsledek spojení.
 *         Paměť je alokována na haldě a musí být uvolněna volající stranou.
 */
char *concat_string_constants(TacInstruction *instr);

/**
 * @brief Vypočítává výsledek aritmetické operace s číselnými konstantami.
 *
 * @param instr Ukazatel na TAC instrukci s aritmetickou operací.
 * @param args_type Typ argumentů (TYPE_NUM nebo TYPE_FLOAT).
 * @return Vypočítaná číselná hodnota.
 */
float calculate_num_constant(TacInstruction *instr);

/**
 * @brief Kontroluje, zda je float celé číslo.
 *
 * @param value Číslo pro kontrolu.
 * @return true pokud je číslo celé, jinak false.
 */
bool check_whole_number(float value);

/**
 * @brief Násobí řetězcovou konstantu číslenou konstantou z TAC instrukce.
 *
 * @param instr Ukazatel na TAC instrukci s operací násobení řetězce číslem.
 * @return Ukazatel na nový řetězec obsahující výsledek násobení.
 *         Paměť je alokována na haldě a musí být uvolněna volající stranou.
 */
char *multiply_string_constant(TacInstruction *instr);

/* ====================================== */
/* ===== Implementace soukromých funkcí ===== */
/* ====================================== */



/* ====================================== */
/* ===== Implementace pomocných funkcí pro Constant Folding ===== */
/* =========================================*/

char *multiply_string_constant(TacInstruction *instr) {
    // Extrahujeme řetězcovou hodnotu a počet opakování
    // Očekáváme, že arg1 - řetězec, arg2 - číslo
    TacConstant str_const = instr->arg1->data.constant;
    TacConstant num_const = instr->arg2->data.constant;
    int repeat_count = 0;
    if (num_const.type == TYPE_NUM) {
        repeat_count = num_const.value.int_value;
    }
    else if (num_const.type == TYPE_FLOAT && check_whole_number(num_const.value.float_value)) {
        repeat_count = (int)num_const.value.float_value;
    }
    else {
        // Nesprávný typ pro násobení řetězce
        return NULL;
    }

    if (repeat_count < 0) {
        // Záporný počet opakování nemá smysl
        return NULL;
    }

    // Alokujeme paměť pro nový řetězec
    size_t str_length = strlen(str_const.value.str_value);
    size_t new_length = str_length * repeat_count + 1;

    char *result_str = (char *)malloc(new_length);
    if (result_str == NULL) {
        // Chyba paměti
        return NULL;
    }

    result_str[0] = '\0'; // Výsledek - prázdný řetězec ""
    // Efektivně kopírujeme řetězec N krát
    char *current_pos = result_str;
    for (int i = 0; i < repeat_count; i++) {
        memcpy(current_pos, str_const.value.str_value, str_length);
        current_pos += str_length;
    }
    *current_pos = '\0'; // Nastavujeme ukončující nulu
    return result_str;
}

void set_num_constant_value(TacConstant *result_const, float result_value) {
    if (check_whole_number(result_value)) {
        result_const->type = TYPE_NUM;
        result_const->value.int_value = (int)result_value;
    }
    else {
        result_const->type = TYPE_FLOAT;
        result_const->value.float_value = result_value;
    }
}

bool check_whole_number(float value) {
    return value == (int)value;
}

char *concat_string_constants(TacInstruction *instr) {
    // Extrahujeme řetězcové hodnoty
    TacConstant arg1_const = instr->arg1->data.constant;
    TacConstant arg2_const = instr->arg2->data.constant;

    // Alokujeme paměť pro nový řetězec
    size_t new_length = strlen(arg1_const.value.str_value) + strlen(arg2_const.value.str_value) + 1;
    char *result_str = (char *)malloc(new_length);
    if (result_str == NULL) {
        // Chyba paměti
        //! Zpracovat chybu alokace paměti vhodným způsobem
        return NULL;
    }

    // Spojujeme řetězce
    strcpy(result_str, arg1_const.value.str_value);
    strcat(result_str, arg2_const.value.str_value);

    return result_str;
}

float calculate_num_constant(TacInstruction *instr) {
    // Extrahujeme konstantní hodnoty
    TacConstant arg1_const = instr->arg1->data.constant;
    TacConstant arg2_const = instr->arg2->data.constant;

    // Převádíme na float pro výpočty
    float arg1_value = arg1_const.type == TYPE_NUM ? (float)arg1_const.value.int_value : arg1_const.value.float_value;
    float arg2_value = arg2_const.type == TYPE_NUM ? (float)arg2_const.value.int_value : arg2_const.value.float_value;
    float result;

    switch (instr->operation_code) {
    case OP_ADD:
        result = arg1_value + arg2_value;
        break;
    case OP_SUBTRACT:
        result = arg1_value - arg2_value;
        break;
    case OP_MULTIPLY:
        result = arg1_value * arg2_value;
        break;
    case OP_DIVIDE:
        // Kontrola dělení nulou
        if (arg2_value == 0.0f) {
            // Chyba dělení nulou
            //! Zpracovat chybu dělení nulou vhodným způsobem
            return nanf("");
        }
        result = arg1_value / arg2_value;
        break;
    default:
        // Neznámá operace
        //! Možná stojí za to přidat zpracování chyb nebo logování
        return nanf("");
        break;
    }
    return result;
}

bool can_be_folded(TacOperationCode op_code) {
    return (op_code == OP_ADD ||
        op_code == OP_SUBTRACT ||
        op_code == OP_DIVIDE ||
        op_code == OP_MULTIPLY ||
        op_code == OP_CONCAT ||
        op_code == OP_MULTIPLY_STRING);
}

bool are_args_constant(TacInstruction *instr) {
    return (instr->arg1->type == OPERAND_TYPE_CONSTANT &&
        instr->arg2->type == OPERAND_TYPE_CONSTANT);
}

/* ======================================*/
/* ===== Implementace technik optimalizace =====*/
/* ======================================*/

void constant_folding(TACDLList *tac_list) {
    TACDLL_First(tac_list);

    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        // Kontrolujeme, zda je operace aritmetická
        if (!can_be_folded(instr->operation_code)) {
            TACDLL_Next(tac_list);
            continue;
        }
        // Kontrolujeme, že oba argumenty - konstanty
        if (!are_args_constant(instr)) {
            TACDLL_Next(tac_list);
            continue;
        }

        bool optimized = false;
        TacConstant result_const;

        TacConstant arg1_const = instr->arg1->data.constant;
        TacConstant arg2_const = instr->arg2->data.constant;
        // Oba argumenty - konstanty, provádíme výpočet
        // Kontrolujeme typ konstant

        // =====
        // Případ: "a" + "b"
        // =====
        if (arg1_const.type == TYPE_STR &&
            arg2_const.type == TYPE_STR &&
            instr->operation_code == OP_CONCAT) {
            char *result_str = concat_string_constants(instr);

            if (result_str != NULL) {

                result_const.type = TYPE_STR;
                result_const.value.str_value = result_str;
                optimized = true;
            }

        }
        // =====
        // Případ: "a" * 5 nebo "a" * 5.0
        // =====
        else if (arg1_const.type == TYPE_STR &&
            (arg2_const.type == TYPE_NUM || arg2_const.type == TYPE_FLOAT) &&
            instr->operation_code == OP_MULTIPLY_STRING) {
            char *result_str = multiply_string_constant(instr);
            if (result_str != NULL) {
                result_const.type = TYPE_STR;
                result_const.value.str_value = result_str;
                optimized = true;
            }
        }
        // =====
        // Případ: 5 * 5 nebo 5.0 + 3.2 atd.
        // =====
        else if ((arg1_const.type == TYPE_NUM ||
            arg1_const.type == TYPE_FLOAT) &&
            (arg2_const.type == TYPE_NUM ||
                arg2_const.type == TYPE_FLOAT)) {

            float result_value = calculate_num_constant(instr);

            if (!isnan(result_value)) {
                set_num_constant_value(&result_const, result_value);
                optimized = true;

            }

        }

        // =====
        // Pokud je optimalizace provedena, aktualizujeme instrukci
        // =====
        if (optimized) {
            //* Označujeme, že byla provedena optimalizace
            optimization_performed = true;
            // Uvolňujeme staré operandy
            free_operand(instr->arg1);
            free_operand(instr->arg2);


            // Vytváříme nový operand pro výsledek
            Operand *result_op = create_constant_operand(result_const);

            // Aktualizujeme instrukci na přiřazení
            instr->operation_code = OP_ASSIGN;
            instr->arg1 = result_op;
            instr->arg2 = NULL;

        }

        // Přecházíme na další instrukci
        TACDLL_Next(tac_list);


    }
}

void unreachable_code(TACDLList *tac_list) {
    TACDLL_First(tac_list);

    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        // Pokud je instrukce - bezpodmínečný skok nebo návrat z funkce
        if (instr->operation_code == OP_JUMP ||
            instr->operation_code == OP_RETURN) {
            // Po této instrukci všechny následující do návěští - nedostupný kód
            TACDLL_Next(tac_list);
            while (TACDLL_IsActive(tac_list)) {
                TacInstruction *next_instr;
                TACDLL_GetValue(tac_list, &next_instr);
                // Pokud je dosaženo návěští, ukončujeme mazání
                if (next_instr->operation_code == OP_LABEL ||
                    next_instr->operation_code == OP_FUNCTION_END) {
                    break;
                }
                // Odstraňujeme nedostupnou instrukci
                TACDLL_Previous(tac_list);
                TACDLL_DeleteAfter(tac_list);

                //* Označujeme, že byla provedena optimalizace
                optimization_performed = true;

                TACDLL_Next(tac_list);
            }

        }
        TACDLL_Next(tac_list);
    }
}



/* ======================================*/
/* ===== Implementace veřejných funkcí =====*/
/* ======================================*/

void optimize_tac(TACDLList *tac_list) {
    optimization_performed = true;

    while (optimization_performed) {
        optimization_performed = false;

        // Volání optimalizace Constant Folding
        constant_folding(tac_list);
        // Volání optimalizace Unreachable Code
        unreachable_code(tac_list);

    }
}   