/**
 * @file optimizer.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Implementace optimalizátoru tříadresného kódu (TAC).
 *
 * @author
 * - Serhij Čepil (253038)
 */

#include "optimizer.h"
#include "symtable.h"
#include "tac.h"

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define MAX_PROPAGATED_VARS 64
// Maximální ID pro dočasné proměnné $tX
// Uděláno kvůli alokaci pole pro sledování použití v DCE
#define MAX_TEMP_ID 16384

/* ======================================*/
/* ===== Globální proměnné ========*/
/* ======================================*/

bool optimization_performed;

/* ======================================*/
/* ===== Pomocné struktury pro DCE =====*/
/* ======================================*/

typedef struct VarUsage {
    bool is_temp;           // True pro $tX, False pro proměnné
    union {
        char *key;          // Jméno proměnné (pro SYMBOL)
        int temp_id;        // ID (pro TEMP)
    } data;
    int count;              // Počet použití
    struct VarUsage *next;
} VarUsage;

/* ======================================*/
/* ===== Pomocná struktura pro Propagation =====*/
/* ======================================*/

typedef struct {
    char *key;              // Jméno proměnné (ze symtable)
    TacConstant constant;   // Hodnota konstanty
    bool active;            // Je slot obsazený
} PropagatorSlot;

typedef struct {
    PropagatorSlot slots[MAX_PROPAGATED_VARS];
} PropagatorMap;

/* ======================================*/
/* ===== Deklarace technik optimalizace =====*/
/* ======================================*/

/**
 * @brief Provádí optimalizaci Constant Folding na zadaném seznamu TAC instrukcí.
 *
 * Tato funkce prochází seznam TAC instrukcí a hledá instrukce, které mohou být
 * vyhodnoceny během kompilace (tj. jejich operandy jsou konstanty).
 * Příklad:
 *         Původní instrukce: ADD 5, 10 -> $t1
 *         Optimalizovaná instrukce: ASSIGN 15 -> $t1
 * 
 * @note nastaví globální příznak optimization_performed na true, pokud dojde k jakékoli optimalizaci. 
 * @param tac_list Seznam TAC instrukcí k optimalizaci.
 */
static void constant_folding(TACDLList *tac_list);

/**
 * @brief Provádí optimalizaci Dead Code Elimination na zadaném seznamu TAC instrukcí.
 *
 * Tato funkce prochází seznam TAC instrukcí a odstraňuje ty, které nikdy nebudou
 * vykonány (například instrukce za nepodmíněným skokem).
 * Příklad:
 *         Původní instrukce:
 *              JUMP L1
 *              ASSIGN 5 -> a   ; Tato instrukce je mrtvý kód
 *        Optimalizovaná instrukce:
 *              JUMP L1
 * 
 * @note nastaví globální příznak optimization_performed na true, pokud dojde k jakékoli optimalizaci.
 * @param tac_list Seznam TAC instrukcí k optimalizaci.
 * 
 */
static void unreachable_code(TACDLList *tac_list);

/**
 * @brief Provádí propagaci konstant na zadaném seznamu TAC instrukcí.
 * 
 * Tato funkce prochází seznam TAC instrukcí a nahrazuje proměnné,
 * které mají známé konstantní hodnoty, těmito hodnotami.
 * Příklad:
 *        Původní instrukce:
 *              ASSIGN 10 -> a
 *              ADD a, 5 -> $t1
 *        Optimalizovaná instrukce:
 *              ASSIGN 15 -> $t1
 * @note nastaví globální příznak optimization_performed na true, pokud dojde k jakékoli optimalizaci.
 * @param tac_list Seznam TAC instrukcí k optimalizaci.
 */
static void constant_propagation(TACDLList *tac_list);

/**
 * @brief Provádí optimalizaci Dead Code Elimination (DCE) na zadaném seznamu TAC instrukcí.
 *
 * Tato funkce analyzuje použití proměnných v seznamu TAC instrukcí
 * a odstraňuje ty instrukce, které přiřazují hodnoty do proměnných,
 * které nejsou nikdy použity.
 * Příklad:
 *         Původní instrukce:
 *              ASSIGN 5 -> a   ; Proměnná 'a' není nikdy použita
 *              ADD 10, 20 -> $t1
 *        Optimalizovaná instrukce:
 *              ADD 10, 20 -> $t1
 * 
 * @note nastaví globální příznak optimization_performed na true, pokud dojde k jakékoli optimalizaci.
 * @param tac_list Seznam TAC instrukcí k optimalizaci.
 */
