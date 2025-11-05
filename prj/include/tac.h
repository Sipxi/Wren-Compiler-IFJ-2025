/* tac.h
 * Трехадресный код (Three Address Code) представление
 * Автор: Serhij Čepil (253038)
 */

#ifndef TAC_H
#define TAC_H

#include "symtable.h"
#include "ast.h"
#include "common.h"


/* ======================================*/
/* ===== Енумы =====*/
/* ======================================*/


/**
 * Этот енум определяет все возможные операции
 * в трехадресном коде.
 */
typedef enum{
    // Операции для управления потоком
    OP_JUMP,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,
    OP_LABEL, // Метка (адресат для JUMP)

    // Арифметические операции
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,

    // Операции сравнения
    OP_LESS,
    OP_GREATER,
    OP_LESS_EQUAL,
    OP_GREATER_EQUAL,
    OP_EQUAL,
    OP_NOT_EQUAL,

    // Присваивание
    OP_ASSIGN, // result = arg1

    // Операции работы с функциями
    OP_CALL,
    OP_RETURN,
    OP_PARAM,
    OP_FUNCTION_BEGIN,
    OP_FUNCTION_END,

    // TODO: Добавить другие операции по мере необходимости
    
} TacOperationCode;

/**
 * Типы операндов в трехадресном коде.
 * Каждый операнд может быть одного из этих типов.
 * Например, это может быть символ, константа или метка.
 * Если операнд не используется, его тип будет OPERAND_TYPE_EMPTY.
 * 
 */
typedef enum{
    OPERAND_TYPE_EMPTY,    // Пустой операнд (не используется)
    OPERAND_TYPE_SYMBOL,   // Символ из symtable (e.g., 'a', 'b')
    OPERAND_TYPE_CONSTANT, // Константа (e.g., 10, "hello")
    OPERAND_TYPE_LABEL,    // Метка (e.g., 'L1', 'L_ELSE')
    OPERAND_TYPE_TEMP      // Временная переменная (e.g., '$t1', '$t2')
} OperandType;

/* ======================================*/
/* ===== Структуры =====*/
/* ======================================*/


/**
 * Структура для представления константы в трехадресном коде.
 * В зависимости от типа константы,
 * значение хранится в соответствующем поле объединения.
 * @note Поле 'type' указывает на тип данных константы.
 * @note Используется предварительное объявление структуры Datatype.
 * Например если константа используется эта структура может указывать на тип INT, FLOAT и т.д.
 *  
 */
typedef struct {
    DataType type;
    union {
        int int_value;
        float float_value; // Для IFJ25 это будет double
        char* str_value;
    } value;
} TacConstant;

/**
 * Структура для представления операнда в трехадресном коде.
 * В зависимости от типа операнда,
 * соответствующее поле объединения содержит данные.
 * Если это просто символ, то хранится указатель на запись таблицы символов.
 * Если это константа, то хранится структура TacConstant.
 * Если это метка, то хранится строка с именем метки.
 */
typedef struct{
    OperandType type;
    union {
        TableEntry *symbol_entry; // Если тип SYMBOL
        TacConstant constant;     // Если тип CONSTANT
        char *label_name;         // Если тип LABEL
        int temp_id;              // Если тип TEMP
    } data;
} Operand;
/**
 * Структура для представления инструкции трехадресного кода.
 * Каждая инструкция содержит код операции и до трех операндов.
 */
typedef struct{
    TacOperationCode operation_code;
    Operand *arg1;
    Operand *arg2;
    Operand *result;
} TacInstruction;


/* ======================================*/
/* ===== Прототипы функций =====*/
/* ======================================*/

void generate_tac(AstNode *ast_node, DLList *tac_list, Symtable *global_table);

void free_tac(DLList *tac_list);

/**
 * @brief Печатает содержимое списка 3AC в читаемом виде.
 * * @param tac_list Заполненный список инструкций.
 */
void print_tac_list(DLList *tac_list);

#endif // TAC_H