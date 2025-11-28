// Имплементация функций генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//
#include "codegen.h"
#include "printer.h"
#include <stdlib.h>
#include "error_codes.h"

Symtable gen_global_table;

/* =============================================================== */
/* ===== Deklarace privátních funkcí generátoru cílového kódu ==== */
/* =============================================================== */

/**
 * @brief Vytvoří počáteční kód a definuje globální proměnné.
 * 
 * @param table Symbolová tabulka pro definování globálních proměnných.
 */
static void gen_init(Symtable *table);

/**
 * @brief Generuje instrukci PUSHFRAME pro přenos dočasného rámce do 
 *  zásobníku lokálních rámců.
 */
static void gen_push_frame();

/**
 * @brief Generuje instrukci CREATEFRAME pro vytvoření nového dočasného rámce.
 */
static void gen_create_frame();

/**
 * @brief Generuje instrukci POPFRAME pro přenos vrchního lokálního rámce
 *  do dočasného rámce.
 */
static void gen_pop_frame();

/**
 * @brief Generuje instrukci LABEL s daným názvem štítku.
 * 
 * @param label_name Název štítku.
 */
static void gen_label(char* label_name);

/**
 * @brief Generuje instrukci JUMP na daný štítek.
 * 
 * @param label_name Název cílového štítku.
 */
static void gen_jump(char* label_name);

/**
 * @brief Generuje instrukci CALL na daný štítek.
 * 
 * @param label_name Název cílového štítku.
 */
static void gen_call(char* label_name);

/**
 * @brief Generuje instrukci RETURN pro návrat z funkce.
 * 
 * @param instr Ukazatel na instrukci TAC obsahující návratovou hodnotu.
 */
static void gen_return(TacInstruction *instr);

/**
 * @brief Generuje instrukci JUMPIFNEQ pro podmíněný skok, 
 *  pokud jsou dvě hodnoty rovny.
 * 
 * @param instr Ukazatel na instrukci TAC obsahující podmínky skoku.
 */
static void gen_jumpifneq(TacInstruction *instr);

/**
 * @brief Generuje kód pro zadaný operánd.
 * 
 * @param op Ukazatel na operánd k vygenerování.
 */
static void gen_operand(Operand *op, int arg_num);

/**
 * @brief Generuje prirazení typu hodnot pro zadanou instrukci TAC.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_type(TacInstruction *instr);

/**
 * @brief Generuje dělení pro zadanou instrukci TAC.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_divide(TacInstruction *instr);

/**
 * @brief Generuje kontrolu, zda mají operandy stejný typ,
 *  pro zadanou instrukci TAC.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_same_operand_check(TacInstruction *instr);

/**
 * @brief Generuje konverzi výsledku pro zadanou instrukci TAC.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_convert_result(TacInstruction *instr);

/**
 * @brief Generuje aritmetickou operaci pro zadanou instrukci TAC.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_arithmetic(TacInstruction *instr);

/**
 * @brief Generuje instrukci pro definici proměnné.
 * 
 * @param var Ukazatel na operánd proměnné k definování.
 */
static void gen_defvar(Operand *op);

/**
 * @brief Generuje instrukci pro definici proměnné z řetězcu.
 * 
 * @param frame Rámec proměnné (GF nebo LF).
 * @param var Název proměnné.
 */
static void gen_defvar_str(char *frame, char *var);

/**
 * @brief Generuje instrukci MOVE pro přiřazení hodnoty.
 * 
 * @param dest Cílový operánd pro přiřazení.
 * @param src Zdrojový operánd pro přiřazení.
 */
static void gen_move(Operand *dest, Operand *src);

/**
 * @brief Generuje instrukci MOVE pro přiřazení hodnoty z charů.
 * 
 * @param dest_frame Rámec cílového operandu (GF nebo LF).
 * @param dest Cílový operánd pro přiřazení.
 * @param src_frame Rámec zdrojového operandu (GF nebo LF).
 * @param src Zdrojový operánd pro přiřazení.
 */
static void gen_move_str(char* dest_frame, char *dest, char* src_frame, char *src);


/**
 * @brief Generuje instrukci pro předání parametru funkci.
 * 
 * @param instructions Seznam TAC instrukcí celého programu.
 */
static void gen_param(TACDLList *instructions);

/**
 * @brief Generuje instrukci pro násobení řetězců.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_mul_str(TacInstruction *instr);

/**
 * @brief Generuje instrukci pro porovnání hodnot.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_comprasion(TacInstruction *instr);

/**
 * @brief Generuje instrukci pro porovnání rovnosti hodnot.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_eq_comprasion(TacInstruction *instr);

/**
 * @brief Generuje instrukci pro porovnání nerovnosti hodnot.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_not_equal(TacInstruction *instr);

/**
 * @brief Generuje instrukci pro deklaraci promenne.
 * 
 * @param result Ukazatel na operánd výsledku funkce.
 */
static void gen_declare(Operand *result);

/**
 * @brief Generuje instrukci pro začátek funkce.
 * 
 * @param instructions Seznam TAC instrukcí celého programu.
 */
static void gen_function_begin(TACDLList  *instructions);

/**
 * @brief Generuje instrukci pro konec funkce.
 * 
 * @param instructions Seznam TAC instrukcí celého programu.
 */
static void gen_label_from_instr(TACDLList  *instructions);

/**
 * @brief Generuje instrukci pro operator IS.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_is(TacInstruction *instr);

/**
 * @brief Generuje vestavěné funkce IFJ.
 * 
 * @param instructions Seznam TAC instrukcí celého programu.
 */
static void gen_ifj_fun(TACDLList *instructions);

/**
 * @brief Generuje vestavěnou funkci pro čtení řetězce.
 */
static void gen_read_str();

/**
 * @brief Generuje vestavěnou funkci pro čtení čísla.
 */
static void gen_read_num();

/**
 * @brief Generuje vestavěnou funkci pro zápis hodnoty.
 * 
 * @param instructions Seznam TAC instrukcí celého programu.
 */
static void gen_write(TACDLList *instructions);

/**
 * @brief Generuje vestavěnou funkci pro délku řetězce.
 * 
 * @param instructions Seznam TAC instrukcí celého programu.
 */
static void gen_floor(TACDLList *instructions);

/**
 * @brief Generuje vestavěnou funkci pro zaokrouhlení čísla dolů.
 * 
 * @param instructions Seznam TAC instrukcí celého programu.
 */
static void gen_str(TACDLList *instructions);

/**
 * @brief Získá prefix rámce pro zadaný operánd.
 * 
 * @param op Ukazatel na operánd.
 * @return char* Prefix rámce (GF@ nebo LF@).
 */
char *get_frame(Operand *op);

static int getter_check(TacInstruction *instr);

/* =================================================================== */
/* ===== Implementace privátních funkcí generátoru cílového kódu ===== */
/* =================================================================== */

