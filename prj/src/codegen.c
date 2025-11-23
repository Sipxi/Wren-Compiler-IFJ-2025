// Имплементация функций генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//
#include "codegen.h"
#include "printer.h"
#include <stdlib.h>

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
 * @brief Generuje instrukci JUMPIFEQ pro podmíněný skok, 
 *  pokud jsou dvě hodnoty rovny.
 * 
 * @param instr Ukazatel na instrukci TAC obsahující podmínky skoku.
 */
static void gen_jumpifeq(TacInstruction *instr);

/**
 * @brief Generuje kód pro zadaný operánd.
 * 
 * @param op Ukazatel na operánd k vygenerování.
 */
static void gen_operand(Operand *op);

// ? Нужно ли  
/**
 * @brief Generuje operandy pro zadanou instrukci TAC.
 * 
 * @param instr Ukazatel na instrukci TAC k vygenerování.
 */
static void gen_tac(TacInstruction *instr);

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
static void gen_defvar(char *var);

/**
 * @brief Generuje instrukci MOVE pro přiřazení hodnoty.
 * 
 * @param dest Cílový operánd pro přiřazení.
 * @param src Zdrojový operánd pro přiřazení.
 */
static void gen_move(Operand *dest, Operand *src);

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
 * @brief Generuje řetězcovou reprezentaci operandu. 
 *  Volající je povinen uvolnit paměť.
 * 
 * @param op Ukazatel na operánd k vygenerování.
 * @return char *Řetězcová reprezentace operandu.
 */
static char *gen_operand_str(Operand *op);


/* =================================================================== */
/* ===== Implementace privátních funkcí generátoru cílového kódu ===== */
/* =================================================================== */

char* gen_operand_str(Operand *op) {
    char *result = NULL;
    int length = 0;

    // Generování proměnné
    if (op->type == OPERAND_TYPE_SYMBOL) {
        const char *prefix;
        // Kontrola globální proměnné
        if (symtable_lookup(&gen_global_table, op->data.symbol_entry->key) != NULL) {
            prefix = "GF@";
        } else {
            prefix = "LF@";
        }
        const char *unique_name = op->data.symbol_entry->data->unique_name;

        // Vypočítáme délku: snprintf s NULL vrací počet znaků, které by byly zapsány
        length = snprintf(NULL, 0, "%s%s", prefix, unique_name);
        
        // Allokujeme paměť pro řetězec
        result = malloc(length + 1);
        if (result == NULL) return NULL;

        // Zapíšeme řetězec
        sprintf(result, "%s%s", prefix, unique_name);

    // Generování konstanty
    } else if (op->type == OPERAND_TYPE_CONSTANT) {
        switch (op->data.constant.type) {
            // Generování celého čísla
            case TYPE_NUM:
                length = snprintf(NULL, 0, "int@%i", op->data.constant.value.int_value);
                result = malloc(length + 1);
                if (result) sprintf(result, "int@%i", op->data.constant.value.int_value);
                break;

            // Generování čísla s řadovou čárkou
            case TYPE_FLOAT:
                // Používáme %a pro šestnáctkové zobrazení float (požadavek IFJcode25)
                length = snprintf(NULL, 0, "float@%a", op->data.constant.value.float_value);
                result = malloc(length + 1);
                if (result) 
                    sprintf(result, "float@%a", op->data.constant.value.float_value);
                break;

            case TYPE_STR: {
                // Generuje escapovaný řetězec
                char *escaped_str = string_to_ifjcode(op->data.constant.value.str_value);
                if (escaped_str == NULL) 
                    return NULL;

                
                length = snprintf(NULL, 0, "string@%s", escaped_str);
                result = malloc(length + 1);
                if (result) 
                    sprintf(result, "string@%s", escaped_str);
                
                free(escaped_str);
                break;
            }

            case TYPE_NIL:
                result = strdup_c99("nil@nil"); 
                break;

            default:
                return NULL;
        }
    } else if (op->type == OPERAND_TYPE_TEMP) {
        length = snprintf(NULL, 0, "LF@$t$%i", op->data.temp_id);
        result = malloc(length + 1);
        if (result) 
            sprintf(result, "LF@$t$%i", op->data.temp_id);
    }

    return result;
}

