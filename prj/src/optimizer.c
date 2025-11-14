/**
 * @file optimizer.c
 *
 * @brief Имплементация оптимизатора трехадресного кода (TAC).
 *
 * Author:
 *    - Serhij Čepil (253038)
 *
 */

#include "optimizer.h"
#include "tac.h"

#include <stdbool.h>
#include <string.h>
#include <math.h>
 /* ======================================*/
 /* ===== Прототипы приватных функций =====*/
 /* ======================================*/

 /**
  * @brief Выполняет оптимизацию "Constant Folding" на списке TAC инструкций.
  *
  * Эта функция ищет арифметические операции с константными операндами
  * и вычисляет их во время компиляции, заменяя инструкцию на
  * присваивание результата.
  *
  * Например :
  * t1 = 2 + 3  ->  t1 = 5
  *
  * @param tac_list Список TAC инструкций для оптимизации.
  */
void constant_folding(DLList *tac_list);

/**
 * @brief Выполняет оптимизацию "Dead Code Elimination" на списке TAC инструкций.
 *
 * Эта функция удаляет инструкции, которые не влияют на результат программы,
 * такие как присваивания в неиспользуемые переменные.
 *
 * @param tac_list Список TAC инструкций для оптимизации.
 */
void dead_code_elimination(DLList *tac_list);

/**
 * @brief Проверяет, является ли операция арифметической.
 *
 * @param op_code Код операции TAC.
 * @return true если операция арифметическая, иначе false.
 */
bool is_instruction_arithmetic(TacOperationCode op_code);

/**
 * @brief Проверяет, являются ли оба аргумента инструкции константами.
 *
 * @param instr Указатель на TAC инструкцию.
 * @return true если оба аргумента константы, иначе false.
 */
bool is_argrs_constant(TacInstruction *instr);


/**
 * @brief Проверяет, является ли число целым и ставит подходящий тип в TacConstant.
 *
 * @param result_const Указатель на TacConstant для установки типа и значения.
 * @param result_value Вычисленное значение.
 */
void set_num_constant_value(TacConstant *result_const, float result_value);

/**
 * @brief Конкатенирует две строковые константы из инструкции TAC.
 *
 * @param instr Указатель на TAC инструкцию с операцией конкатенации.
 * @return Указатель на новую строку, содержащую результат конкатенации.
 *         Память выделяется в куче и должна быть освобождена вызывающей стороной.
 */
char *concat_string_constants(TacInstruction *instr);

/**
 * @brief Вычисляет результат арифметической операции с числовыми константами.
 *
 * @param instr Указатель на TAC инструкцию с арифметической операцией.
 * @param args_type Тип аргументов (TYPE_NUM или TYPE_FLOAT).
 * @return Вычисленное числовое значение.
 */
float calculate_num_constant(TacInstruction *instr);

/**
 * @brief Проверяет, является ли float целым.
 *
 * @param value Число для проверки.
 * @return true если число целое, иначе false.
 */
bool check_whole_number(float value);

/**
 * @brief Умножает строковую константу на числовую константу из инструкции TAC.
 *
 * @param instr Указатель на TAC инструкцию с операцией умножения строки на число.
 * @return Указатель на новую строку, содержащую результат умножения.
 *         Память выделяется в куче и должна быть освобождена вызывающей стороной.
 */
char *multiply_string_constant(TacInstruction *instr);

/* ====================================== */
/* ===== Имплементация приватных функций ===== */
/* ====================================== */



/* ====================*/
/* ===== Constant folding ===== */
/* ====================*/