char *get_frame(Operand *op){
    // Hledáme na 0 úrovni symtable proměnné
    if (symtable_lookup(&gen_global_table, op->data.symbol_entry->key) != NULL) 
        return "GF";
    // Pokud není, to je Local Frame
    return "LF";
}


static void gen_init(Symtable *table){
    // Hlavicka IFJcode25
    fprintf(stdout, ".IFJcode25\n");

    // Definice pomocných globálních proměnných
    gen_defvar_str("GF","$tmp");
    gen_defvar_str("GF","$tmp_2");
    gen_defvar_str("GF","$tmp_type_1");
    gen_defvar_str("GF","$tmp_type_2");
    gen_defvar_str("GF","$tmp_op_1");
    gen_defvar_str("GF","$tmp_op_2");
    gen_defvar_str("GF","$ret_ifj_fun");
    gen_defvar_str("GF","$getter_1");
    gen_defvar_str("GF","$getter_2");
    

    // Definice globálních proměnných z tabulky symbolů
    for (size_t i = 0; i < table->capacity; i++) {
        // ? Прямое обращение к entries это нормально?
        TableEntry* entry = &table->entries[i];     
        if (entry->status == SLOT_OCCUPIED) {
            if (entry->data->kind == KIND_VAR) {
                gen_defvar_str("GF", entry->data->unique_name);
                gen_move_str("GF", entry->data->unique_name, "nil", "nil");
            }
        }
    }

    // Volání hlavní funkce
    gen_create_frame();
    gen_call("$main");

    // Ukončení programu s návratovým kódem 0
    fprintf(stdout, "EXIT int@0\n");
    // Ukončení programu s běhovými chybami
    gen_label("$EXIT25");
    fprintf(stdout, "EXIT int@25\n");
    gen_label("$EXIT26");
    fprintf(stdout, "EXIT int@26\n");
}

static void gen_push_frame(){
    fprintf(stdout, "PUSHFRAME\n");
}

static void gen_create_frame(){
    fprintf(stdout, "CREATEFRAME\n");
}

static void gen_pop_frame(){
    fprintf(stdout, "POPFRAME\n");
}

static void gen_label(char* label_name) { 
    fprintf(stdout, "\nLABEL $%s\n", label_name);
}

static void gen_jump(char* label_name){
    fprintf(stdout, "JUMP $%s\n", label_name);
}

static void gen_call(char* label_name){
    fprintf(stdout, "CALL $%s\n", label_name);
}

static void gen_return(TacInstruction *instr){
    // Pokud je návratová hodnota, vložíme ji do LF@ret
    if (instr->result != NULL) {
        fprintf(stdout, "MOVE LF@ret ");
        gen_operand(instr->result, 0);
        fprintf(stdout, "\n");
    }

    // Provádíme pop frame pro návrat z funkce
    gen_pop_frame();
    fprintf(stdout, "RETURN\n");
}

static void gen_jumpifeq_str(char *label_name, char *frame_op_1,
                           char *op_1, char *frame_op_2, char *op_2){
    fprintf(stdout, "JUMPIFEQ $%s %s@%s %s@%s\n", label_name, frame_op_1, op_1, frame_op_2,  op_2);
}

static void gen_jumpifneq_str(char *label_name, char *frame_op_1,
                           char *op_1, char *frame_op_2, char *op_2){
    fprintf(stdout, "JUMPIFNEQ $%s %s@%s %s@%s\n", label_name, frame_op_1, op_1, frame_op_2,  op_2);
}


static void gen_jumpifneq(TacInstruction *instr){
    // JUMPIFEQ padá na typové neshodě, takže nemůžeme rovnou porovnat s bool@false,
    // pokud nevíme, že operand je typu bool.

    // Pokud je hodnota nil, skáčeme (nil je nepravda)
    fprintf(stdout, "JUMPIFEQ $%s ", instr->arg2->data.label_name);
    gen_operand(instr->arg1, 1);
    fprintf(stdout, " nil@nil\n");

    // Zjistíme typ operandu
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Vytvoříme unikátní návěští pro přeskočení
    char *label_skip = create_unique_label("SKIP_BOOL_CHECK");

    // Pokud typ NENÍ bool, tak to nemůže být 'false'. 
    // A protože už víme, že to není 'nil' (krok 1), musí to být "pravda" (int, string, ...).
    // Takže neskáčeme na ELSE, ale pokračujeme dál  přeskočíme porovnání s false.
    fprintf(stdout, "JUMPIFNEQ $%s GF@$tmp_type_1 string@bool\n", label_skip);

    // Můžeme bezpečně porovnat s false.
    fprintf(stdout, "JUMPIFEQ $%s ", instr->arg2->data.label_name);
    gen_operand(instr->arg1, 1);
    fprintf(stdout, " bool@false\n");

    // Návěští pro přeskočení
    gen_label(label_skip);
    
    free(label_skip);
}

static void gen_operand(Operand *op, int arg_num){
    // Generování proměnné
    if (op->type == OPERAND_TYPE_SYMBOL) {
        char *prefix = get_frame(op);
        fprintf(stdout, "%s@%s", prefix, op->data.symbol_entry->data->unique_name);

    // Generování konstanty
    } else if (op->type == OPERAND_TYPE_CONSTANT) {
        switch (op->data.constant.type)
        {
        case TYPE_NUM:
            fprintf(stdout, "int@%i", op->data.constant.value.int_value);
            break;
        case TYPE_FLOAT:
            fprintf(stdout, "float@%a", op->data.constant.value.float_value);
            break;
        case TYPE_STR:
        {
            char *escaped_str = string_to_ifjcode(op->data.constant.value.str_value);
            // char *escaped_str = op->data.constant.value.str_value;
            fprintf(stdout, "string@%s", escaped_str);
            free(escaped_str);
            break;
        }
        case TYPE_NIL:
            fprintf(stdout, "nil@nil");
            break;
        default:
            break;
        }
        return;

    // generování dočasné proměnné
    } else if (op->type == OPERAND_TYPE_TEMP) {
        fprintf(stdout, "LF@$t$%i", op->data.temp_id);
    } else if (op->type == OPERAND_TYPE_LABEL) {
        fprintf(stdout, "GF@$setter_%i", arg_num);
    }

}

static void gen_type(TacInstruction *instr) {
    // Zapíšeme do pomocných globálních promenných typy argumentů 
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_2 ");
    gen_operand(instr->arg2, 2);
    fprintf(stdout, "\n");
}

static void gen_divide(TacInstruction *instr){
    char *label_div = create_unique_label("DIV");
    char *label_idiv = create_unique_label("IDIV");
    char *label_end = create_unique_label("END_DIV");
    
    // Kontrola typů a příprava operandů
    gen_type(instr);
    gen_same_operand_check(instr);


    // Typ operandů po přípravě
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    // Přechod do dělení na float nebo int
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@float\n", label_div);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@int\n", label_idiv);
    // pokud není float ani int, tak chyba
    gen_jump("$EXIT26");

    // dělení float
    gen_label(label_div);
    fprintf(stdout, "DIV ");
    gen_operand(instr->result, 0);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");
    fprintf(stdout, "JUMP $%s\n", label_end);

    // dělení int
    gen_label(label_idiv);
    fprintf(stdout, "IDIV ");
    gen_operand(instr->result, 0);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");

    // po dělení typu float konverze výsledku
    gen_convert_result(instr);
    
    // konec dělení
    gen_label(label_end);
    
    free(label_div);
    free(label_idiv);
    free(label_end);
}

