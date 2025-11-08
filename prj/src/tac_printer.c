/**
 * @file tac_printer.c
 * 
 * @brief Реализация печати и очистки 3AC.
 * 
 * Author:
 *     - Serhij Čepil (253038)
 */
#include <stdio.h>
#include <stdlib.h>

#include "tac_printer.h"
#include "tac.h" // Нужен для TacInstruction, Operand, и т.д.

/*=======================================*/
/* ===== Глобальные переменные =====*/
/*=======================================*/

// Массив строк для имен операций
const char* op_code_to_string[] = {
    [OP_JUMP] = "JUMP",
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
};

/*=======================================*/
/* ===== Приватные декларации =====*/
/*=======================================*/

/**
 * @brief Вспомогательная функция для очистки операнда.
 * 
 * @param op Указатель на операнд для очистки.
 * @note Освобождает память, занятую операндом и его внутренними данными.
 * 
 */
static void free_operand(Operand *op);

/**
 * @brief Вспомогательная функция для печати операнда.
 * 
 * @param op Указатель на операнд для печати.
 */
static void print_operand(Operand *op);

/*=======================================*/
/* ===== Реализация приватных функций =====*/
/*=======================================*/

static void free_operand(Operand *op) {
    // 
    if (op == NULL) return;
    
    // Если операнд - строка ИЛИ метка, чистим 'char*'
    if (op->type == OPERAND_TYPE_LABEL) {
        free(op->data.label_name);
    } else if (op->type == OPERAND_TYPE_CONSTANT && op->data.constant.type == TYPE_STR) {
        free(op->data.constant.value.str_value);
    }
    
    // Чистим саму структуру Operand
    free(op);
}

static void print_operand(Operand *op) {
    if (op == NULL || op->type == OPERAND_TYPE_EMPTY) {
        printf("____");
        return;
    }
    switch (op->type) {
        case OPERAND_TYPE_SYMBOL:
            // Печатаем ключ (имя) из symtable
            if (op->data.symbol_entry)
                printf("%s", op->data.symbol_entry->key);
            else
                printf("ERR_SYM");
            break;
        case OPERAND_TYPE_CONSTANT:
            if (op->data.constant.type == TYPE_NUM) {
                // В IFJ25 это double, так что %g
                printf("%g", op->data.constant.value.float_value);
            } else if (op->data.constant.type == TYPE_STR) {
                printf("\"%s\"", op->data.constant.value.str_value);
            } else if (op->data.constant.type == TYPE_NIL) {
                printf("nil");
            }
            break;
        case OPERAND_TYPE_LABEL:
            printf("%s", op->data.label_name);
            break;
        case OPERAND_TYPE_TEMP:
            printf("$t%d", op->data.temp_id);
            break;
        default: 
            printf("???");
    }
}

/*=======================================*/
/* ===== Реализация публичных функций =====*/
/*=======================================*/

void free_tac_instruction(void* data) {
    if (data == NULL) return;
    
    TacInstruction *instr = (TacInstruction*)data;
    
    // Чистим все 3 операнда
    free_operand(instr->result);
    free_operand(instr->arg1);
    free_operand(instr->arg2);
    
    // Чистим саму инструкцию
    free(instr);
}

void print_tac_list(DLList *tac_list) {
    printf("\n--- Generated 3-Address Code (Quadruples) ---\n");
    printf("OPCODE\t\t | RESULT\t | ARG1\t\t | ARG2\n");
    printf("-----------------+---------------+---------------+---------------\n");

    DLL_First(tac_list);
    int count = 0;
    while (DLL_IsActive(tac_list)) {
        TacInstruction *instr;
        DLL_GetValue(tac_list, (void**)&instr);

        if (instr->operation_code == OP_LABEL) {
            // Метки печатаем отдельно для красоты
            printf("\n%s:\n", instr->arg1->data.label_name);
            DLL_Next(tac_list);
            continue;
        }

        printf("%-15s | ", op_code_to_string[instr->operation_code]);
        
        print_operand(instr->result);
        printf("\t\t | ");
        
        print_operand(instr->arg1);
        printf("\t\t | ");
        
        print_operand(instr->arg2);
        printf("\n");

        DLL_Next(tac_list);
        count++;
    }
    printf("-----------------+---------------+---------------+---------------\n");
    printf("Total instructions: %d\n", count);
}