char *multiply_string_constant(TacInstruction *instr) {
    // Извлекаем строковое значение и количество повторений
    // Ожидается, что arg1 - строка, arg2 - число
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
        // Некорректный тип для умножения строки
        return NULL;
    }

    if (repeat_count < 0) {
        // Отрицательное количество повторений не имеет смысла
        return NULL;
    }

    // Выделяем память для новой строки
    size_t str_length = strlen(str_const.value.str_value);
    size_t new_length = str_length * repeat_count + 1;

    char *result_str = (char *)malloc(new_length);
    if (result_str == NULL) {
        // Ошибка памяти
        return NULL;
    }

    result_str[0] = '\0'; // Результат - пустая строка ""
    // Эффективно копируем строку N раз
    char *current_pos = result_str;
    for (int i = 0; i < repeat_count; i++) {
        memcpy(current_pos, str_const.value.str_value, str_length);
        current_pos += str_length;
    }
    *current_pos = '\0'; // Ставим завершающий ноль
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
    // Извлекаем строковые значения
    TacConstant arg1_const = instr->arg1->data.constant;
    TacConstant arg2_const = instr->arg2->data.constant;

    // Выделяем память для новой строки
    size_t new_length = strlen(arg1_const.value.str_value) + strlen(arg2_const.value.str_value) + 1;
    char *result_str = (char *)malloc(new_length);
    if (result_str == NULL) {
        // Ошибка памяти
        //! Обработать ошибку выделения памяти подходящим образом
        return NULL;
    }

    // Конкатенируем строки
    strcpy(result_str, arg1_const.value.str_value);
    strcat(result_str, arg2_const.value.str_value);

    return result_str;
}

float calculate_num_constant(TacInstruction *instr) {
    // Извлекаем константные значения
    TacConstant arg1_const = instr->arg1->data.constant;
    TacConstant arg2_const = instr->arg2->data.constant;

    // Приводим к float для вычислений
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
        // Проверка деления на ноль
        if (arg2_value == 0.0f) {
            // Ошибка деления на ноль
            //! Обработать ошибку деления на ноль подходящим образом
            return nanf("");
        }
        result = arg1_value / arg2_value;
        break;
    default:
        // Неизвестная операция
        //! Возможно стоит добавить обработку ошибок или логирование
        return nanf("");
        break;
    }
    return result;
}

bool is_instruction_arithmetic(TacOperationCode op_code) {
    return (op_code == OP_ADD ||
        op_code == OP_SUBTRACT ||
        op_code == OP_DIVIDE ||
        op_code == OP_MULTIPLY);
}

bool is_argrs_constant(TacInstruction *instr) {
    return (instr->arg1->type == OPERAND_TYPE_CONSTANT &&
        instr->arg2->type == OPERAND_TYPE_CONSTANT);
}

void constant_folding(DLList *tac_list) {
    DLL_First(tac_list);
    while (DLL_IsActive(tac_list)) {
        TacInstruction *instr = (TacInstruction *)tac_list->active_element->data;

        // Проверяем, является ли операция арифметической
        if (!is_instruction_arithmetic(instr->operation_code)) {
            DLL_Next(tac_list);
            continue;
        }
        // Проверяем, что оба аргумента - константы
        if (!is_argrs_constant(instr)) {
            DLL_Next(tac_list);
            continue;
        }

        bool optimized = false;
        TacConstant result_const;

        TacConstant arg1_const = instr->arg1->data.constant;
        TacConstant arg2_const = instr->arg2->data.constant;
        // Оба аргумента - константы, выполняем вычисление
        // Проверяем тип констант

        // =====
        // Случай: "a" + "b"
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
        // Случай: "a" * 5 или "a" * 5.0
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
        // Случай: 5 * 5 или 5.0 + 3.2 и т.д.
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
        // Если оптимизация выполнена, обновляем инструкцию
        // =====
        if (optimized) {
            // Освобождаем старые операнды
            free_operand(instr->arg1);
            free_operand(instr->arg2);


            // Создаем новый операнд для результата
            Operand *result_op = create_constant_operand(result_const);

            // Обновляем инструкцию на присваивание
            instr->operation_code = OP_ASSIGN;
            instr->arg1 = result_op;
            instr->arg2 = NULL;

        }

        // Переходим к следующей инструкции
        DLL_Next(tac_list);


    }
}

/* ======================================*/
/* ===== Имплементация публичных функций =====*/
/* ======================================*/

void optimize_tac(DLList *tac_list) {
    // Вызов оптимизации Constant Folding
    constant_folding(tac_list);
}