static void gen_same_operand_check(TacInstruction *instr){
    
    // Kontrola na operandy nil a bool
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_1", "string", "nil");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "nil");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_1", "string", "bool");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "bool");

    
    // Uložení operandů do dočasných proměnných
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg2, 1);
    fprintf(stdout, "\n");
    

    // pro operaci CONCAT speciální kontrola
    if (instr->operation_code == OP_CONCAT) {
        gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_1", "string", "int");
        gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "int");
        gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_1", "string", "float");
        gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "float");
        return;
    }

    // Vytvoření štítků
    char *label_start_operation = create_unique_label("START_OPERATION");
    char *label_arg_2_float= create_unique_label("ARG2_TO_FLOAT");

    // Kontrola na shodné typy
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_1", "string", "string");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "string");

    // Pokud jsou oba int nebo float, pokračuj
    gen_jumpifeq_str(label_start_operation, "GF", "$tmp_type_1", "GF", "$tmp_type_2");

    // Convertace int na float, pokud je jeden z operandů float
    gen_jumpifeq_str(label_arg_2_float, "GF", "$tmp_type_1", "string", "float");

    // Konverze prvního operandu
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_1 GF@$tmp_op_1\n");
    gen_jump(label_start_operation);

    // Konverze druhého operandu
    gen_label(label_arg_2_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_2 GF@$tmp_op_2\n");

    // Začátek operace
    gen_label(label_start_operation);

    free(label_start_operation);
    free(label_arg_2_float);
}

static void gen_convert_result(TacInstruction *instr){
    char *label_end_convert = create_unique_label("END_CONVERT");

    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->result, 0);
    fprintf(stdout, "\n");

    gen_jumpifeq_str(label_end_convert, "GF", "$tmp_type_1", "string", "int");

    // Kontrola, zda je výsledek float
    fprintf(stdout, "ISINT GF@$tmp ");
    gen_operand(instr->result, 0);
    fprintf(stdout, "\n");
    

    gen_jumpifeq_str(label_end_convert, "GF", "$tmp", "bool", "false");

    // Konverze float na int
    fprintf(stdout, "FLOAT2INT ");
    gen_operand(instr->result, 0);
    fprintf(stdout, " ");
    gen_operand(instr->result, 0);
    fprintf(stdout, "\n");

    // Štítek pro konec Konverze
    gen_label(label_end_convert);

    free(label_end_convert);
}

static void gen_arithmetic(TacInstruction *instr){
    // Kontrola typů a příprava operandů
    gen_type(instr);
    gen_same_operand_check(instr);

    // Generování aritmetické operace
    switch (instr->operation_code) {
        case OP_ADD:
            fprintf(stdout, "ADD ");
            break;
        case OP_SUBTRACT:
            fprintf(stdout, "SUB ");
            break;
        case OP_MULTIPLY:
            fprintf(stdout, "MUL ");
            break;
        case OP_CONCAT:
            fprintf(stdout, "CONCAT ");
            break;
        default:
            break;
    }
    gen_operand(instr->result, 0);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");

    // Konverze výsledku zpět na int, pokud je to potřeba
    if (instr->operation_code != OP_CONCAT) {
        gen_convert_result(instr);
    }
}

static void gen_defvar(Operand *var){
    // Výpis declarace promenné
    fprintf(stdout, "DEFVAR ");
    gen_operand(var, 0);
    fprintf(stdout, "\n");
}

static void gen_defvar_str(char *frame, char *var){
    // Výpis declarace promenné pomoci charů
    fprintf(stdout, "DEFVAR %s@%s\n", frame, var);
}

static void gen_move_str(char* dest_frame, char *dest, char* src_frame, char *src){
    // Výpis přiřazení pomoci charů
    fprintf(stdout, "MOVE %s@%s %s@%s\n", dest_frame, dest, src_frame, src);
}

static void gen_move(Operand *dest, Operand *src){
    // Výpis přiřazení

    fprintf(stdout, "MOVE ");
    gen_operand(dest, 0);
    fprintf(stdout, " ");

    // Kontrola na přířazení navratové hodnoty funkce 
    if (src->type == OPERAND_TYPE_LABEL) {
        // Pro vestavěné funkce
        if (strstr(src->data.label_name, "Ifj.") != NULL)
            fprintf(stdout, "GF@$ret_ifj_fun");
        // Pro programovatelné funkce
        else
            fprintf(stdout, "TF@ret");

    // Pokud není funkce přířazíme druhý argument
    } else
        gen_operand(src, 1);

    fprintf(stdout, "\n");
}

static void gen_param(TACDLList *instructions){
    // Vytvoříme nový rámec pro lokální proměnné
    gen_create_frame();


    // Definice návratové proměnné
    gen_defvar_str("TF", "ret");
    gen_move_str("TF", "ret", "nil", "nil");

    TacInstruction *instr;  // Actualní volající instrukce
    TACDLL_GetValue(instructions, &instr);


    char *label_name; // Jmeno funkce
    if (instr->operation_code == OP_ASSIGN)
        label_name = instr->result->data.label_name;
    else
        label_name = instr->arg1->data.label_name;

    // Kontrola přítomnosti parametrů funkce 
    TACDLL_Next(instructions);
    
    if (TACDLL_IsActive(instructions)){
        TACDLL_GetValue(instructions, &instr);
        TACDLL_Previous(instructions);
        if (instr->operation_code != OP_PARAM)
            return;
    }



    TACDLList instrs_fun = *instructions;   // Copie seznamu instrukcí
    TacInstruction *instr_param; // Actualní instrukce funkce
    
    // Od začatku hledáme žačatek funkce
    TACDLL_First(&instrs_fun);
    while (TACDLL_IsActive(&instrs_fun)) {
        TACDLL_GetValue(&instrs_fun, &instr_param);
        TACDLL_Next(&instrs_fun);
        if (instr_param->operation_code == OP_LABEL &&
            strcmp(instr_param->arg1->data.label_name, label_name) == 0)
            break;
    }
    TACDLL_Next(instructions);  // Přeskočíme volání funkce
    TACDLL_Next(&instrs_fun);    

    // Teď kdy máme active_element na parametrech a argumentech
    // Přirazíme paremetrům hodnoty argumentů  
    while (TACDLL_IsActive(instructions)) {
        TACDLL_GetValue(instructions, &instr);
        TACDLL_GetValue(&instrs_fun, &instr_param);

        // Pokud parametry se skončily vracíme o jednu instrukci zpět
        // a skončíme generace parametrů
        if (instr->operation_code != OP_PARAM){
            TACDLL_Previous(instructions);
            break;
        }

        // Declarace a přiřazení hodnot
        char *param_name = instr_param->result->data.symbol_entry->data->unique_name;
        gen_defvar_str("TF", param_name);
        fprintf(stdout, "MOVE TF@%s ", param_name);
        gen_operand(instr->arg1, 1);
        fprintf(stdout, "\n");

        TACDLL_Next(instructions);
        TACDLL_Next(&instrs_fun);
    }
}