static void dead_code_elimination(TACDLList *tac_list);


/* ======================================*/
/* ===== Pomocné funkce pro Constant Folding ===== */
/* ======================================*/

/**
 * @brief Kontroluje, zda může být daná instrukce složena (folded).
 * 
 * @param op_code Kód operace instrukce.
 * @return true Pokud instrukce může být složena (folded).
 */
static bool can_be_folded(TacOperationCode op_code);

/**
 * @brief Kontroluje, zda jsou oba argumenty instrukce konstanty.
 * 
 * @param instr Ukazatel na TAC instrukci.
 * @return true Pokud jsou oba argumenty konstanty.
 */
static bool are_args_constant(TacInstruction *instr);

/**
 * @brief Nastavuje hodnotu číselné konstanty.
 * 
 * @param result_const Ukazatel na konstantu, do které se nastaví hodnota.
 * @param result_value Hodnota, která se nastaví.
 */
static void set_num_constant_value(TacConstant *result_const, float result_value);

/**
 * @brief Provádí složení (folding) číselných konstant pro danou instrukci.
 * 
 * @param instr Ukazatel na TAC instrukci.
 * @return Hodnota výsledné číselné konstanty.
 */
static char *concat_string_constants(TacInstruction *instr);

/**
 * @brief Provádí složení (folding) číselných konstant pro danou instrukci.
 * 
 * @param instr Ukazatel na TAC instrukci.
 * @return Hodnota výsledné číselné konstanty.
 */
static float calculate_num_constant(TacInstruction *instr);

/**
 * @brief Kontroluje, zda je zadaná hodnota float celé číslo.
 * 
 * @param value Hodnota float k kontrole.
 * @return true Pokud je hodnota celé číslo.
 */
static bool check_whole_number(float value);

/**
 * @brief Provádí složení (folding) řetězcových konstant pro danou instrukci.
 * 
 * @param instr Ukazatel na TAC instrukci.
 * @return Hodnota výsledné číselné konstanty.
 */
static char *multiply_string_constant(TacInstruction *instr);


/* ======================================*/
/* ===== Pomocné funkce pro DCE ===== */
/* ======================================*/

/**
 * @brief Kontroluje, zda má instrukce vedlejší efekty.
 * 
 * Vedlejší efekty jsou operace, které mění stav programu nebo mají vliv mimo svůj výstup,
 * například volání funkcí, skoky, návraty z funkcí apod.
 * 
 * @param op Kód operace instrukce.
 * @return true Pokud instrukce má vedlejší efekty.
 */
static bool op_has_global_effect(TacOperationCode op);

/**
 * @brief Kontroluje, zda je proměnná globální.
 * 
 * @param name Jméno proměnné.
 * @return true Pokud je proměnná globální.
 */
static bool is_global_var(const char *name);

/* ======================================*/
/* ===== Funkce pro VarUsage strukturu =====*/
/* ======================================*/

static void usage_inc(VarUsage **head, char *key, int temp_id, bool is_temp) {
    VarUsage *curr = *head;
    while (curr) {
        bool match = false;
        
        if (is_temp) {
            // Porovnáváme ID pouze pokud je uzel také TEMP
            if (curr->is_temp && curr->data.temp_id == temp_id) match = true;
        } else {
            // Porovnáváme klíč pouze pokud uzel není TEMP
            if (!curr->is_temp && strcmp(curr->data.key, key) == 0) match = true;
        }

        if (match) {
            curr->count++;
            return;
        }
        curr = curr->next;
    }

    // Pokud neexistuje, vytvoříme nový uzel
    VarUsage *new_node = malloc(sizeof(VarUsage));
    if (new_node) {
        new_node->is_temp = is_temp;
        if (is_temp) {
            new_node->data.temp_id = temp_id;
        } else {
            new_node->data.key = key;
        }
        new_node->count = 1;
        new_node->next = *head;
        *head = new_node;
    }
}

static int usage_get(VarUsage *head, char *key, int temp_id, bool is_temp) {
    while (head) {
        if (is_temp) {
            if (head->is_temp && head->data.temp_id == temp_id) return head->count;
        } else {
            if (!head->is_temp && strcmp(head->data.key, key) == 0) return head->count;
        }
        head = head->next;
    }
    return 0; // Pokud není v seznamu, použití je 0
}

