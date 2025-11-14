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
    OP_CONCAT, // Конкатенация строк например: CONCAT arg1 arg2
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


/* ====================================== */
/* ===== Структура TAC_DLL =====*/
/* ====================================== */



typedef struct TACDLLElement {
	TacInstruction *tac_instruction;
	struct TACDLLElement *prev_element;
	struct TACDLLElement *next_element;
} *TACDLLElementPtr;

typedef struct{
  TACDLLElementPtr first_element;
  TACDLLElementPtr last_element;
  TACDLLElementPtr active_element;
} TACDLList;

/* ======================================*/
/* ===== Прототипы функций TACDLL =====*/
/* ======================================*/

/**
 * @brief Внешняя функция для очистки данных, хранимых в списке.
 * Она должна быть определена где-то еще (e.g., в tac_playground.c)
 * Мы просто объявляем ее здесь, чтобы common.c мог ее использовать.
 */
void free_tac_instruction(TacInstruction *tac_instruction);


// ======================================*/
// ===== Публичные функции для работы с DLL =====*/
// ======================================*/

/**
 * @brief Инициализирует DLL
 * 
 * @param list Указательно где инициализировать лист
 */
void TACDLL_Init(TACDLList *list);


/**
 * @brief Освобождает память всех элементов в списке и так же сам список
 * 
 * @param list Указатель на инициализированную структуру двусвязного списка
 */
void TACDLL_Dispose(TACDLList *list);

/**
 * Вставляет новый элемент в начало списка list.
 * В случае, если недостаточно памяти для нового элемента при операции malloc,
 * вызывает функцию TACDLL_Error().
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 * @param tac_intructions Значение для вставки в начало списка
 */
void TACDLL_InsertFirst(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Вставляет новый элемент в конец списка list (симметричная операция к TACDLL_InsertFirst).
 * В случае, если недостаточно памяти для нового элемента при операции malloc,
 * вызывает функцию TACDLL_Error().
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 * @param tac_intructions Значение для вставки в конец списка
 */
void TACDLL_InsertLast(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Устанавливает первый элемент списка list как активный.
 * Реализуйте функцию как одну команду, не тестируя,
 * является ли список list пустым.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 */
void TACDLL_First(TACDLList *list);

/**
 * Устанавливает последний элемент списка list как активный.
 * Реализуйте функцию как одну команду, не тестируя,
 * является ли список list пустым.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 */
void TACDLL_Last(TACDLList *list);

/**
 * Через параметр tac_intructions возвращает значение первого элемента списка list.
 * Если список list пустой, вызывает функцию TACDLL_Error().
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 * @param tac_intructions Указатель на целевую переменную
 */
void TACDLL_GetFirst(TACDLList *list, TacInstruction **tac_intructions);

/**
 * Через параметр tac_intructions возвращает значение последнего элемента списка list.
 * Если список list пустой, вызывает функцию TACDLL_Error().
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 * @param tac_intructions Указатель на целевую переменную
 */
void TACDLL_GetLast(TACDLList *list, TacInstruction **tac_intructions);

/**
 * Удаляет первый элемент списка list.
 * Если первый элемент был активным, активность теряется.
 * Если список list был пустым, ничего не происходит.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 */
void TACDLL_DeleteFirst(TACDLList *list);

/**
 * Удаляет последний элемент списка list.
 * Если последний элемент был активным, активность списка теряется.
 * Если список list был пустым, ничего не происходит.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 */
void TACDLL_DeleteLast(TACDLList *list);

/**
 * Удаляет элемент списка list за активным элементом.
 * Если список list неактивен или если активный элемент
 * является последним элементом списка, ничего не происходит.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 */
void TACDLL_DeleteAfter(TACDLList *list);

/**
 * Удаляет элемент перед активным элементом списка list.
 * Если список list неактивен или если активный элемент
 * является первым элементом списка, ничего не происходит.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 */
void TACDLL_DeleteBefore(TACDLList *list);

/**
 * Вставляет элемент за активным элементом списка list.
 * Если список list не был активным, ничего не происходит.
 * В случае, если недостаточно памяти для нового элемента при операции malloc,
 * вызывает функцию TACDLL_Error().
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 * @param tac_intructions Значение для вставки в список за текущим активным элементом
 */
void TACDLL_InsertAfter(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Вставляет элемент перед активным элементом списка list.
 * Если список list не был активным, ничего не происходит.
 * В случае, если недостаточно памяти для нового элемента при операции malloc,
 * вызывает функцию TACDLL_Error().
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 * @param tac_intructions Значение для вставки в список перед текущим активным элементом
 */
void TACDLL_InsertBefore(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Через параметр tac_intructions возвращает значение активного элемента списка list.
 * Если список list не активен, вызывает функцию TACDLL_Error().
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 * @param tac_intructions Указатель на целевую переменную
 */
void TACDLL_GetValue(TACDLList *list, TacInstruction **tac_intructions);

/**
 * Перезаписывает содержимое активного элемента списка list.
 * Если список list не активен, ничего не делает.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 * @param tac_intructions Новое значение текущего активного элемента
 */
void TACDLL_SetValue(TACDLList *list, TacInstruction *tac_intructions);

/**
 * Перемещает активность на следующий элемент списка list.
 * Если список не активен, ничего не делает.
 * Обратите внимание, что при активности на последнем элементе список становится неактивным.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 */
void TACDLL_Next(TACDLList *list);

/**
 * Перемещает активность на предыдущий элемент списка list.
 * Если список не активен, ничего не делает.
 * Обратите внимание, что при активности на первом элементе список становится неактивным.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 */
void TACDLL_Previous(TACDLList *list);

/**
 * Если список list активен, возвращает ненулевое значение, иначе возвращает 0.
 * Функцию целесообразно реализовать одной командой return.
 *
 * @param list Указатель на инициализированную структуру двусвязного списка
 *
 * @returns Ненулевое значение в случае активности элемента списка, иначе ноль
 */
bool TACDLL_IsActive(TACDLList *list);

// ======================================*/
// ===== Публичные функций TAC =====*/
// ======================================*/



/**
 * @brief Создает операнд-константу.
 * @attention Если константа - это TYPE_STR, эта функция
 * создает копию строки в куче. Эта копия будет
 * освобождена, когда 'free_tac_instruction' будет
 * чистить 'Operand'.
 *
 * @param constant Структура TacConstant (может быть из AST).
 * @return Указатель на новый Operand (выделен в куче).
 */
Operand *create_constant_operand(TacConstant);


/**
 * @brief Вспомогательная функция для очистки операнда.
 * * @param op Указатель на операнд для очистки.
 * @note Освобождает память, занятую операндом и его внутренними данными.
 * */
void free_operand(Operand *op);


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
void generate_tac(AstNode *ast_node, TACDLList *tac_list, Symtable *global_table);

/**
 * @brief Освобождает память, занятую списком трехадресного кода.
 * 
 * @param tac_list Список инструкций для освобождения.
 */
void free_tac(TACDLList *tac_list);

/**
 * @brief Печатает содержимое списка 3AC в читаемом виде.
 * @param tac_list Заполненный список инструкций.
 */
void print_tac_list(TACDLList *tac_list);

/**
 * ! Временная функция ошибки
 * @brief Выводит "красивую" ошибку 3AC-генератора и завершает программу.
 * @param message Описание ошибки (e.g., "Unimplemented NodeType")
 * @param line_number Номер строки из AST-узла (для контекста)
 * @param error_code Код ошибки (для exit())
 */
void raise_tac_error(const char *message, int line_number, ErrorCode error_code);

#endif // TAC_H