static void gen_mul_str(TacInstruction *instr){
    char *label_loop = create_unique_label("MUL_STR_LOOP");
    char *label_loop_end = create_unique_label("MUL_STR_LOOP_END");
    char *label_end_convert = create_unique_label("END_CONVERT");
    
    // Kontrola typů operandů
    gen_type(instr);
    
    gen_jumpifneq_str("$EXIT26", "GF", "$tmp_type_1", "string", "string");

    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "string");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "nil");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "bool");

    
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg2, 1);
    fprintf(stdout, "\n");

    // kontrola že druhý operand je int
    fprintf(stdout, "ISINT GF@$tmp GF@$tmp_op_2\n");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp", "bool", "false");
    
    // Konverze na int pokud je float
    gen_jumpifeq_str(label_end_convert, "GF", "$tmp_type_2", "string", "int");
    fprintf(stdout, "FLOAT2INT GF@$tmp_op_2 GF@$tmp_op_2\n");

    // Konec konverze
    gen_label(label_end_convert);

    // Kontrola na nenegativnost
    fprintf(stdout, "LT GF@$tmp_2 GF@$tmp_op_2 int@0\n");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_2", "bool", "true");

    // Inicializace výsledné string proměnné a počítadla
    fprintf(stdout, "MOVE ");
    gen_operand(instr->result, 0);
    fprintf(stdout, " string@\n");

    // Začátek smyčky
    gen_label(label_loop);
    
    // Kontrola počítadla
    gen_jumpifeq_str(label_loop_end, "GF", "$tmp_op_2", "int", "0");
    
    // Konkatenace řetězce
    fprintf(stdout, "CONCAT ");
    gen_operand(instr->result, 0);
    fprintf(stdout, " ");
    gen_operand(instr->result, 0);
    fprintf(stdout, " ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // DEC počítadla
    fprintf(stdout, "SUB GF@$tmp_op_2 GF@$tmp_op_2 int@1\n");
    // Pokračování smyčky
    fprintf(stdout, "JUMP $%s\n", label_loop);

    // Konec smyčky
    gen_label(label_loop_end);
    free(label_loop);
    free(label_loop_end);
    free(label_end_convert);
}

static void gen_comprasion(TacInstruction *instr){
    char *label_start_operation = create_unique_label("START_OPERATION");
    char *label_arg_2_float = create_unique_label("ARG2_TO_FLOAT");
    char *label_end = create_unique_label("END_COMPARASION");

    // Kontrola typů a příprava operandů
    gen_type(instr);
    
    
   
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg2, 1);
    fprintf(stdout, "\n");

    // Pokud jsou oba operandy stejného typu, pokračuj
    gen_jumpifeq_str(label_start_operation, "GF", "$tmp_type_1", "GF", "$tmp_type_2");

    fprintf(stdout, "MOVE ");
    gen_operand(instr->result, 0);
    fprintf(stdout, " bool@false\n");
    gen_jump(label_end);

    // Pokud operandy nejsou stejného typu, null pro string a bool
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_1", "string", "string");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "string");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_1", "string", "bool");
    gen_jumpifeq_str("$EXIT26", "GF", "$tmp_type_2", "string", "bool");

    // Konverze int na float, pokud je jeden z operandů float
    gen_jumpifeq_str(label_arg_2_float, "GF", "$tmp_type_1", "string", "int");

    // Konverze prvního operandu
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");
    gen_jump(label_start_operation);

    // Konverze druhého operandu
    gen_label(label_arg_2_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_2 ");
    gen_operand(instr->arg2, 2);
    fprintf(stdout, "\n");

    // Začátek operace  
    gen_label(label_start_operation);

    // Generování instrucke relační operace
    switch (instr->operation_code) {
        case OP_LESS:
        case OP_LESS_EQUAL:
            fprintf(stdout, "LT ");
            break;
        case OP_GREATER:
        case OP_GREATER_EQUAL:
            fprintf(stdout, "GT ");
            break;
        case OP_EQUAL:
        case OP_NOT_EQUAL:
            fprintf(stdout, "EQ ");
            break;
        default:
            break;
    }
    gen_operand(instr->result, 0);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");

    // Konec porovnání
    gen_label(label_end);

    free(label_end);
    free(label_start_operation);
    free(label_arg_2_float);
}

static void gen_eq_comprasion(TacInstruction *instr){
    // Kontrola pro LT GT
    gen_comprasion(instr);

    // Kontrola pro EQUAL
    fprintf(stdout, "EQ GF@$tmp GF@$tmp_op_1 GF@$tmp_op_2\n");

    // OR výsledku s předchozím
    fprintf(stdout, "OR ");
    gen_operand(instr->result, 0);
    fprintf(stdout, " GF@$tmp ");
    gen_operand(instr->result, 0);
    fprintf(stdout, "\n");
}

static void gen_not_equal(TacInstruction *instr){
    // Kontrola pro EQUAL
    gen_comprasion(instr);

    // NOT výsledku
    fprintf(stdout, "NOT ");
    gen_operand(instr->result, 0);
    fprintf(stdout, " ");
    gen_operand(instr->result, 0);
    fprintf(stdout, "\n");
}

static void gen_declare(Operand *result){
    // Declarace proměnné v IFJ25 znamená pouze inicializaci na nil v IFJcode25
    // protoze proměnné jsou declarovány na zacatku vygenerovaného funkce
    fprintf(stdout, "MOVE ");
    gen_operand(result, 0);
    fprintf(stdout, " nil@nil\n");
}

static void gen_function_begin(TACDLList  *instructions){
    TacInstruction *instr;      // aktuální instrukce
    TACDLL_GetValue(instructions, &instr);
    
    // Generování labelu funkce
    if (strcmp(instr->arg1->data.label_name, "main$0") == 0) {
        gen_label("$main");     
    } else
        gen_label(instr->arg1->data.label_name);

    // Vytvoření rámce
    gen_push_frame();   
    
    // Definice lokálních proměnných
    TACDLL_Next(instructions);
    while (TACDLL_IsActive(instructions)){
        TACDLL_GetValue(instructions, &instr);
        TacOperationCode op_code = instr->operation_code;

        // Kontrola na konec funkce
        if (op_code == OP_FUNCTION_END) {
            break;

        // Definice lokální proměnné
        } else if (op_code == OP_DECLARE) {
            // ? Деклариется ли глобальная переменная?
            // Kontrola neglobální proměnné
            if (instr->result != NULL) {
                if (instr->result->type == OPERAND_TYPE_SYMBOL) {
                    // Hledáme na 0 úrovni symtable proměnné
                    if (symtable_lookup(&gen_global_table, 
                        instr->result->data.symbol_entry->key) == NULL) {
                        gen_defvar(instr->result);
                    }
                } else
                    gen_defvar(instr->result);
            }
        }
        TACDLL_Next(instructions);
    }
}

