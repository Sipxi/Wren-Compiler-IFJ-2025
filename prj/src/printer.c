/**
 * @file printer.c
 * * @brief Реализация печати
 * * Author:
 * - Serhij Čepil (253038)
 */


#include "printer.h"
#include "tac.h" // Нужен для TacInstruction, Operand, и т.д.
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Нам понадобится sprintf

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
 * @brief Рекурсивная функция для печати AST с отступами.
 * @param node Текущий узел для печати.
 * @param indent_level Уровень отступа (глубина в дереве).
 */
static void ast_print_recursive(AstNode* node, int indent_level);

/**
 * @brief Вспомогательная функция для получения строкового представления NodeType.
 * @param type Тип узла AST.
 * @return Строковое представление типа узла.
 */
static const char* node_type_to_string(NodeType type);

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



static const char* node_type_to_string(NodeType type)
{
    switch (type) {
        case NODE_PROGRAM: return "NODE_PROGRAM";
        case NODE_FUNCTION_DEF: return "NODE_FUNCTION_DEF";
        case NODE_SETTER_DEF: return "NODE_SETTER_DEF";
        case NODE_GETTER_DEF: return "NODE_GETTER_DEF";
        case NODE_PARAM_LIST: return "NODE_PARAM_LIST";
        case NODE_PARAM: return "NODE_PARAM";
        case NODE_BLOCK: return "NODE_BLOCK";
        case NODE_IF: return "NODE_IF";
        case NODE_WHILE: return "NODE_WHILE";
        case NODE_VAR_DEF: return "NODE_VAR_DEF";
        case NODE_RETURN: return "NODE_RETURN";
        case NODE_ASSIGNMENT: return "NODE_ASSIGNMENT";
        case NODE_CALL_STATEMENT: return "NODE_CALL_STATEMENT";
        case NODE_ARGUMENT_LIST: return "NODE_ARGUMENT_LIST";
        case NODE_OP_PLUS: return "NODE_OP_PLUS (+)";
        case NODE_OP_MINUS: return "NODE_OP_MINUS (-)";
        case NODE_OP_MUL: return "NODE_OP_MUL (*)";
        case NODE_OP_DIV: return "NODE_OP_DIV (/)";
        case NODE_OP_LT: return "NODE_OP_LT (<)";
        case NODE_OP_GT: return "NODE_OP_GT (>)";
        case NODE_OP_LTE: return "NODE_OP_LTE (<=)";
        case NODE_OP_GTE: return "NODE_OP_GTE (>=)";
        case NODE_OP_EQ: return "NODE_OP_EQ (==)";
        case NODE_OP_NEQ: return "NODE_OP_NEQ (!=)";
        case NODE_OP_IS: return "NODE_OP_IS (is)";
        case NODE_ID: return "NODE_ID";
        case NODE_LITERAL_NUM: return "NODE_LITERAL_NUM";
        case NODE_LITERAL_STRING: return "NODE_LITERAL_STRING";
        case NODE_LITERAL_NULL: return "NODE_LITERAL_NULL";
        case NODE_TYPE_NAME: return "NODE_TYPE_NAME";
        default: return "UNKNOWN_NODE";
    }
}


static void ast_print_recursive(AstNode* node, int indent_level) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < indent_level; ++i) {
        printf("  ");
    }
    if (node->type == NODE_ID) {
        printf("%s: {%s} (line %d)\n", node_type_to_string(node->type), node->data.identifier, node->line_number);
    } else if (node->type == NODE_LITERAL_NUM) {
        printf("%s: {%g} (line %d)\n", node_type_to_string(node->type), node->data.literal_num, node->line_number);
    } else if (node->type == NODE_LITERAL_STRING) {
        printf("%s: \"%s\" (line %d)\n", node_type_to_string(node->type), node->data.literal_string, node->line_number);
    } else {
        printf("%s (line %d)\n", node_type_to_string(node->type), node->line_number);
    }
    AstNode* child = node->child;
    while (child != NULL) {
        ast_print_recursive(child, indent_level + 1);
        child = child->sibling;
    }
}



/*=======================================*/
/* ===== Реализация публичных функций =====*/
/*=======================================*/


void ast_print_debug(AstNode* node)
{
    printf("--- [ AST DEBUG PRINT ] ---\n");
    ast_print_recursive(node, 0);
    printf("---------------------------\n");
}

void print_single_tac_instruction(TacInstruction *instr){

    const int op_width = 18;
    const int col_width = 15; // Ширина для столбцов RESULT, ARG1
    
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
}

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

        print_single_tac_instruction(instr);

        TACDLL_Next(tac_list);
        count++;
    }
    printf("--------------------+-----------------+-----------------+-----------------\n");
    printf("Total instructions: %d\n", count);
}