static void usage_list_free(VarUsage *head) {
    while (head) {
        VarUsage *tmp = head;
        head = head->next;
        free(tmp);
    }
}

/* ======================================*/
/* ===== Funkce pro PropagatorMap strukturu =====*/
/* ======================================*/

static void prop_map_clear(PropagatorMap *map) {
    for (int i = 0; i < MAX_PROPAGATED_VARS; i++) {
        map->slots[i].active = false;
    }
}

static void prop_map_set(PropagatorMap *map, char *key, TacConstant val) {
    for (int i = 0; i < MAX_PROPAGATED_VARS; i++) {
        if (map->slots[i].active && strcmp(map->slots[i].key, key) == 0) {
            map->slots[i].constant = val;
            return;
        }
    }
    for (int i = 0; i < MAX_PROPAGATED_VARS; i++) {
        if (!map->slots[i].active) {
            map->slots[i].active = true;
            map->slots[i].key = key;
            map->slots[i].constant = val;
            return;
        }
    }
}

static bool prop_map_get(PropagatorMap *map, char *key, TacConstant *out_val) {
    for (int i = 0; i < MAX_PROPAGATED_VARS; i++) {
        if (map->slots[i].active && strcmp(map->slots[i].key, key) == 0) {
            *out_val = map->slots[i].constant;
            return true;
        }
    }
    return false;
}

static void prop_map_remove(PropagatorMap *map, char *key) {
    for (int i = 0; i < MAX_PROPAGATED_VARS; i++) {
        if (map->slots[i].active && strcmp(map->slots[i].key, key) == 0) {
            map->slots[i].active = false;
            return;
        }
    }
}

/* ====================================== */
/* ===== Implementace pomocných funkcí pro Constant Folding ===== */
/* =========================================*/

static char *multiply_string_constant(TacInstruction *instr) {
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
        return NULL;
    }

    if (repeat_count < 0) return NULL;

    size_t str_length = strlen(str_const.value.str_value);
    size_t new_length = str_length * repeat_count + 1;

    char *result_str = (char *)malloc(new_length);
    if (result_str == NULL) return NULL;

    result_str[0] = '\0';
    char *current_pos = result_str;
    for (int i = 0; i < repeat_count; i++) {
        memcpy(current_pos, str_const.value.str_value, str_length);
        current_pos += str_length;
    }
    *current_pos = '\0';
    return result_str;
}

static void set_num_constant_value(TacConstant *result_const, float result_value) {
    if (check_whole_number(result_value)) {
        result_const->type = TYPE_NUM;
        result_const->value.int_value = (int)result_value;
    }
    else {
        result_const->type = TYPE_FLOAT;
        result_const->value.float_value = result_value;
    }
}

static bool check_whole_number(float value) {
    return value == (int)value;
}

static char *concat_string_constants(TacInstruction *instr) {
    TacConstant arg1_const = instr->arg1->data.constant;
    TacConstant arg2_const = instr->arg2->data.constant;

    size_t new_length = strlen(arg1_const.value.str_value) + strlen(arg2_const.value.str_value) + 1;
    char *result_str = (char *)malloc(new_length);
    if (result_str == NULL) return NULL;

    strcpy(result_str, arg1_const.value.str_value);
    strcat(result_str, arg2_const.value.str_value);

    return result_str;
}

static float calculate_num_constant(TacInstruction *instr) {
    TacConstant arg1_const = instr->arg1->data.constant;
    TacConstant arg2_const = instr->arg2->data.constant;

    float arg1_value = arg1_const.type == TYPE_NUM ? (float)arg1_const.value.int_value : arg1_const.value.float_value;
    float arg2_value = arg2_const.type == TYPE_NUM ? (float)arg2_const.value.int_value : arg2_const.value.float_value;
    float result = 0.0f;

    switch (instr->operation_code) {
    case OP_ADD: result = arg1_value + arg2_value; break;
    case OP_SUBTRACT: result = arg1_value - arg2_value; break;
    case OP_MULTIPLY: result = arg1_value * arg2_value; break;
    case OP_DIVIDE:
        if (arg2_value == 0.0f) return nanf("");
        result = arg1_value / arg2_value;
        break;
    default: return nanf("");
    }
    return result;
}