static char *gen_operand_str(Operand *op){
    if (op->type == OPERAND_TYPE_SYMBOL) {
        if (symtable_lookup(&gen_global_table, op->data.symbol_entry->key) != NULL) 
            fprintf(stdout, "GF@");
        else 
            fprintf(stdout, "LF@");
        fprintf(stdout, "%s", op->data.symbol_entry->data->unique_name);
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
        // Обработка других типов операндов по мере необходимости
        return;
    } else if (op->type == OPERAND_TYPE_TEMP) {
        // Для временных переменных можно использовать LF
        fprintf(stdout, "LF@$t$%i", op->data.temp_id);
    }
}


static void gen_init(Symtable *table){
    // Hlavicka IFJcode25
    fprintf(stdout, ".IFJcode25\n");

    // Definice pomocných globálních proměnných
    fprintf(stdout, "DEFVAR GF@$tmp_2\n");
    fprintf(stdout, "DEFVAR GF@$tmp\n");
    fprintf(stdout, "DEFVAR GF@$tmp_type_1\n");
    fprintf(stdout, "DEFVAR GF@$tmp_type_2\n");
    fprintf(stdout, "DEFVAR GF@$tmp_op_1\n");
    fprintf(stdout, "DEFVAR GF@$tmp_op_2\n");
    fprintf(stdout, "DEFVAR GF@$ret_ifj_fun\n");

    // Definice globálních proměnných z tabulky symbolů
    for (size_t i = 0; i < table->capacity; i++) {
        TableEntry* entry = &table->entries[i];
        if (entry->status == SLOT_OCCUPIED) {
            // Определяем переменную в GF
            if (entry->data->kind == KIND_VAR) {
                fprintf(stdout, "DEFVAR GF@%s\n", entry->data->unique_name);
                fprintf(stdout, "MOVE GF@%s nil@nil\n", entry->data->unique_name);
            }
        }
    }

    gen_create_frame();
    fprintf(stdout, "CALL $$main\n\n");
    fprintf(stdout, "EXIT int@0\n");
    fprintf(stdout, "LABEL $EXIT25\n");
    fprintf(stdout, "EXIT int@25\n");
    fprintf(stdout, "LABEL $EXIT26\n");
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
    fprintf(stdout, "LABEL $%s\n", label_name);
}
static void gen_jump(char* label_name){
    fprintf(stdout, "JUMP $%s\n", label_name);
}
static void gen_call(char* label_name){
    fprintf(stdout, "CALL $%s\n", label_name);
}
static void gen_return(TacInstruction *instr){
    if (instr->result != NULL) {
        fprintf(stdout, "MOVE LF@ret ");
        gen_operand(instr->result);
        fprintf(stdout, "\n");
    }
    gen_pop_frame();
    fprintf(stdout, "RETURN\n");
}