static void gen_label_from_instr(TACDLList  *instructions){
    TacInstruction *instr;  // aktuální instrukce
    TACDLL_GetValue(instructions, &instr);

    // Zkopírujeme seznam instrukcí pro pohyb bez ovlivnění původního
    TACDLList instructions_copy = *instructions;
    TACDLL_Next(&instructions_copy);
    
    // Kontrola, zda je další instrukce
    if (TACDLL_IsActive(&instructions_copy)) {
        TACDLL_GetValue(&instructions_copy, &instr);
    } 

    // ! потом выбрать что-то одно из двух
    // Kontrola, zda je další instrukce začátek funkce
    if (instr->operation_code == OP_FUNCTION_BEGIN || instr->operation_code == OP_PARAM) {
        TACDLL_Previous(&instructions_copy);\
        gen_function_begin(&instructions_copy);
    // pokud ne, generujeme běžný label
    } else {
        TACDLL_GetValue(instructions, &instr);
        gen_label(instr->arg1->data.label_name);
    }
}

static void gen_is(TacInstruction *instr){
    char *type = instr->arg2->data.constant.value.str_value;
    
    // Získání typu prvního operandu
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Porovnání typu s požadovaným typem
    fprintf(stdout, "EQ ");
    // Vysledek uložíme do výsledné proměnné
    gen_operand(instr->result, 0);
    // První operand je typ prvního operandu
    fprintf(stdout, " GF@$tmp_type_1 ");

    // Podle požadovaného typu generujeme porovnání
    if (strcmp(type, "String") == 0)
        fprintf(stdout, "string@string\n");

    else if (strcmp(type, "Null") == 0)
        fprintf(stdout, "string@nil\n");

    else if (strcmp(type, "Bool") == 0)
        fprintf(stdout, "string@bool\n");
        
    else if (strcmp(type, "Num") == 0){
        // Pro Num musíme porovnat na int nebo float
        fprintf(stdout, "string@int\n");
        fprintf(stdout, "EQ GF@$tmp GF@$tmp_type_1 string@float\n");
        fprintf(stdout, "OR ");
        gen_operand(instr->result, 0);
        fprintf(stdout, " GF@$tmp ");
        gen_operand(instr->result, 0);
        fprintf(stdout, "\n");
    } 

}

static void gen_read_str(){
    fprintf(stdout, "READ GF@$ret_ifj_fun string\n");
}

static void gen_read_num(){
    char *label_end = create_unique_label("READ_NUM_END");
    // Čtění floatu
    fprintf(stdout, "READ GF@$ret_ifj_fun float\n");

    // Pokud null, vrátíme nil
    gen_jumpifeq_str(label_end, "GF", "$ret_ifj_fun", "nil", "nil");

    // Pokud celé číslo convertujeme do int
    fprintf(stdout, "ISINT GF@$tmp GF@$ret_ifj_fun\n");
    gen_jumpifeq_str(label_end, "GF", "$tmp", "bool", "false");
    fprintf(stdout, "FLOAT2INT GF@$ret_ifj_fun GF@$ret_ifj_fun\n");

    gen_label(label_end);
    free (label_end);
}

static void gen_write(TACDLList *instructions){
    TacInstruction *instr; // Aktuální instrukce
    char *label_write = create_unique_label("WRITE");

    // Jdeme na argument
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);

    // Přířazujeme argument do pomocné proměnné
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Kontrola typu float
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    gen_jumpifneq_str(label_write, "GF", "$tmp_type_1", "string", "float");
    // Pokud je float, convertujeme na int pokud je to celé číslo
    fprintf(stdout, "ISINT GF@$tmp GF@$tmp_op_1\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp bool@false\n", label_write);
    fprintf(stdout, "FLOAT2INT GF@$tmp_op_1 GF@$tmp_op_1\n");
    
    // Vypis hodnoty
    gen_label(label_write);
    fprintf(stdout, "WRITE GF@$tmp_op_1\n");
    free(label_write);
}