static bool can_be_folded(TacOperationCode op_code) {
    return (op_code == OP_ADD || op_code == OP_SUBTRACT ||
        op_code == OP_DIVIDE || op_code == OP_MULTIPLY ||
        op_code == OP_CONCAT || op_code == OP_MULTIPLY_STRING);
}

static bool are_args_constant(TacInstruction *instr) {
    return (instr->arg1->type == OPERAND_TYPE_CONSTANT &&
        instr->arg2->type == OPERAND_TYPE_CONSTANT);
}

/* ======================================*/
/* ===== Implementace pomocných funkcí pro DCE =====*/
/* ======================================*/

static bool op_has_global_effect(TacOperationCode op) {
    switch (op) {
        case OP_CALL: 
        case OP_JUMP:
        case OP_JUMP_IF_FALSE:
        case OP_LABEL:
        case OP_FUNCTION_BEGIN:
        case OP_FUNCTION_END:
        case OP_RETURN:
        case OP_PARAM:
            return true;
        default:
            return false;
    }
}

static bool is_global_var(const char *name) {
    // Předpokládáme, že globální proměnné začínají prefixem "__"
    return strncmp(name, "__", 2) == 0;
}

/* ======================================*/
/* ===== Implementace technik optimalizace =====*/
/* ======================================*/

static void dead_code_elimination(TACDLList *tac_list) {
    // Seznam pro počítání referencí (Temp i Symbol)
    VarUsage *usage_list = NULL;

    // Počítání použití (Counting)
    TACDLL_First(tac_list);
    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        if (instr->operation_code == OP_RETURN && instr->result) {
            // Pokud RETURN vrací hodnotu, zkontrolujeme RESULT
            Operand *ret_op = instr->result;
            if (ret_op->type == OPERAND_TYPE_TEMP) {
                 int id = ret_op->data.temp_id;
                 if (id >= 0 && id < MAX_TEMP_ID) {
                     // Zvyšujeme počet použití pro vrácený TEMP
                     usage_inc(&usage_list, NULL, id, true);
                 }
            } else if (ret_op->type == OPERAND_TYPE_SYMBOL) {
                 // Zvyšujeme počet použití pro vrácený Symbol
                 char *key = ret_op->data.symbol_entry->key;
                 if (!is_global_var(key)) {
                     usage_inc(&usage_list, key, 0, false);
                 }
            }
        }

        // Kontrola ARG1
        if (instr->arg1) {
            if (instr->arg1->type == OPERAND_TYPE_TEMP) {
                int id = instr->arg1->data.temp_id;
                if (id >= 0 && id < MAX_TEMP_ID) {
                    usage_inc(&usage_list, NULL, id, true);
                }
            } 
            else if (instr->arg1->type == OPERAND_TYPE_SYMBOL) {
                char *key = instr->arg1->data.symbol_entry->key;
                // Pokud to není globální proměnná
                if (!is_global_var(key)) {
                    usage_inc(&usage_list, key, 0, false);
                }
            }
        }

        // Kontrola ARG2
        if (instr->arg2) {
            if (instr->arg2->type == OPERAND_TYPE_TEMP) {
                int id = instr->arg2->data.temp_id;
                if (id >= 0 && id < MAX_TEMP_ID) {
                    usage_inc(&usage_list, NULL, id, true);
                }
            }
            else if (instr->arg2->type == OPERAND_TYPE_SYMBOL) {
                char *key = instr->arg2->data.symbol_entry->key;
                if (!is_global_var(key)) {
                    usage_inc(&usage_list, key, 0, false);
                }
            }
        }

        TACDLL_Next(tac_list);
    }

    // Mazání 
    TACDLL_First(tac_list);
    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        bool should_remove = false;

        // Kontrolujeme instrukce, které zapisují do RESULT
        if (instr->result) {
            // Zápis do TEMP
            if (instr->result->type == OPERAND_TYPE_TEMP) {
                int id = instr->result->data.temp_id;
                if (id >= 0 && id < MAX_TEMP_ID) {
                    int count = usage_get(usage_list, NULL, id, true);
                    if (count == 0 && !op_has_global_effect(instr->operation_code)) {
                        should_remove = true;
                    }
                    
                }
            }
            // Zápis do lokálního symbolu
            else if (instr->result->type == OPERAND_TYPE_SYMBOL) {
                char *key = instr->result->data.symbol_entry->key;
                if (!is_global_var(key)) {
                    int count = usage_get(usage_list, key, 0, false);
                    if (count == 0 && !op_has_global_effect(instr->operation_code)) {
                        should_remove = true;
                    }
                }
            }
        }
        // Provádíme odstranění instrukce, pokud je to potřeba
        if (should_remove) {
            if (tac_list->active_element == tac_list->first_element) {
                TACDLL_DeleteFirst(tac_list);
                TACDLL_First(tac_list);
            } else {
                TACDLL_Previous(tac_list);
                TACDLL_DeleteAfter(tac_list);
                TACDLL_Next(tac_list);
            }
            optimization_performed = true;
        } else {
            TACDLL_Next(tac_list);
        }
    }

    usage_list_free(usage_list);
}

