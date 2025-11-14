/**
 * @file printer.c
 * * @brief Реализация печати
 * * Author:
 * - Serhij Čepil (253038)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Нам понадобится sprintf

#include "printer.h"
#include "tac.h" // Нужен для TacInstruction, Operand, и т.д.

/*=======================================*/
/* ===== Глобальные переменные =====*/
/*=======================================*/

// Массив строк для имен операций
const char* op_code_to_string[] = {
    [OP_JUMP] = "OP_JUMP",
    [OP_JUMP_IF_FALSE] = "JUMP_IF_FALSE",
    [OP_ADD] = "ADD",
    [OP_SUBTRACT] = "SUB",
    [OP_MULTIPLY] = "MUL",
    [OP_DIVIDE] = "DIV",
    [OP_ASSIGN] = "ASSIGN",
    [OP_LESS] = "LESS",
    [OP_GREATER] = "GT",
    [OP_LESS_EQUAL] = "LE",
    [OP_GREATER_EQUAL] = "GE",
    [OP_EQUAL] = "EQ",
    [OP_NOT_EQUAL] = "NEQ",
    [OP_LABEL] = "LABEL",
    [OP_CALL] = "CALL",
    [OP_RETURN] = "RETURN",
    [OP_PARAM] = "PARAM",
    [OP_FUNCTION_BEGIN] = "FUNC_BEGIN",
    [OP_FUNCTION_END] = "FUNC_END",
    [OP_CONCAT] = "CONCAT",
    [OP_IS] = "IS",
    [OP_DECLARE] = "DECLARE",
    [OP_MULTIPLY_STRING] = "MUL_STR",
};

/*=======================================*/
/* ===== Приватные декларации =====*/
/*=======================================*/

/**
 * @brief Вспомогательная функция для форматирования операнда в строковый буфер.
 * * @param op Указатель на операнд для форматирования.
 * @param buffer Буфер, в который будет записан отформатированный операнд.
 */
static void format_operand(Operand *op, char *buffer);

/*=======================================*/
/* ===== Реализация приватных функций =====*/
/*=======================================*/


/**
 * @brief Вспомогательная функция для форматирования операнда в строковый буфер.
 * * @param op Указатель на операнд для форматирования.
 * @param buffer Буфер, в который будет записан отформатированный операнд.
 */
static void format_operand(Operand *op, char *buffer) {
    if (op == NULL || op->type == OPERAND_TYPE_EMPTY) {
        sprintf(buffer, "____"); // Используем sprintf вместо printf
        return;
    }
    switch (op->type) {
        case OPERAND_TYPE_SYMBOL:
            // Печатаем ключ (имя) из symtable
            if (op->data.symbol_entry)
                sprintf(buffer, "%s", op->data.symbol_entry->key);
            else
                sprintf(buffer, "ERR_SYM");
            break;
        case OPERAND_TYPE_CONSTANT:
            if (op->data.constant.type == TYPE_FLOAT) {
                // В IFJ25 это double, так что %g
                sprintf(buffer, "%g", op->data.constant.value.float_value);
            } else if (op->data.constant.type == TYPE_NUM) {
                sprintf(buffer, "%d", op->data.constant.value.int_value);
            } else if (op->data.constant.type == TYPE_STR) {
                sprintf(buffer, "\"%s\"", op->data.constant.value.str_value);
            } else if (op->data.constant.type == TYPE_NIL) {
                sprintf(buffer, "nil");
            }
            break;
        case OPERAND_TYPE_LABEL:
            sprintf(buffer, "%s", op->data.label_name);
            break;
        case OPERAND_TYPE_TEMP:
            sprintf(buffer, "$t%d", op->data.temp_id);
            break;
        default: 
            sprintf(buffer, "???");
    }
}

/*=======================================*/
/* ===== Реализация публичных функций =====*/
/*=======================================*/

void free_operand(Operand *op) {
    if (op == NULL) return;
    
    // Don't free TEMP operands - they are shared between instructions
    // (result of one instruction is used as arg in another)
    // This creates a small memory leak, but prevents double-free crashes
    // TODO: Реализовать правильный подсчет ссылок для общих операндов
    if (op->type == OPERAND_TYPE_TEMP) {
        return;
    }
    
    // Free internal data for LABEL and CONSTANT types
    if (op->type == OPERAND_TYPE_LABEL) {
        free(op->data.label_name);
    } else if (op->type == OPERAND_TYPE_CONSTANT && op->data.constant.type == TYPE_STR) {
        free(op->data.constant.value.str_value);
    }
    
    // Чистим саму структуру Operand
    free(op);
}



void free_tac_instruction(TacInstruction *tac_intruction) {
    if (tac_intruction == NULL) return;
    
    
    // Чистим все 3 операнда
    free_operand(tac_intruction->result);
    free_operand(tac_intruction->arg1);
    free_operand(tac_intruction->arg2);
    
    // Чистим саму инструкцию
    free(tac_intruction);
}

void print_tac_list(TACDLList *tac_list) {
    // Определяем ширину столбцов для идеального выравнивания
    // (OP_FUNCTION_BEGIN - 16 символов, дадим 18)
    const int op_width = 18;
    const int col_width = 15; // Ширина для столбцов RESULT, ARG1

    printf("\n--- Generated 3-Address Code (Quadruples) ---\n");
    // Печатаем заголовок с учетом ширины
    printf("%-*s | %-*s | %-*s | %s\n",
           op_width, "OPCODE",
           col_width, "RESULT",
           col_width, "ARG1",
           "ARG2"); // Последний столбец не нуждается в ширине

    // Печатаем разделитель, соответствующий ширине
    // (18) + (15) + (15) + (15-ish) + (3 * " | ")
    printf("--------------------+-----------------+-----------------+-----------------\n");

    TACDLL_First(tac_list);
    int count = 0;
    while (TACDLL_IsActive(tac_list)) {
        TacInstruction *instr;
        TACDLL_GetValue(tac_list, &instr);

        if (instr->operation_code == OP_LABEL) {
            // Метки печатаем отдельно для красоты
            // (arg1 хранит имя метки)
            printf("\n%s:\n", instr->arg1->data.label_name);
            TACDLL_Next(tac_list);
            continue;
        }

        // Создаем буферы для отформатированных операндов
        char result_buf[256];
        char arg1_buf[256];
        char arg2_buf[256];

        // Заполняем буферы
        format_operand(instr->result, result_buf);
        format_operand(instr->arg1, arg1_buf);
        format_operand(instr->arg2, arg2_buf);

        // Печатаем ОДНУ строку с идеальным выравниванием
        printf("%-*s | %-*s | %-*s | %s\n",
               op_width, op_code_to_string[instr->operation_code], // OPCODE
               col_width, result_buf,                              // RESULT
               col_width, arg1_buf,                                // ARG1
               arg2_buf);                                          // ARG2

        TACDLL_Next(tac_list);
        count++;
    }
    printf("--------------------+-----------------+-----------------+-----------------\n");
    printf("Total instructions: %d\n", count);
}