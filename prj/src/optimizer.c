#include "optimizer.h"
#include "tac.h"
#include <stdbool.h>

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
 * @param tac_list Список TAC инструкций для оптимизации.
 */
void constant_folding(DLList *tac_list);

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


/* ====================================== */
/* ===== Имплементация приватных функций ===== */
/* ====================================== */

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
    while (DLL_IsActive(tac_list)){
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
        // Извлекаем константные значения
        TacConstant arg1_const = instr->arg1->data.constant;
        TacConstant arg2_const = instr->arg2->data.constant;

        // Поддерживаем только числовые константы для арифметики
        //? А что с float? Добавить поддержку?
        if (arg1_const.type != TYPE_NUM || arg2_const.type != TYPE_NUM) {
            DLL_Next(tac_list);
            continue;
        }
        
        int result_value;
        // Вычисляем результат в зависимости от операции
        switch (instr->operation_code) {
            case OP_ADD:
                result_value = arg1_const.value.int_value + arg2_const.value.int_value;
                break;
            case OP_SUBTRACT:
                result_value = arg1_const.value.int_value - arg2_const.value.int_value;
                break;
            case OP_MULTIPLY:
                result_value = arg1_const.value.int_value * arg2_const.value.int_value;
                break;
            case OP_DIVIDE:
                // Проверка деления на ноль
                if (arg2_const.value.int_value == 0) {
                    DLL_Next(tac_list);
                    // Пропускаем оптимизацию
                    //? Нужно ли логировать ошибку?
                    continue; 
                }
                result_value = arg1_const.value.int_value / arg2_const.value.int_value;
                break;
            default:
                DLL_Next(tac_list);
                // Неизвестная операция, пропускаем оптимизацию
                
                continue; 
            }
        // Создаем новую константу для результата
        TacConstant result_const;
        result_const.type = TYPE_NUM;
        result_const.value.int_value = result_value;

        // Освобождаем старые операнды
        free_operand(instr->arg1);
        free_operand(instr->arg2);


        // Создаем новый операнд для результата
        Operand *result_op = create_constant_operand(result_const);

        // Обновляем инструкцию на присваивание
        instr->operation_code = OP_ASSIGN;
        instr->arg1 = result_op;
        instr->arg2 = NULL;
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