static void constant_propagation(TACDLList *tac_list) {
    // Mapa pro pojmenované proměnné (symtable)
    PropagatorMap map;
    prop_map_clear(&map);

    // Pole pro sledování dočasných proměnných (TEMPS)
    TacConstant temp_values[MAX_TEMP_ID];
    bool temp_active[MAX_TEMP_ID];
    
    // Inicializace temp pole (vše neaktivní)
    for(int i = 0; i < MAX_TEMP_ID; i++) temp_active[i] = false;

    TACDLL_First(tac_list);
    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        // Pokud narazíme na kontrolní tok, vyčistíme mapu a temp pole
        if (instr->operation_code == OP_LABEL || 
            instr->operation_code == OP_JUMP || 
            instr->operation_code == OP_JUMP_IF_FALSE || 
            instr->operation_code == OP_FUNCTION_BEGIN ||
            instr->operation_code == OP_CALL) {
            
            prop_map_clear(&map);
            for(int i = 0; i < MAX_TEMP_ID; i++) temp_active[i] = false;

            TACDLL_Next(tac_list);
            continue;
        }

        // Nahrazování (Substitution) v ARG1
        if (instr->arg1) {
            // Pokud je SYMBOL
            if (instr->arg1->type == OPERAND_TYPE_SYMBOL) {
                TacConstant known_val;
                if (prop_map_get(&map, instr->arg1->data.symbol_entry->key, &known_val)) {
                    free_operand(instr->arg1);
                    instr->arg1 = create_constant_operand(known_val);
                    optimization_performed = true;
                }
            }
            // Pokud je TEMP
            else if (instr->arg1->type == OPERAND_TYPE_TEMP) {
                int id = instr->arg1->data.temp_id;
                if (id >= 0 && id < MAX_TEMP_ID && temp_active[id]) {
                    free_operand(instr->arg1);
                    instr->arg1 = create_constant_operand(temp_values[id]);
                    optimization_performed = true;
                }
            }
        }

        // Nahrazování (Substitution) v ARG2
        if (instr->arg2) {
            // Pokud je SYMBOL
            if (instr->arg2->type == OPERAND_TYPE_SYMBOL) {
                TacConstant known_val;
                if (prop_map_get(&map, instr->arg2->data.symbol_entry->key, &known_val)) {
                    free_operand(instr->arg2);
                    instr->arg2 = create_constant_operand(known_val);
                    optimization_performed = true;
                }
            }
            // Pokud je TEMP
            else if (instr->arg2->type == OPERAND_TYPE_TEMP) {
                int id = instr->arg2->data.temp_id;
                if (id >= 0 && id < MAX_TEMP_ID && temp_active[id]) {
                    free_operand(instr->arg2);
                    instr->arg2 = create_constant_operand(temp_values[id]);
                    optimization_performed = true;
                }
            }
        }

        // Ukládání RESULT
        if (instr->result) {
            // Zjištění, zda se jedná o přiřazení konstanty
            bool is_assignment_of_const = (instr->operation_code == OP_ASSIGN && 
                                           instr->arg1 && 
                                           instr->arg1->type == OPERAND_TYPE_CONSTANT);
            // Pokud je SYMBOL
            if (instr->result->type == OPERAND_TYPE_SYMBOL) {
                char *result_key = instr->result->data.symbol_entry->key;
                if (is_assignment_of_const) {
                    prop_map_set(&map, result_key, instr->arg1->data.constant);
                } else {
                    // Výsledek není konstanta, odstraníme z mapy
                    prop_map_remove(&map, result_key);
                }
            }
            // Pokud je TEMP
            else if (instr->result->type == OPERAND_TYPE_TEMP) {
                int id = instr->result->data.temp_id;
                if (id >= 0 && id < MAX_TEMP_ID) {
                    if (is_assignment_of_const) {
                        temp_values[id] = instr->arg1->data.constant;
                        temp_active[id] = true;
                    } else {
                        temp_active[id] = false;
                    }
                }
            }
        }

        TACDLL_Next(tac_list);
    }
}