static void gen_jumpifeq(TacInstruction *instr){
    // Если результат в GF@$tmp
    // fprintf(stdout, "JUMPIFEQ $%s GF@$tmp bool@false", label_name);
    // !Пока делаем из резулата
    fprintf(stdout, "JUMPIFEQ $%s ", instr->arg2->data.label_name);
    gen_operand(instr->arg1);
    fprintf(stdout, " bool@false\n");

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

static void gen_operand(Operand *op){
    if (op->type == OPERAND_TYPE_SYMBOL) {
        if (symtable_lookup(&gen_global_table, op->data.symbol_entry->key) != NULL) 
            fprintf(stdout, "GF@");
        else 
            fprintf(stdout, "LF@");
        fprintf(stdout, "%s", op->data.symbol_entry->data->unique_name);
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
        // Обработка других типов операндов по мере необходимости
        return;
    } else if (op->type == OPERAND_TYPE_TEMP) {
        // Для временных переменных можно использовать LF
        fprintf(stdout, "LF@$t$%i", op->data.temp_id);
    }

}
static void gen_tac(TacInstruction *instr){
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->arg1);
    fprintf(stdout, " ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
}

static void gen_type(TacInstruction *instr) {
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
}
static void gen_divide(TacInstruction *instr){
    char *label_div = create_unique_label("DIV");
    char *label_idiv = create_unique_label("IDIV");
    char *label_end = create_unique_label("END_DIV");
    char *label_arg_1_float= create_unique_label("ARG1_TO_FLOAT");
    char *label_arg_2_float= create_unique_label("ARG2_TO_FLOAT");
    char *label_arg_2_check= create_unique_label("ARG2_CHECK");
    char *label_start_operation= create_unique_label("START_OPERATION");
    // Проверка типов
    gen_type(instr);
    


    // Проверка на операнды nil и bool
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@bool\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@bool\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@string\n");
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@int\n", label_arg_1_float);
    gen_label(label_arg_2_check);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_2 string@int\n", label_arg_2_float);
    fprintf(stdout, "JUMP $%s\n", label_start_operation);
    gen_label(label_arg_1_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP $%s\n", label_arg_2_check);
    gen_label(label_arg_2_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");


    gen_label(label_start_operation);
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    // разделение на деление float и int 
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@float\n", label_div);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@int\n", label_idiv);
    // если не float и не int то ошибка
    gen_jump("$EXIT26");

    // деление float
    gen_label(label_div);
    //деление
    fprintf(stdout, "DIV ");
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");
    // прыжок в конец
    fprintf(stdout, "JUMP $%s\n", label_end);

    // деление int
    gen_label(label_idiv);
    //деление
    fprintf(stdout, "IDIV ");
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");
    gen_convert_result(instr);
    gen_label(label_end);
    
    free(label_div);
    free(label_idiv);
    free(label_end);
    free(label_arg_1_float);
    free(label_arg_2_float);
    free(label_arg_2_check);
    free(label_start_operation);
}
static void gen_same_operand_check(TacInstruction *instr){
    
    // Проверка на операнды nil и bool
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@bool\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@bool\n");

    // Дополнительная проверка для конкатенации
    if (instr->operation_code == OP_CONCAT) {
        fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@int\n");
        fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@int\n");
        fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@float\n");
        fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@float\n");
        fprintf(stdout, "MOVE GF@$tmp_op_1 ");
        gen_operand(instr->arg1);
        fprintf(stdout, "\n");
        fprintf(stdout, "MOVE GF@$tmp_op_2 ");
        gen_operand(instr->arg2);
        fprintf(stdout, "\n");
        return;
    }

    char *label_start_operation = create_unique_label("START_OPERATION");
    char *label_arg_2_float= create_unique_label("ARG2_TO_FLOAT");

    // Общая проверка для остальных операций
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@string\n");
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 GF@$tmp_type_2\n", label_start_operation);

    // конвертация в float
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_2 string@int\n", label_arg_2_float);

    fprintf(stdout, "INT2FLOAT GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    gen_jump(label_start_operation);

    gen_label(label_arg_2_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");

    gen_label(label_start_operation);

    free(label_start_operation);
    free(label_arg_2_float);
}
static void gen_convert_result(TacInstruction *instr){
    char *label_end_convert = create_unique_label("END_CONVERT");
    // Конвертация результата обратно в int, если нужно
    fprintf(stdout, "ISINT GF@$tmp ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp bool@false\n", label_end_convert);
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@int\n", label_end_convert);
    fprintf(stdout, "FLOAT2INT ");
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");

    gen_label(label_end_convert);

    free(label_end_convert);
}
static void gen_arithmetic(TacInstruction *instr){
    gen_type(instr);
    gen_same_operand_check(instr);
    // Проверка типов

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
        // Добавьте другие арифметические операции по мере необходимости
        default:
            break;
    }
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");
    if (instr->operation_code != OP_CONCAT) {
        gen_convert_result(instr);
    }
}

static void gen_defvar(char *var){
    fprintf(stdout, "DEFVAR GF@%s\n", var);
}

static void gen_move(Operand *dest, Operand *src){
    fprintf(stdout, "MOVE ");
    gen_operand(dest);
    fprintf(stdout, " ");
    if (src->type == OPERAND_TYPE_LABEL) {
        if (strstr(src->data.label_name, "Ifj.") != NULL)
            fprintf(stdout, "GF@$ret_ifj_fun");
        else
            fprintf(stdout, "TF@ret");
    } else
        gen_operand(src);
    fprintf(stdout, "\n");

}

static void gen_param(TACDLList *instructions){
    gen_create_frame();

    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    char *label_name = instr->arg1->data.label_name;

    TACDLList instrs_fun = *instructions;
    TacInstruction *instr_param;
    TACDLL_First(&instrs_fun);
    while (TACDLL_IsActive(&instrs_fun)){
        TACDLL_GetValue(&instrs_fun, &instr_param);
        TACDLL_Next(&instrs_fun);    
        if (instr_param->operation_code == OP_LABEL &&
            strcmp(instr_param->arg1->data.label_name, label_name) == 0)
            break;
    }

    TACDLL_Next(&instrs_fun);    
    TACDLL_Next(instructions);
    while (TACDLL_IsActive(instructions)) {
        TACDLL_GetValue(instructions, &instr);
        TACDLL_GetValue(&instrs_fun, &instr_param);

        if (instr->operation_code != OP_PARAM){
            TACDLL_Previous(instructions);
            break;
        }

        char *param_name = instr_param->result->data.symbol_entry->data->unique_name;
        fprintf(stdout, "DEFVAR TF@%s\n", param_name);
        fprintf(stdout, "MOVE TF@%s ", param_name);
        gen_operand(instr->arg1);
        fprintf(stdout, "\n");

        TACDLL_Next(instructions);
        TACDLL_Next(&instrs_fun);
    }
}

static void gen_mul_str(TacInstruction *instr){
    gen_type(instr);
    // Проверка типов
    fprintf(stdout, "JUMPIFNEQ $EXIT26 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@bool\n");
    // Проверка на целое число
    fprintf(stdout, "ISINT GF@$tmp ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp bool@false\n");
    
    char *label_loop = create_unique_label("MUL_STR_LOOP");
    char *label_loop_end = create_unique_label("MUL_STR_LOOP_END");
    char *label_end_convert = create_unique_label("END_CONVERT");
    // Перевод на инт
    fprintf(stdout, "MOVE GF@$tmp ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_2 string@int\n", label_end_convert);
    fprintf(stdout, "FLOAT2INT GF@$tmp ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    // Проверка на неотрицательность
    fprintf(stdout, "LT GF@$tmp_2 GF@$tmp int@0\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_2 bool@true\n");
    gen_label(label_end_convert);
    // Инициализация счетчика и результата
    fprintf(stdout, "MOVE ");
    gen_operand(instr->result);
    fprintf(stdout, " string@\n");
    // Начало цикла
    gen_label(label_loop);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp int@0\n", label_loop_end);
    // Конкатенация строки
    fprintf(stdout, "CONCAT ");
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    // Декремент счетчика
    fprintf(stdout, "SUB GF@$tmp GF@$tmp int@1\n");
    // Переход к началу цикла
    fprintf(stdout, "JUMP $%s\n", label_loop);
    // Конец цикла
    gen_label(label_loop_end);
    free(label_loop);
    free(label_loop_end);
    free(label_end_convert);
}

static void gen_comprasion(TacInstruction *instr){
    gen_type(instr);
    // Проверка типов
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@nil\n");
    char *label_start_operation = create_unique_label("START_OPERATION");
    char *label_arg_2_float= create_unique_label("ARG2_TO_FLOAT");
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 GF@$tmp_type_2\n", label_start_operation);
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@bool\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@bool\n");

     // конвертация в float
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_2 string@int\n", label_arg_2_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP %s\n", label_start_operation);

    gen_label(label_arg_2_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");

    
    gen_label(label_start_operation);
    fprintf(stdout, "JUMPIFNEQ $EXIT26 GF@$tmp_type_1 GF@$tmp_type_2\n");
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
        // Добавьте другие операции сравнения по мере необходимости
        default:
            break;
    }
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");

    
    free(label_start_operation);
    free(label_arg_2_float);
}
static void gen_eq_comprasion(TacInstruction *instr){
    // Проверка на REATER и LESS
    gen_comprasion(instr);
    // Проверка на EQUAL
    fprintf(stdout, "EQ GF@$tmp ");
    gen_operand(instr->arg1);
    fprintf(stdout, " ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    // OR результата с предыдущим
    fprintf(stdout, "OR ");
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");
}
static void gen_not_equal(TacInstruction *instr){
    // Проверка на EQUAL
    gen_comprasion(instr);
    // NOT результата
    fprintf(stdout, "NOT ");
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");
}
static void gen_declare(Operand *result){
    fprintf(stdout, "MOVE ");
    gen_operand(result);
    fprintf(stdout, " nil@nil\n");
}

static void gen_function_begin(TACDLList  *instructions){
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    if (strcmp(instr->arg1->data.label_name, "main$0") == 0) {
        fprintf(stdout, "LABEL $$main\n");        
    } else
        gen_label(instr->arg1->data.label_name);
    gen_push_frame();
    
    fprintf(stdout, "DEFVAR LF@ret\n");
    fprintf(stdout, "MOVE LF@ret nil@nil\n");
    
    TACDLL_Next(instructions);
    while (TACDLL_IsActive(instructions)){
        TACDLL_GetValue(instructions, &instr);
        TacOperationCode op_code = instr->operation_code;
        if (op_code == OP_FUNCTION_END) {
            break;
        } else if (op_code == OP_DECLARE) {
            // Разкоментить при тестировке с готовой таблицей символов
            // if (instr->result->data.symbol_entry->data->nesting_level != 0)
                gen_defvar(gen_operand_str(instr->result));
        }
        TACDLL_Next(instructions);
    }
}

static void gen_label_from_instr(TACDLList  *instructions){
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    TACDLList instructions_copy = *instructions;
    TACDLL_Next(&instructions_copy);
    if (TACDLL_IsActive(&instructions_copy)) {
        TACDLL_GetValue(&instructions_copy, &instr);
    }
    if (instr->operation_code == OP_FUNCTION_BEGIN || instr->operation_code == OP_PARAM) {
        TACDLL_Previous(&instructions_copy);
        gen_function_begin(&instructions_copy);
    } else {
        TACDLL_GetValue(instructions, &instr);
        gen_label(instr->arg1->data.label_name);
    }
}

static void gen_is(TacInstruction *instr){
    char *type = instr->arg2->data.constant.value.str_value;
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "EQ ");
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_type_1 ");
    if (strcmp(type, "String") == 0)
        fprintf(stdout, "string@string\n");
    else if (strcmp(type, "Null") == 0)
        fprintf(stdout, "string@nil\n");
    else if (strcmp(type, "Num") == 0){
        fprintf(stdout, "string@int\n");
        fprintf(stdout, "EQ GF@$tmp_type_1 GF@$tmp_type_1 string@float\n");
        fprintf(stdout, "OR ");
        gen_operand(instr->result);
        fprintf(stdout, " GF@$tmp_type_1 ");
        gen_operand(instr->result);
        fprintf(stdout, "\n");
    }

}
static void gen_read_str(){
    fprintf(stdout, "READ GF@$ret_ifj_fun string\n");
}
static void gen_read_num(){
    char *label_end = create_unique_label("READ_NUM_END");
    fprintf(stdout, "READ GF@$ret_ifj_fun float\n");
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$ret_ifj_fun\n");
    fprintf(stdout, "JUMPIFNEQ $%s GF@$tmp_type_1 string@float\n", label_end);
    fprintf(stdout, "ISINT GF@$tmp GF@$ret_ifj_fun\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp bool@false\n", label_end);
    fprintf(stdout, "FLOAT2INT GF@$ret_ifj_fun GF@$ret_ifj_fun\n");
    gen_label(label_end);
    free (label_end);
}
static void gen_write(TACDLList *instructions){
    TACDLL_Next(instructions);
    TacInstruction *instr;
    char *label_write = create_unique_label("WRITE");
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@nil\n", label_write);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@string\n", label_write);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@bool\n", label_write);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@int\n", label_write);

    fprintf(stdout, "ISINT GF@$tmp_type_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 bool@false\n", label_write);
    fprintf(stdout, "FLOAT2INT GF@$tmp ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    gen_label(label_write);
    fprintf(stdout, "WRITE GF@$tmp\n");
    free(label_write);
}
static void gen_floor(TACDLList *instructions){
    TacInstruction *instr;
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$ret_ifj_fun\n");
    char *label_end = create_unique_label("FLOOR_END");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@int\n", label_end);
    fprintf(stdout, "JUMPIFNEQ $EXIT25 GF@$tmp_type_1 string@float\n");
    fprintf(stdout, "FLOAT2INT GF@$ret_ifj_fun GF@$ret_ifj_fun\n");
    gen_label(label_end);
    free(label_end);
}
static void gen_str(TACDLList *instructions) {
    TACDLL_Next(instructions);
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    
    char *label_float2str = create_unique_label("FLOAT2STR");
    char *label_convert = create_unique_label("CONVERT");
    char *label_int2str = create_unique_label("INT2STR");
    char *label_nil2str = create_unique_label("NIL2STR");
    char *label_bool2str = create_unique_label("BOOL2STR");
    char *label_true2str = create_unique_label("TRUE2STR");
    char *label_end = create_unique_label("STR_END");

    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$ret_ifj_fun\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@string\n", label_end);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@int\n", label_int2str);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@float\n", label_float2str);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@nil\n", label_nil2str);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@bool\n", label_bool2str);
    
    gen_label(label_float2str);
    fprintf(stdout, "ISINT GF@$tmp GF@$ret_ifj_fun\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp bool@true\n", label_int2str);
    fprintf(stdout, "FLOAT2STR GF@$ret_ifj_fun GF@$ret_ifj_fun\n");
    fprintf(stdout, "JUMP $%s\n", label_end);

    gen_label(label_convert);
    fprintf(stdout, "FLOAT2INT GF@$ret_ifj_fun GF@$ret_ifj_fun\n");

    gen_label(label_int2str);
    fprintf(stdout, "INT2STR GF@$ret_ifj_fun GF@$ret_ifj_fun\n");
    fprintf(stdout, "JUMP $%s\n", label_end);

    gen_label(label_nil2str);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun string@null\n");
    fprintf(stdout, "JUMP $%s\n", label_end);

    gen_label(label_bool2str);
    fprintf(stdout, "JUMPIFEQ $%s GF@$ret_ifj_fun bool@true\n", label_true2str);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun string@false\n");
    fprintf(stdout, "JUMP $%s\n", label_end);
    gen_label(label_true2str);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun string@true\n");
    
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
    TACDLL_Next(instructions);
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "STRLEN GF@$ret_ifj_fun ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
}
static void gen_substring(TACDLList *instructions) {
    char *label_end_convert = create_unique_label("SUBSTRING_END_CONVERT");
    char *label_convert_arg_2 = create_unique_label("SUBSTRING_CONVERT_ARG2");
    char *label_end = create_unique_label("SUBSTRING_END");
    char *label_while = create_unique_label("WHILE_START");
    fprintf(stdout, "MOVE GF@$ret_ifj_fun nil@nil\n");
    TACDLL_Next(instructions);
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);

    fprintf(stdout, "MOVE GF@$tmp ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp\n");
    fprintf(stdout, "JUMPIFNEQ $EXIT25 GF@$tmp_type_1 string@string\n");

    // op 1
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    fprintf(stdout, "EQ GF@$tmp_type_2 GF@$tmp_type_1 string@float\n");
    fprintf(stdout, "EQ GF@$tmp_type_1 GF@$tmp_type_1 string@int\n");
    fprintf(stdout, "OR GF@$tmp_type_1 GF@$tmp_type_1 GF@$tmp_type_2\n");
    fprintf(stdout, "JUMPIFNEQ $EXIT25 GF@$tmp_type_1 bool@true\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_2 bool@false\n", label_convert_arg_2);
    
    fprintf(stdout, "ISINT GF@$tmp_type_2 GF@$tmp_op_1\n");
    fprintf(stdout, "JUMPIFEQ $EXIT25 GF@$tmp_type_2 bool@false\n");
    fprintf(stdout, "FLOAT2INT GF@$tmp_op_1 GF@$tmp_op_1\n");
    

    gen_label(label_convert_arg_2);
    // op 2
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_2\n");
    fprintf(stdout, "EQ GF@$tmp_type_2 GF@$tmp_type_1 string@float\n");
    fprintf(stdout, "EQ GF@$tmp_type_1 GF@$tmp_type_1 string@int\n");
    fprintf(stdout, "OR GF@$tmp_type_1 GF@$tmp_type_1 GF@$tmp_type_2\n");
    fprintf(stdout, "JUMPIFNEQ $EXIT25 GF@$tmp_type_1 bool@true\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_2 bool@false\n", label_end_convert);
    
    fprintf(stdout, "ISINT GF@$tmp_type_2 GF@$tmp_op_2\n");
    fprintf(stdout, "JUMPIFEQ $EXIT25 GF@$tmp_type_2 bool@false\n");
    fprintf(stdout, "FLOAT2INT GF@$tmp_op_2 GF@$tmp_op_2\n");
    gen_label(label_end_convert);
    
    // 𝑖 < 0
    // 𝑗 < 0
    // 𝑖 > 𝑗
    fprintf(stdout, "LT GF@$tmp_2 GF@$tmp_op_1 int@0\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_2 bool@true\n", label_end);
    fprintf(stdout, "LT GF@$tmp_2 GF@$tmp_op_2 int@0\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_2 bool@true\n", label_end);
    fprintf(stdout, "GT GF@$tmp_2 GF@$tmp_op_1 GF@$tmp_op_2\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_2 bool@true\n", label_end);


    // 𝑖 ≥ Ifj.length(𝑠)
    // 𝑗 > Ifj.length(𝑠)
    fprintf(stdout, "STRLEN GF@$tmp_2 GF@$tmp\n");
    fprintf(stdout, "GT GF@$tmp_type_1 GF@$tmp_op_2 GF@$tmp_2\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 bool@true\n", label_end);
    fprintf(stdout, "LT GF@$tmp_type_1 GF@$tmp_op_1 GF@$tmp_2\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 bool@false\n", label_end);

    fprintf(stdout, "MOVE GF@$ret_ifj_fun string@\n");

    gen_label(label_while);
    fprintf(stdout, "GETCHAR GF@$tmp_2 GF@$tmp GF@$tmp_op_1\n");
    fprintf(stdout, "CONCAT GF@$ret_ifj_fun GF@$ret_ifj_fun GF@$tmp_2\n");

    fprintf(stdout, "ADD GF@$tmp_op_1 GF@$tmp_op_1 int@1\n");
    fprintf(stdout, "GT GF@$tmp_2 GF@$tmp_op_1 GF@$tmp_op_2\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_2 bool@false\n", label_while);

    gen_label(label_end);
    free(label_while);
    free(label_convert_arg_2);
    free(label_end_convert);
    free(label_end);
}
static void gen_strcmp(TACDLList *instructions){
    char *label_min_len = create_unique_label("STRCMP_MIN_LEN");
    char *label_while = create_unique_label("STRCMP_WHILE");
    char *label_end = create_unique_label("STRCMP_END");
    char *label_gt_lt = create_unique_label("STRCMP_GT_LT");
    char *label_longer = create_unique_label("STRCMP_LONGER");
    char *label_longer_1 = create_unique_label("STRCMP_LONGER_1");
    char *label_longer_2 = create_unique_label("STRCMP_LONGER_2");
    TacInstruction *instr;
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    // Проверка типов
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    fprintf(stdout, "TYPE GF@$tmp_type_2 GF@$tmp_op_2\n");
    fprintf(stdout, "JUMPIFNEQ $EXIT25 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFNEQ $EXIT25 GF@$tmp_type_2 string@string\n");

    // Нахождение минимальной длины
    fprintf(stdout, "STRLEN GF@$tmp GF@$tmp_op_1\n");
    fprintf(stdout, "STRLEN GF@$tmp_2 GF@$tmp_op_2\n");
    fprintf(stdout, "GT GF@$tmp_type_1 GF@$tmp_2 GF@$tmp\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 bool@true\n", label_min_len);
    fprintf(stdout, "MOVE GF@$tmp GF@$tmp_2\n");
    gen_label(label_min_len);
    fprintf(stdout, "MOVE GF@$tmp_2 int@-1\n");
    gen_label(label_while);
    fprintf(stdout, "ADD GF@$tmp_2 GF@$tmp_2 int@1\n");
    fprintf(stdout, "LT GF@$tmp_type_1 GF@$tmp_2 GF@$tmp\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 bool@false\n", label_longer);
    fprintf(stdout, "GETCHAR GF@$tmp_type_1 GF@$tmp_op_1 GF@$tmp_2\n");
    fprintf(stdout, "GETCHAR GF@$tmp_type_2 GF@$tmp_op_2 GF@$tmp_2\n");
    fprintf(stdout, "JUMPIFNEQ $%s GF@$tmp_type_1 GF@$tmp_type_2\n", label_gt_lt);
    fprintf(stdout, "JUMP $%s\n", label_while);

    gen_label(label_longer);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun int@0\n");
    fprintf(stdout, "STRLEN GF@$tmp GF@$tmp_op_1\n");
    fprintf(stdout, "STRLEN GF@$tmp_2 GF@$tmp_op_2\n");
    fprintf(stdout, "LT GF@$tmp_type_1 GF@$tmp GF@$tmp_2\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 bool@true\n", label_longer_1);
    fprintf(stdout, "GT GF@$tmp_type_1 GF@$tmp GF@$tmp_2\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 bool@true\n", label_longer_2);
    fprintf(stdout, "JUMP $%s\n", label_end);
    gen_label(label_longer_1);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun int@-1\n");
    fprintf(stdout, "JUMP $%s\n", label_end);
    gen_label(label_longer_2);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun int@1\n");
    fprintf(stdout, "JUMP $%s\n", label_end);

    gen_label(label_gt_lt);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun int@-1\n");
    fprintf(stdout, "LT GF@$tmp GF@$tmp_type_1 GF@$tmp_type_2\n"); 
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp bool@true\n", label_end);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun int@1\n");

    
    gen_label(label_end);
    free(label_gt_lt);
    free(label_end);
    free(label_while);
    free(label_longer);
    free(label_longer_1);
    free(label_longer_2);
    free(label_min_len);
}
static void gen_ord(TACDLList *instructions){
    char *label_start_check = create_unique_label("ORD_START_CHECK");
    char *label_end = create_unique_label("ORD_END");
    TACDLL_Next(instructions);
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$ret_ifj_fun int@0\n");
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    fprintf(stdout, "TYPE GF@$tmp_type_2 GF@$tmp_op_2\n");
    fprintf(stdout, "JUMPIFNEQ $EXIT25 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "EQ GF@$tmp_2 GF@$tmp_type_2 string@int\n");
    fprintf(stdout, "EQ GF@$tmp GF@$tmp_type_2 string@float\n");
    fprintf(stdout, "OR GF@$tmp GF@$tmp_2 GF@$tmp\n");
    fprintf(stdout, "JUMPIFNEQ $EXIT25 GF@$tmp bool@true\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_2 bool@true\n", label_start_check);
    fprintf(stdout, "ISINT GF@$tmp GF@$tmp_op_2\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp bool@false\n");
    fprintf(stdout, "FLOAT2INT GF@$tmp_op_2 GF@$tmp_op_2\n");

    gen_label(label_start_check);
    fprintf(stdout, "LT GF@$tmp GF@$tmp_op_2 int@0\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp bool@true\n", label_end);

    fprintf(stdout, "STRLEN GF@$tmp GF@$tmp_op_1\n");
    fprintf(stdout, "LT GF@$tmp_2 GF@$tmp_op_2 GF@$tmp\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_2 bool@false\n", label_end);

    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp int@0\n", label_end);

    fprintf(stdout, "STRI2INT GF@$ret_ifj_fun GF@$tmp_op_1 GF@$tmp_op_2\n");
    
    
    gen_label(label_end);
    free(label_start_check);
    free(label_end);
}
static void gen_chr(TACDLList *instructions){
    char *label_start_check = create_unique_label("CHR_START_CHECK");
    TACDLL_Next(instructions);
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    fprintf(stdout, "EQ GF@$tmp_2 GF@$tmp_type_1 string@int\n");
    fprintf(stdout, "EQ GF@$tmp GF@$tmp_type_1 string@float\n");
    fprintf(stdout, "OR GF@$tmp GF@$tmp_2 GF@$tmp\n");
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_2 bool@true\n", label_start_check);
    fprintf(stdout, "ISINT GF@$tmp GF@$tmp_op_1\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp bool@false\n");
    fprintf(stdout, "FLOAT2INT GF@$tmp_op_1 GF@$tmp_op_1\n");

    gen_label(label_start_check);
    fprintf(stdout, "INT2CHAR GF@$ret_ifj_fun GF@$tmp_op_1\n");

    free(label_start_check);

}
static void gen_ifj_fun(TACDLList *instructions){
    fprintf(stdout, "MOVE GF@$ret_ifj_fun nil@nil\n");
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    if (strstr(instr->arg1->data.label_name, "Ifj.read_str") != NULL) {
        gen_read_str();
    } else if (strstr(instr->arg1->data.label_name, "Ifj.read_num") != NULL) {
        gen_read_num();
    } else if (strstr(instr->arg1->data.label_name, "Ifj.write") != NULL) {
        gen_write(instructions);
    } else if (strstr(instr->arg1->data.label_name, "Ifj.floor") != NULL) {
        gen_floor(instructions);
    } else if (strstr(instr->arg1->data.label_name, "Ifj.strcmp") != NULL) {
        gen_strcmp(instructions);
    } else if (strstr(instr->arg1->data.label_name, "Ifj.str") != NULL) {
        gen_str(instructions);
    } else if (strstr(instr->arg1->data.label_name, "Ifj.length") != NULL) {
        gen_length(instructions);
    } else if (strstr(instr->arg1->data.label_name, "Ifj.substring") != NULL) {
        gen_substring(instructions);
    } else if (strstr(instr->arg1->data.label_name, "Ifj.ord") != NULL) {
        gen_ord(instructions);
    } else if (strstr(instr->arg1->data.label_name, "Ifj.chr") != NULL) {
        gen_chr(instructions);
    }
}


/* ================================================================= */
/* ===== Implementace veřejných funkcí generátoru cílového kódu ==== */
/* ================================================================= */

int generate_code(TACDLList *instructions, Symtable *table) {
    gen_global_table = *table;
    gen_init(table);
    TACDLL_First(instructions);
    while (TACDLL_IsActive(instructions)) {
        TacInstruction *instr;
        TACDLL_GetValue(instructions, &instr);

        fprintf(stdout, "\n# ======================= NEW INSTRUCTION =======================\n");
        // fprintf(stdout, "#"); print_single_tac_instruction(instr);

        switch (instr->operation_code) {
        case OP_JUMP:
            gen_jump(instr->arg2->data.label_name);
            break;
        case OP_JUMP_IF_FALSE:
            gen_jumpifeq(instr);
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