static void gen_floor(TACDLList *instructions){
    TacInstruction *instr; // Aktuální instrukce
    char *label_end = create_unique_label("FLOOR_END");

    // Jdeme na argument
    TACDLL_Next(instructions); 
    TACDLL_GetValue(instructions, &instr);
    
    // Přiřazení argumentu do návratové hodnoty
    fprintf(stdout, "MOVE GF@$ret_ifj_fun ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Kontrola typu
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$ret_ifj_fun\n");
    gen_jumpifeq_str(label_end, "GF", "$tmp_type_1", "string", "int");
    gen_jumpifneq_str("$EXIT25", "GF", "$tmp_type_1", "string", "float");

    // Provádíme floor
    fprintf(stdout, "FLOAT2INT GF@$ret_ifj_fun GF@$ret_ifj_fun\n");

    gen_label(label_end);
    free(label_end);
}

static void gen_str(TACDLList *instructions) {
    TacInstruction *instr;  // Aktuální instrukce
    char *label_float2str = create_unique_label("FLOAT2STR");
    char *label_convert = create_unique_label("CONVERT");
    char *label_int2str = create_unique_label("INT2STR");
    char *label_nil2str = create_unique_label("NIL2STR");
    char *label_bool2str = create_unique_label("BOOL2STR");
    char *label_true2str = create_unique_label("TRUE2STR");
    char *label_end = create_unique_label("STR_END");

    // Jdeme na argument
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    // Přiřazení argumentu do návratové hodnoty
    fprintf(stdout, "MOVE GF@$ret_ifj_fun ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Switch podle typu
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$ret_ifj_fun\n");
    gen_jumpifeq_str(label_int2str, "GF", "$tmp_type_1", "string", "int");
    gen_jumpifeq_str(label_float2str, "GF", "$tmp_type_1", "string", "float");
    gen_jumpifeq_str(label_nil2str, "GF", "$tmp_type_1", "string", "nil");
    gen_jumpifeq_str(label_bool2str, "GF", "$tmp_type_1", "string", "bool");
    // Pokud není žádný z typů, to je str a vratíme jak je
    gen_jump(label_end);    
    
    // Konverze float na string
    gen_label(label_float2str);
    // Kontrola zda je float celé číslo
    fprintf(stdout, "ISINT GF@$tmp GF@$ret_ifj_fun\n");
    gen_jumpifeq_str(label_convert, "GF", "$tmp", "bool", "true");

    // Pokud není celé číslo, převedeme přímo na string
    fprintf(stdout, "FLOAT2STR GF@$ret_ifj_fun GF@$ret_ifj_fun\n");
    gen_jump(label_end);

    gen_label(label_convert);
    fprintf(stdout, "FLOAT2INT GF@$ret_ifj_fun GF@$ret_ifj_fun\n");

    // Konverze int na string
    gen_label(label_int2str);
    fprintf(stdout, "INT2STR GF@$ret_ifj_fun GF@$ret_ifj_fun\n");
    gen_jump(label_end);

    // Konverze nil na string
    gen_label(label_nil2str);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun string@null\n");
    gen_jump(label_end);

    // Konverze bool na string
    gen_label(label_bool2str);
    // Kontrola na true nebo false
    gen_jumpifeq_str(label_true2str, "GF", "$ret_ifj_fun", "bool", "true");
    // Konverze false na string
    gen_move_str("GF", "$ret_ifj_fun", "string", "false");
    gen_jump(label_end);
    // Konverze true na string
    gen_label(label_true2str);
    gen_move_str("GF", "$ret_ifj_fun", "string", "true");
    
    gen_label(label_end);

    free(label_float2str);
    free(label_int2str);
    free(label_nil2str);
    free(label_bool2str);
    free(label_convert);
    free(label_true2str);
    free(label_end);
}

static void gen_length(TACDLList *instructions){
    TacInstruction *instr;  // Aktuální instrukce

    // Jdeme na argument
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);

    fprintf(stdout, "MOVE GF@$ret_ifj_fun ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Kontrola na string
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$ret_ifj_fun\n");
    gen_jumpifneq_str("$EXIT25", "GF", "$tmp_type_1", "string", "string");

    // Získání délky stringu
    fprintf(stdout, "STRLEN GF@$ret_ifj_fun GF@$ret_ifj_fun\n");
}

static void gen_substring(TACDLList *instructions) {
    TacInstruction *instr; // Aktuální instrukce
    char *label_check_23_arg = create_unique_label("CHECK_ARG_23");
    char *label_zero_arg = create_unique_label("ZERO_CHECK_ARG");
    char *label_end = create_unique_label("SUBSTRING_END");
    char *label_while = create_unique_label("WHILE_START");
    char *label_check_with_len = create_unique_label("CHECK_WITH_LEN");


    // Získaní a kontrola 1. argument
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);

    
    fprintf(stdout, "MOVE GF@$tmp ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");
    
    // Kontrola typu
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp\n");
    gen_jumpifneq_str("$EXIT25", "GF", "$tmp_type_1", "string", "string");

    // Získaní delky stringu
    fprintf(stdout, "STRLEN GF@$tmp_2 GF@$tmp\n");


    // Získaní a kontrola 2. a 3. argumentu
    gen_move_str("GF", "$tmp_type_2", "int", "0");
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    gen_label(label_check_23_arg);
    fprintf(stdout, "ADD GF@$tmp_type_2 GF@$tmp_type_2 int@1\n");
    
    // Kontrola typu
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_2\n");
    gen_jumpifeq_str(label_zero_arg, "GF", "$tmp_type_1", "string", "int");
    gen_jumpifneq_str("$EXIT25", "GF", "$tmp_type_1", "string", "float");

    // Konverze float na int
    fprintf(stdout, "ISINT GF@$tmp_type_1 GF@$tmp_op_2\n");
    gen_jumpifeq_str("$EXIT25", "GF", "$tmp_type_1", "bool", "false");
    fprintf(stdout, "FLOAT2INT GF@$tmp_op_2 GF@$tmp_op_2\n");

    // kontrola i < 0 j < 0
    gen_label(label_zero_arg);
    fprintf(stdout, "LT GF@$tmp_type_1 GF@$tmp_op_2 int@0\n");
    gen_jumpifeq_str(label_end, "GF", "$tmp_type_1", "bool", "true");
    
    // Pokud oba argumenty spacovaný - jump pro kontrolu i > j
    gen_jumpifneq_str(label_check_with_len, "GF", "$tmp_type_2", "int", "1");

    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    gen_move_str("GF", "$tmp_op_1", "GF" ,"$tmp_op_2");
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Jump na kontrolu 2. argumentu
    gen_jump(label_check_23_arg);


    // i > j
    gen_label(label_check_with_len);
    fprintf(stdout, "GT GF@$tmp_type_1 GF@$tmp_op_1 GF@$tmp_op_2\n");
    gen_jumpifeq_str(label_end, "GF", "$tmp_type_1", "bool", "true");


    // 𝑖 ≥ Ifj.length(𝑠)
    fprintf(stdout, "LT GF@$tmp_type_1 GF@$tmp_op_1 GF@$tmp_2\n");
    gen_jumpifeq_str(label_end, "GF", "$tmp_type_1", "bool", "false");
    // 𝑗 > Ifj.length(𝑠)
    fprintf(stdout, "GT GF@$tmp_type_1 GF@$tmp_op_2 GF@$tmp_2\n");
    gen_jumpifeq_str(label_end, "GF", "$tmp_type_1", "bool", "true");

    
    // Generace vysledku
    fprintf(stdout, "MOVE GF@$ret_ifj_fun string@\n");

    gen_label(label_while);
    // Kontrola konce podřetezce
    gen_jumpifeq_str(label_end, "GF", "$tmp_op_1", "GF", "$tmp_op_2");

    // Získaní a konkatinace char
    fprintf(stdout, "GETCHAR GF@$tmp_2 GF@$tmp GF@$tmp_op_1\n");
    fprintf(stdout, "CONCAT GF@$ret_ifj_fun GF@$ret_ifj_fun GF@$tmp_2\n");
    // Inc počitadla
    fprintf(stdout, "ADD GF@$tmp_op_1 GF@$tmp_op_1 int@1\n");
    gen_jump(label_while);

    gen_label(label_end);

    free(label_zero_arg);
    free(label_check_23_arg);
    free(label_while);
    free(label_end);
    free(label_check_with_len);
}

static void gen_strcmp(TACDLList *instructions){
    char *label_min_len = create_unique_label("STRCMP_MIN_LEN");
    char *label_while = create_unique_label("STRCMP_WHILE");
    char *label_end = create_unique_label("STRCMP_END");
    char *label_gt_lt = create_unique_label("STRCMP_GT_LT");
    char *label_eq_len = create_unique_label("EQUAL_LEN");
    TacInstruction *instr; // Aktualní instrukce

    // Získaní 1. argumentu
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Získaní 2. argumentu
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Kontrola typů
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    gen_jumpifneq_str("$EXIT25", "GF", "$tmp_type_1", "string", "string");
    fprintf(stdout, "TYPE GF@$tmp_type_2 GF@$tmp_op_2\n");
    gen_jumpifneq_str("$EXIT25", "GF", "$tmp_type_2", "string", "string");

    // Pro pohodlnost na žačatku nastavíme return na 1
    gen_move_str("GF", "$ret_ifj_fun", "int", "1");

    // Ve tmp je minimalní delka, ve tmp_2 je počitadlo
    // Minimalní delka
    fprintf(stdout, "STRLEN GF@$tmp GF@$tmp_op_1\n");
    fprintf(stdout, "STRLEN GF@$tmp_2 GF@$tmp_op_2\n");
    gen_jumpifeq_str(label_eq_len, "GF", "$tmp", "GF", "$tmp_2");
    fprintf(stdout, "LT GF@$tmp_type_1 GF@$tmp_2 GF@$tmp\n");
    gen_jumpifeq_str(label_min_len, "GF", "$tmp_type_1", "bool", "true");

    // Ve pokud 1. str delší je minimalní delka a navratova hodnota bude -1
    gen_move_str("GF", "$tmp_2", "GF", "$tmp");
    gen_move_str("GF", "$ret_ifj_fun", "int", "-1");
    gen_jump(label_min_len);
    
    // Pokud stejná delka navratova hodnota bude 0
    gen_label(label_eq_len);
    gen_move_str("GF", "$ret_ifj_fun", "int", "0");
    gen_jump(label_min_len);

    // Nastavení počitadla
    gen_label(label_min_len);
    gen_move_str("GF", "$tmp", "int", "0");
    

    // strcmp
    gen_label(label_while);
    // Kontrala konce nejmenšího řetězce
    gen_jumpifeq_str(label_end, "GF", "$tmp", "GF", "$tmp_2");
    // Porovnaní charů
    fprintf(stdout, "GETCHAR GF@$tmp_type_1 GF@$tmp_op_1 GF@$tmp\n");
    fprintf(stdout, "GETCHAR GF@$tmp_type_2 GF@$tmp_op_2 GF@$tmp\n");
    gen_jumpifneq_str(label_gt_lt, "GF", "$tmp_type_1", "GF", "$tmp_type_2");
    // Inc počitadla
    fprintf(stdout, "ADD GF@$tmp GF@$tmp int@1\n");
    gen_jump(label_while);


    gen_label(label_gt_lt);
    gen_move_str("GF", "$ret_ifj_fun", "int", "-1");
    fprintf(stdout, "LT GF@$tmp GF@$tmp_type_1 GF@$tmp_type_2\n"); 
    gen_jumpifeq_str(label_end, "GF", "$tmp", "bool", "true");
    gen_move_str("GF", "$ret_ifj_fun", "int", "1");

    
    gen_label(label_end);

    free(label_gt_lt);
    free(label_end);
    free(label_while);
    free(label_min_len);
    free(label_eq_len);
}

static void gen_ord(TACDLList *instructions){
    char *label_start_check = create_unique_label("ORD_START_CHECK");
    char *label_end = create_unique_label("ORD_END");
    TacInstruction *instr; // Aktualní instrukce

    // Počateční navratová hodnota
    gen_move_str("GF", "$ret_ifj_fun", "int", "0");

    // Získání 1. argumentu
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Získání 2. argumentu
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");
    
    // Kontrola typů
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    fprintf(stdout, "TYPE GF@$tmp_type_2 GF@$tmp_op_2\n");
    // Kontrola 1. argumentu
    gen_jumpifneq_str("$EXIT25", "GF", "$tmp_type_1", "string", "string");
    // Kontrola 2. argumentu
    fprintf(stdout, "EQ GF@$tmp_2 GF@$tmp_type_2 string@int\n");
    fprintf(stdout, "EQ GF@$tmp GF@$tmp_type_2 string@float\n");
    fprintf(stdout, "OR GF@$tmp GF@$tmp_2 GF@$tmp\n");
    gen_jumpifneq_str("$EXIT25", "GF", "$tmp", "bool", "true");
    gen_jumpifeq_str(label_start_check, "GF", "$tmp_2", "bool", "true");

    // Konverze 2. argumentu
    fprintf(stdout, "ISINT GF@$tmp GF@$tmp_op_2\n");
    gen_jumpifeq_str("$EXIT25", "GF", "$tmp", "bool", "false");
    fprintf(stdout, "FLOAT2INT GF@$tmp_op_2 GF@$tmp_op_2\n");

    // i < 0
    gen_label(label_start_check);
    fprintf(stdout, "LT GF@$tmp GF@$tmp_op_2 int@0\n");
    gen_jumpifeq_str(label_end, "GF", "$tmp", "bool", "true");

    // Ifj.length(s) < i
    fprintf(stdout, "STRLEN GF@$tmp GF@$tmp_op_1\n");
    fprintf(stdout, "LT GF@$tmp_2 GF@$tmp_op_2 GF@$tmp\n");
    gen_jumpifeq_str(label_end, "GF", "$tmp_2", "bool", "false");

    // ord
    fprintf(stdout, "STRI2INT GF@$ret_ifj_fun GF@$tmp_op_1 GF@$tmp_op_2\n");
    
    gen_label(label_end);

    free(label_start_check);
    free(label_end);
}

static void gen_chr(TACDLList *instructions){
    char *label_start_check = create_unique_label("CHR_START_CHECK");
    TacInstruction *instr; // Aktualní instrukce

    // Získání argumentu
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1, 1);
    fprintf(stdout, "\n");

    // Kontrola typu
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    gen_jumpifeq_str(label_start_check, "GF", "$tmp_type_1", "string", "int");
    gen_jumpifneq_str("$EXIT25", "GF", "$tmp_type_1", "string", "float");
    fprintf(stdout, "ISINT GF@$tmp GF@$tmp_op_1\n");
    gen_jumpifeq_str("$EXIT25", "GF", "$tmp_", "string", "false");
    fprintf(stdout, "FLOAT2INT GF@$tmp_op_1 GF@$tmp_op_1\n");

    // chr
    gen_label(label_start_check);
    fprintf(stdout, "INT2CHAR GF@$ret_ifj_fun GF@$tmp_op_1\n");

    free(label_start_check);

}

