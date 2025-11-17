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
#include "symtable.h"

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>


 /* ======================================*/
 /* ===== Глобальные переменные ========*/
 /* ======================================*/

bool optimization_performed;

/* ======================================*/
/* ===== Прототипы приватных функций =====*/
/* ======================================*/

/* ========================================*/
/* ===== Техники оптимизации =====*/
/* ========================================*/

 /**
  * @brief Выполняет оптимизацию "Constant Folding" на списке TAC инструкций.
  *
  * Эта функция ищет арифметические операции с константными операндами
  * и вычисляет их во время компиляции, заменяя инструкцию на
  * присваивание результата.
  * @note Настраивает глобальную переменную 'optimization_performed' в true
  * если была выполнена хотя бы одна оптимизация.
  * 
  * Например :
  * t1 = 2 + 3  ->  t1 = 5
  *
  * @param tac_list Список TAC инструкций для оптимизации.
  */
void constant_folding(TACDLList *tac_list);

/**
 * @brief Удаляет недостижимый код из списка TAC инструкций.
 *
 * Эта функция ищет инструкции, которые никогда не будут выполнены
 * (например, инструкции после безусловного перехода или возврата из функции)
 * и удаляет их из списка.
 * 
 * @note Настраивает глобальную переменную 'optimization_performed' в true
 * если была выполнена хотя бы одна оптимизация.
 *
 * @param tac_list Список TAC инструкций для оптимизации.
 */
void unreachable_code(TACDLList *tac_list);


/* ======================================*/
/* ===== Помощные функции для Constant Folding ===== */
/* ======================================*/

/**
 * @brief Проверяет, является ли операция арифметической.
 *
 * @param op_code Код операции TAC.
 * @return true если операция арифметическая, иначе false.
 */
bool can_be_folded(TacOperationCode op_code);

/**
 * @brief Проверяет, являются ли оба аргумента инструкции константами.
 *
 * @param instr Указатель на TAC инструкцию.
 * @return true если оба аргумента константы, иначе false.
 */
bool are_args_constant(TacInstruction *instr);

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



/* ====================================== */
/* ===== Имплементация помощных функций для Constant Folding ===== */
/* =========================================*/

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
/* ===== Имплементация техник оптимизации =====*/
/* ======================================*/

void constant_folding(TACDLList *tac_list) {
    TACDLL_First(tac_list);

    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        // Проверяем, является ли операция арифметической
        if (!can_be_folded(instr->operation_code)) {
            TACDLL_Next(tac_list);
            continue;
        }
        // Проверяем, что оба аргумента - константы
        if (!are_args_constant(instr)) {
            TACDLL_Next(tac_list);
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
            //* Помечаем, что была выполнена оптимизация
            optimization_performed = true;
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
        TACDLL_Next(tac_list);


    }
}

void unreachable_code(TACDLList *tac_list) {
    TACDLL_First(tac_list);

    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        // Если инструкция - безусловный переход или возврат из функции
        if (instr->operation_code == OP_JUMP ||
            instr->operation_code == OP_RETURN) {
            // После этой инструкции все следующие до метки - недостижимый код
            TACDLL_Next(tac_list);
            while (TACDLL_IsActive(tac_list)) {
                TacInstruction *next_instr;
                TACDLL_GetValue(tac_list, &next_instr);
                // Если достигнута метка, прекращаем удаление
                if (next_instr->operation_code == OP_LABEL ||
                    next_instr->operation_code == OP_FUNCTION_END) {
                    break;
                }
                // Удаляем недостижимую инструкцию
                TACDLL_Previous(tac_list);
                TACDLL_DeleteAfter(tac_list);

                //* Помечаем, что была выполнена оптимизация
                optimization_performed = true;

                TACDLL_Next(tac_list);
            }

        }
        TACDLL_Next(tac_list);
    }
}



/* ======================================*/
/* ===== Имплементация публичных функций =====*/
/* ======================================*/

void optimize_tac(TACDLList *tac_list) {
    optimization_performed = true;

    while (optimization_performed) {
        optimization_performed = false;

        // Вызов оптимизации Constant Folding
        constant_folding(tac_list);
        // Вызов оптимизации Unreachable Code
        unreachable_code(tac_list);

    }
}   