static void constant_folding(TACDLList *tac_list) {
    TACDLL_First(tac_list);

    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        // Kontrola, zda je typ operace vhodný pro folding (aritmetika, stringy atd.)
        if (!can_be_folded(instr->operation_code)) {
            TACDLL_Next(tac_list);
            continue;
        }
        // Kontrola, zda jsou oba operandy skutečně konstanty (ne proměnné)
        if (!are_args_constant(instr)) {
            TACDLL_Next(tac_list);
            continue;
        }

        bool optimized = false;
        TacConstant result_const;
        memset(&result_const, 0, sizeof(TacConstant));

        // Načtení hodnot konstant z operandů
        TacConstant arg1_const = instr->arg1->data.constant;
        TacConstant arg2_const = instr->arg2->data.constant;

        // Spojování řetězců ("a" + "b")
        if (arg1_const.type == TYPE_STR && arg2_const.type == TYPE_STR && instr->operation_code == OP_CONCAT) {
            char *result_str = concat_string_constants(instr);
            if (result_str != NULL) {
                result_const.type = TYPE_STR;
                result_const.value.str_value = result_str;
                optimized = true;
            }
        }
        // Násobení řetězce číslem ("a" * 3)
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
        // Aritmetické operace s čísly (Int/Float)
        else if ((arg1_const.type == TYPE_NUM || arg1_const.type == TYPE_FLOAT) &&
                 (arg2_const.type == TYPE_NUM || arg2_const.type == TYPE_FLOAT)) {
            float result_value = calculate_num_constant(instr);
            // Kontrola na NaN (např. při dělení nulou se folding neprovádí)
            if (!isnan(result_value)) {
                set_num_constant_value(&result_const, result_value);
                optimized = true;
            }
        }

        // Pokud se podařilo hodnotu vypočítat (složit), upravíme instrukci
        if (optimized) {
            optimization_performed = true;
            
            // Uvolníme paměť starých operandů, už nejsou potřeba
            free_operand(instr->arg1);
            free_operand(instr->arg2);

            // Vytvoříme nový operand obsahující vypočtený výsledek
            Operand *result_op = create_constant_operand(result_const);

            // Změníme instrukci na prosté přiřazení (res = constant)
            instr->operation_code = OP_ASSIGN;
            instr->arg1 = result_op;
            instr->arg2 = NULL; // Druhý operand u přiřazení není
        }

        TACDLL_Next(tac_list);
    }
}

static void unreachable_code(TACDLList *tac_list) {
    TACDLL_First(tac_list);

    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        // Odstranění kódu za skokem nebo návratem
        if (instr->operation_code == OP_JUMP || instr->operation_code == OP_RETURN) {
            
            TACDLL_Next(tac_list);

            while (TACDLL_IsActive(tac_list)) {
                TacInstruction *next_instr;
                TACDLL_GetValue(tac_list, &next_instr);

                // Konec neviditelného kódu na LABEL nebo FUNCTION_END
                if (next_instr->operation_code == OP_LABEL ||
                    next_instr->operation_code == OP_FUNCTION_END) {
                    break;
                }

                TACDLL_Previous(tac_list);
                TACDLL_DeleteAfter(tac_list);
                TACDLL_Next(tac_list);

                optimization_performed = true;
            }
        } else {
            TACDLL_Next(tac_list);
        }
    }
}

/* ======================================*/
/* ===== Implementace veřejných funkcí =====*/
/* ======================================*/


void optimize_tac(TACDLList *tac_list) {
    optimization_performed = true;

    // Nekonečné opakování, dokud se provádějí optimalizace
    while (optimization_performed) {
        // Nastavení příznaku na false před každou iterací
        optimization_performed = false;

        // Volaní constant propagation
        constant_propagation(tac_list);
        // Volaní constant folding
        constant_folding(tac_list);
        // Volaní unreachable code elimination
        unreachable_code(tac_list);
        // Volaní dead code elimination
        dead_code_elimination(tac_list);
    }
}