static void gen_ifj_fun(TACDLList *instructions){
    TacInstruction *instr; // Aktualní instrukce

    // Nastavení standartní navratové hodnoty
    fprintf(stdout, "MOVE GF@$ret_ifj_fun nil@nil\n");

    // Získání instrukce
    TACDLL_GetValue(instructions, &instr);

    char *fun_name = instr->arg1->data.label_name; // Jmeno funkce

    // Switch mezi nazvů funkce
    if (strstr(fun_name, "Ifj.read_str") != NULL) {
        gen_read_str();
    } else if (strstr(fun_name, "Ifj.read_num") != NULL) {
        gen_read_num();
    } else if (strstr(fun_name, "Ifj.write") != NULL) {
        gen_write(instructions);
    } else if (strstr(fun_name, "Ifj.floor") != NULL) {
        gen_floor(instructions);
    } else if (strstr(fun_name, "Ifj.strcmp") != NULL) {
        gen_strcmp(instructions);
    } else if (strstr(fun_name, "Ifj.str") != NULL) {
        gen_str(instructions);
    } else if (strstr(fun_name, "Ifj.length") != NULL) {
        gen_length(instructions);
    } else if (strstr(fun_name, "Ifj.substring") != NULL) {
        gen_substring(instructions);
    } else if (strstr(fun_name, "Ifj.ord") != NULL) {
        gen_ord(instructions);
    } else if (strstr(fun_name, "Ifj.chr") != NULL) {
        gen_chr(instructions);
    }
}

