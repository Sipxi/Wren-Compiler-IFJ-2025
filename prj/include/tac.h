/** 
 * @file tac.h
 * @brief Заголовочный файл для трехадресного кода (Three Address Code).
 * 
 * Author:
 *      - Serhij Čepil (253038)
 */

#ifndef TAC_H
#define TAC_H

#include "symtable.h"
#include "ast.h"
#include "dll.h"
#include "error_codes.h"


/* ======================================*/
/* ===== Енумы =====*/
/* ======================================*/


/**
 * @brief Енум для кодов операций трехадресного кода.
 * 
 * Этот енум определяет все возможные операции
 * в трехадресном коде.
 */
typedef enum{
    /* Операции для управления потоком */
    OP_JUMP, // Безусловный переход например: JUMP L1
    OP_JUMP_IF_FALSE, // Условный переход например: JUMP_IF_FALSE arg1 L1
    OP_LABEL, // Метка (адресат для JUMP) например: LABEL L1
    
    /* Арифметические операции */
    OP_ADD, // Сложение например: ADD arg1 arg2
    OP_SUBTRACT, // Вычитание например: SUBTRACT arg1 arg2
    OP_MULTIPLY, // Умножение например: MULTIPLY arg1 arg2
    OP_DIVIDE, // Деление например: DIVIDE arg1 arg2

    
    /* Операции конкатенации строк */
    OP_CONCAT, // Конкатенация строк например:  
    OP_MULTIPLY_STRING, // Умножение строки например: MULTIPLY_STRING arg1 arg2
    
    /* Операции сравнения */
    OP_LESS, // Меньше например: OP_LESS arg1 arg2
    OP_GREATER, // Больше например: OP_GREATER arg1 arg2
    OP_LESS_EQUAL, // Меньше или равно например: OP_LESS_EQUAL arg1 arg2
    OP_GREATER_EQUAL, // Больше или равно например: OP_GREATER_EQUAL arg1 arg2
    OP_EQUAL, // Равно например: OP_EQUAL arg1 arg2
    OP_NOT_EQUAL, // Не равно например: OP_NOT_EQUAL arg1 arg2
    
    /* Логические операции */
    OP_IS,  // Оператор 'is' например: IS arg1 arg2

    /* Присваивание */
    OP_ASSIGN, // Присваивание например: ASSIGN arg1 result
    OP_DECLARE, // Объявление переменной например: DECLARE result

    /* Операции работы с функциями */
    OP_CALL, // Вызов функции например: CALL arg1 (где arg1 - имя функции)
    OP_RETURN, // Возврат из функции например: RETURN arg1
    OP_PARAM, // Передача параметра функции например: PARAM arg1
    OP_FUNCTION_BEGIN, // Начало функции например: FUNCTION_BEGIN arg1 (где arg1 - имя функции)
    OP_FUNCTION_END, // Конец функции например: FUNCTION_END arg1 (где arg1 - имя функции)

} TacOperationCode;

/**
 * @brief Типы операндов в трехадресном коде.
 * 
 * @note Каждый операнд может быть одного из этих типов.
 * @note Например, это может быть символ, константа или метка.
 * @note Если операнд не используется, его тип будет OPERAND_TYPE_EMPTY.
 * 
 */
typedef enum{
    OPERAND_TYPE_EMPTY,    // Пустой операнд (не используется)
    OPERAND_TYPE_SYMBOL,   // Символ из symtable ('a', 'b')
    OPERAND_TYPE_CONSTANT, // Константа (10, "hello")
    OPERAND_TYPE_LABEL,    // Метка ('L1', 'L_ELSE')
    OPERAND_TYPE_TEMP      // Временная переменная ('$t1', '$t2')
} OperandType;


/* ======================================*/
/* ===== Структуры =====*/
/* ======================================*/


/**
 * @brief Структура для представления константы в трехадресном коде.
 * 
 * В зависимости от типа константы,
 * значение хранится в соответствующем поле объединения.
 * 
 * @note Поле 'type' указывает на тип данных константы.
 * @note Например если константа используется эта структура может указывать на тип INT, FLOAT и т.д.
 *  
 */
typedef struct {
    DataType type;
    union {
        int int_value;
        float float_value; // Для IFJ25 это будет double
        char *str_value;
    } value;
} TacConstant;

/**
 * @brief Структура для представления операнда в трехадресном коде.
 * 
 * В зависимости от типа операнда,
 * соответствующее поле объединения содержит данные.
 * 
 * @note Если это просто символ, то хранится указатель на запись таблицы символов.
 * @note Если это константа, то хранится структура TacConstant.
 * @note Если это метка, то хранится строка с именем метки.
 * @note Если это временная переменная, то хранится уникальный идентификатор.
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
 * @brief Структура для представления инструкции трехадресного кода.
 * 
 * Каждая инструкция содержит код операции и до трех операндов.
 * Результат операции также хранится в отдельном операнде.
 * 
 * @note Если операнд не используется, он будет NULL.
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

/**
 * @brief Главная функция для генерации трехадресного кода (3AC) из AST.
 * 
 * Эта публичная функция запускает рекурсивную генерацию 3AC.
 * 
 * @param ast_node Корневой узел AST.
 * @param tac_list Список для хранения сгенерированных инструкций 3AC.
 * @param global_table Глобальная таблица символов.
 * 
 * @note Эта функция инициализирует глобальные счетчики для временных переменных и меток.
 * Сделано для поддержки повторных вызовов.
 */
void generate_tac(AstNode *ast_node, DLList *tac_list, Symtable *global_table);

/**
 * @brief Освобождает память, занятую списком трехадресного кода.
 * 
 * @param tac_list Список инструкций для освобождения.
 */
void free_tac(DLList *tac_list);

/**
 * @brief Печатает содержимое списка 3AC в читаемом виде.
 * @param tac_list Заполненный список инструкций.
 */
void print_tac_list(DLList *tac_list);

/**
 * ! Временная функция ошибки
 * @brief Выводит "красивую" ошибку 3AC-генератора и завершает программу.
 * @param message Описание ошибки (e.g., "Unimplemented NodeType")
 * @param line_number Номер строки из AST-узла (для контекста)
 * @param error_code Код ошибки (для exit())
 */
void raise_tac_error(const char *message, int line_number, ErrorCode error_code);

#endif // TAC_H