static int getter_check(TacInstruction *instr){
    int result = 0;
    if (instr->arg1 != NULL &&
        (instr->arg1->type == OPERAND_TYPE_LABEL) &&
        instr->operation_code != OP_CALL) 
    {
        gen_create_frame();
        gen_defvar_str("TF", "ret");
        gen_call(instr->arg1->data.label_name);
        gen_move_str("GF", "$getter_1", "TF", "ret");
        result = 1;
    }

    if (instr->arg2 != NULL &&
        instr->arg2->type == OPERAND_TYPE_LABEL &&
        instr->operation_code != OP_CALL) 
    {   
        gen_create_frame();
        gen_defvar_str("TF", "ret");
        gen_call(instr->arg1->data.label_name);
        gen_move_str("GF", "$getter_2", "TF", "ret");
        result = 2;
    }
    return result;
}

static void gen_param_setter(TACDLList Instructions) {
    TacInstruction *instr; // Aktuální instrukce
    TACDLL_GetValue(&Instructions, &instr);
    char *setter_name = instr->result->data.symbol_entry->key;
    Operand *arg1 = instr->arg1;

    TACDLL_First(&Instructions);
    while (TACDLL_IsActive(&Instructions)) {
        TACDLL_GetValue(&Instructions, &instr);
        TACDLL_Next(&Instructions);
        // Kontrola na konec volání setteru
        if (instr->operation_code == OP_LABEL &&
            strcmp(instr->arg1->data.label_name, setter_name) == 0) {
            TACDLL_Next(&Instructions);
            break;
        }
    }

    TACDLL_GetValue(&Instructions, &instr);
    gen_create_frame();
    gen_defvar_str("TF", instr->result->data.symbol_entry->data->unique_name);
    fprintf(stdout, "MOVE TF@%s ", instr->result->data.symbol_entry->data->unique_name);
    gen_operand(arg1, 1);
    fprintf(stdout, "\n");
}

/* ================================================================= */
/* ===== Implementace veřejných funkcí generátoru cílového kódu ==== */
/* ================================================================= */

int generate_code(TACDLList *instructions, Symtable *table) {
    // nastavení globální tabulky symbolů
    gen_global_table = *table;

    // Generování hlavičky kódu
    gen_init(table);

    // Procházení seznamu tříadresných instrukcí a generování kódu
    TACDLL_First(instructions);
    while (TACDLL_IsActive(instructions)) {
        TacInstruction *instr;  // Aktuální tříadresná instrukce
        TACDLL_GetValue(instructions, &instr);

        // ! test output
        fprintf(stdout, "\n# ======================= NEW INSTRUCTION =======================\n");
        fprintf(stdout, "#"); print_single_tac_instruction_gencode(instr);

        switch (instr->operation_code) {
            case OP_JUMP:
                gen_jump(instr->arg2->data.label_name);
                break;
            case OP_JUMP_IF_FALSE:
                gen_jumpifneq(instr);
                break;
            case OP_LABEL:
                gen_label_from_instr(instructions);
                break;
            case OP_ADD:
            case OP_SUBTRACT:
            case OP_MULTIPLY:
            case OP_CONCAT:
                gen_arithmetic(instr);
                break;
            case OP_LESS:
            case OP_GREATER:
            case OP_EQUAL:
                gen_comprasion(instr);
                break;
            case OP_LESS_EQUAL:
            case OP_GREATER_EQUAL:
                gen_eq_comprasion(instr);
                break;
            case OP_NOT_EQUAL:
                gen_not_equal(instr);
                break;
            case OP_DIVIDE:
                gen_divide(instr);
                break;
            case OP_MULTIPLY_STRING:
                gen_mul_str(instr);
                break;
            case OP_RETURN:
            case OP_FUNCTION_END:
                gen_return(instr);
                break;
            case OP_CALL:
                if (strstr(instr->arg1->data.label_name, "Ifj.")) {
                    gen_ifj_fun(instructions);
                } else {
                    gen_param(instructions);
                    gen_call(instr->arg1->data.label_name);
                }
                break;
            case OP_ASSIGN:
                // Kontrola zda je volání setteru
                if (instr->result->type == OPERAND_TYPE_SYMBOL  &&
                    strstr(instr->result->data.symbol_entry->key, "$setter") != NULL) {
                    gen_param_setter(*instructions);
                    gen_call(instr->result->data.symbol_entry->key);
                } else
                    gen_move(instr->result, instr->arg1);
                break;
            case OP_DECLARE:
                gen_declare(instr->result);
                break;
            case OP_IS:
                gen_is(instr);
                break;
            case OP_PARAM:
            case OP_FUNCTION_BEGIN:
            default:
                break;
        }
        TACDLL_Next(instructions);
    }
    return 0;
}

// Функция принимает "сырую" строку и возвращает новую, экранированную для IFJcode25
// ВАЖНО: Возвращаемая строка аллоцируется динамически, не забудьте сделать free()!
char* string_to_ifjcode(const char *original) {
    if (original == NULL) 
        return NULL;

    // 1. Проход: Вычисляем необходимый размер буфера
    size_t length = 0;
    for (int i = 0; original[i] != '\0'; i++) {
        char c = original[i];
        
        // Условия экранирования согласно спецификации IFJcode25 (раздел 10.3):
        // - ASCII 000-032 (управляющие и пробел)
        // - ASCII 035 (#)
        // - ASCII 092 (\)
        if (c <= 32 || c == 35 || c == 92) {
            length += 4; // \xyz -> 4 символа
        } else {
            length += 1; // Обычный символ
        }
    }

    // Выделяем память (+1 для нуль-терминатора)
    char *escaped = (char *)malloc(length + 1);
    if (escaped == NULL) {
        // Обработка ошибки выделения памяти (например, вызов вашей функции error_exit)
        fprintf(stderr, "Internal compiler error: memory allocation failed\n");
        exit(99); // Код 99 для внутренней ошибки [cite: 43]
    }

    // 2. Проход: Заполнение новой строки
    size_t pos = 0;
    for (int i = 0; original[i] != '\0'; i++) {
        unsigned char c = (unsigned char)original[i];

        if (c == 10) {
            // Специальный случай для новой строки
            sprintf(escaped + pos, "\\010");
            pos += 4;
        } else if (c == 13) {
            // Специальный случай для новой строки
            sprintf(escaped + pos, "\\013");
            pos += 4;
        
        } else if (c <= 32 || c == 35 || c == 92) {
            // Записываем escape-последовательность
            // %03d гарантирует 3 цифры (например, \010, а не \10)
            sprintf(escaped + pos, "\\%03d", c);
            pos += 4;
        } else {
            // Копируем символ как есть
            escaped[pos] = c;
            pos += 1;
        }
    }

    escaped[pos] = '\0'; // Завершаем строку
    return escaped;
}