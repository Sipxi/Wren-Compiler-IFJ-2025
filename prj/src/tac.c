    /**
 * @file tac.c
 *
 * @brief Имплементация генерации трехадресного кода (TAC)
 *
 * Author:
 *     - Serhij Čepil (253038)
 *
 * TODO Протестировать все конструкции языка
 * TODO Добавить обработку ошибок связанных с семантикой
 * ? @Bulcer Будешь ли ты ловить все семантические ошибки в семантическом анализаторе?
 * ? Или часть из них будет ловиться здесь, в генераторе 3AC?
 *
 * TODO Очистка памяти
 * TODO Проверить на утечки памяти
 * TODO label_counter тоже самое
 *
 *
 */

#include "tac.h"
#include "utils.h"
#include "error_codes.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 /* ======================================*/
 /* ===== Глобальные переменные =====*/
 /* ======================================*/

int global_temp_counter = 0;
int global_label_counter = 0;

/* ======================================*/
/* ===== Прототипы приватных функций =====*/
/* ======================================*/



/**
 * @brief Главная рекурсивная функция. Обходит AST и генерирует TAC.
 *
 * Функция рекурсивно обходит дерево (post-order для выражений)
 *
 * @param node Текущий узел AST для обработки.
 * @param tac_list Список TAC инструкций для добавления новых инструкций.
 * @param symtable Таблица символов (в основном для 'read-only' доступа).
 *
 * @return Возвращает 'Operand*', в котором лежит результат выражения ($t1 или
константа 5).
 * @note Для стейтментов (if, var def, block) возвращает NULL, так как они не
 * производят "значения", а только генерируют инструкции в 'tac_list'.
 *
 * @example
 * .// Исходный код:
 * static main() {
 *   if (a < 10) {
 *     b = a + 1
 *   }
 * }
 *
 * // Сгенерированный 3AC (квадрупли):
 * // (OPCODE, RESULT, ARG1, ARG2)
 * LABEL,          ____,      main,   ____
 * FUNC_BEGIN,      ____,      main,   ____
 * // 1. tac_gen_recursive(NODE_OP_LT) возвращает '$t0'
 * LESS,           $t0,         a,     10
 * // 2. tac_gen_recursive(NODE_IF) генерирует JUMP и метки
 * JUMP_IF_FALSE,   ____,       $t0,    L_ENDIF_0
 * // 3. tac_gen_recursive(NODE_OP_PLUS) возвращает '$t1'
 * ADD,             $t1,         a,     1
 * // 4. tac_gen_recursive(NODE_ASSIGNMENT) использует '$t1'
 * ASSIGN,           b,         $t1,   ____
 * // 5. Конец NODE_IF
 * LABEL,          ____,      L_ENDIF_0, ____
 * FUNC_END,        ____,      main,   ____
 *  */
static Operand *tac_gen_recursive(AstNode *node, TACDLList *tac_list,
    Symtable *symtable);

/**
 * @brief Создает операнд для символа (переменной) из таблицы символов.
 *
 * Это фабричная функция, которая создает и выделяет память под операнд
 *
 * @note Устанавливает тип OPERAND_TYPE_SYMBOL
 * @note Устанавливает data.symbol_entry в предоставленную запись таблицы
 * символов.
 * @param entry Запись таблицы символов.
 * @return Указатель на созданный операнд.
 */
static Operand *create_symbol_operand(TableEntry *entry);

/**
 * @brief Создает пустой операнд заданного типа.
 *
 * @param type Тип создаваемого операнда.
 * @return Указатель на созданный операнд.
 * @note Вспомогательная функция.
 */
static Operand *create_operand(OperandType type);

/**
 * @brief Создаёт новый временный операнд. Например, $t1, $t2 и т.д.
 *
 * @param tac_list Список TAC инструкций, в который будет добавлена инструкция
 *
 * @note Глобальный счетчик временных переменных увеличивается при каждом вызове.
 * @note Создает инструкцию DECLARE для новой временной переменной.
 *
 * @return Указатель на созданный временный операнд.
 */
static Operand *create_temp_operand(TACDLList *tac_list);

/**
 * Создает операнд для метки с заданным именем.
 *
 * @param label_name Имя метки (например, "L_FUNCTION").
 * @return Указатель на созданный операнд.
 */
static Operand *create_label_operand(const char *label_name);

/**
 * Рекурсивно генерирует TAC для всех детей узла.
 *
 * @param node Узел AST, чьих детей нужно обработать.
 * @param tac_list Список TAC инструкций для добавления новых инструкций.
 * @param symtable Таблица символов.
 * @return void
 */
static void tac_gen_children_list(AstNode *node, TACDLList *tac_list,
    Symtable *symtable);

/**
 * Создание инструкции и добавление ее в конец списка.
 *
 * @param list Список инструкций.
 * @param op Код операции.
 * @param res Результирующий операнд.
 * @param arg1 Первый аргумент.
 * @param arg2 Второй аргумент.
 * @return void
 */
static void generate_instruction(TACDLList *list, TacOperationCode op,
    Operand *res, Operand *arg1, Operand *arg2);


    
/**
 * @brief Выводит "красивую" ошибку 3AC-генератора и завершает программу.
 * @param message Описание ошибки (e.g., "Unimplemented NodeType")
 * @param line_number Номер строки из AST-узла (для контекста)
 * @param error_code Код ошибки (для exit())
 */
void raise_tac_error(const char *message, ErrorCode error_code);


/* ======================================*/
/* ===== Имплементация приватных функций TACDLList =====*/
/* ======================================*/

void TACDLL_Error(void) {
    fprintf(stderr, "TACDLList: Chyba alokace paměti\n");
    exit(INTERNAL_ERROR);
}

/* ======================================*/
/* ===== Имплементация публичных функций TACDLList =====*/
/* ======================================*/

void TACDLL_Init(TACDLList *list) {
    list->first_element = NULL;
    list->active_element = NULL;
    list->last_element = NULL;
}

void TACDLL_Dispose(TACDLList *list) {

    TACDLLElementPtr tmp = list->first_element;
    // Помощный флаг для обхода списка
    TACDLLElementPtr next;
    // Освобождает память всех элементов
    while (tmp != NULL) {
        free(tmp->tac_instruction);
        next = tmp->next_element;
        free(tmp);
        tmp = next;
    }
    // Инициализирует список заново
    TACDLL_Init(list);
}

void TACDLL_InsertFirst(TACDLList *list, TacInstruction *tac_instruction) {
    TACDLLElementPtr newElement = (TACDLLElementPtr)malloc(sizeof(struct TACDLLElement));
    if (newElement == NULL) {
        TACDLL_Error();
    }

    // Начинаем заполнять новый элемент
    newElement->tac_instruction = tac_instruction;
    // Новый элемент будет первым, не имеет предшественника
    newElement->prev_element = NULL;
    // Новый элемент будет указывать на бывший первый элемент
    newElement->next_element = list->first_element;
    // Если список не был пустым, устанавливаем предшественника бывшего первого элемента на новый элемент
    if (list->first_element != NULL)
        list->first_element->prev_element = newElement;
    // Если список был пустым, устанавливаем также указатель на последний элемент
    else
        list->last_element = newElement;
    list->first_element = newElement;
}

void TACDLL_InsertLast(TACDLList *list, TacInstruction *tac_intruction) {
    TACDLLElementPtr newElement = (TACDLLElementPtr)malloc(sizeof(struct TACDLLElement));
    if (newElement == NULL) {
        TACDLL_Error();
    }
    newElement->tac_instruction = tac_intruction;
    newElement->next_element = NULL;
    newElement->prev_element = list->last_element;
    // Если список не был пустым, устанавливаем указатель на следующий элемент последнего элемента на новый элемент
    if (list->last_element != NULL)
        list->last_element->next_element = newElement;
    // Если список был пустым, устанавливаем также указатель на первый элемент
    else
        list->first_element = newElement;
    list->last_element = newElement;
}

void TACDLL_First(TACDLList *list) {
    list->active_element = list->first_element;
}

void TACDLL_Last(TACDLList *list) {
    list->active_element = list->last_element;
}

void TACDLL_GetFirst(TACDLList *list, TacInstruction **tac_instruction_ptr) {
    if (list->first_element != NULL)
        *tac_instruction_ptr = list->first_element->tac_instruction;
    else
        TACDLL_Error();
}

void TACDLL_GetLast(TACDLList *list, TacInstruction **tac_instruction_ptr) {
    if (list->last_element != NULL)
        *tac_instruction_ptr = list->last_element->tac_instruction;
    else
        TACDLL_Error();
}

void TACDLL_DeleteFirst(TACDLList *list) {
    if (list->first_element == NULL) return; // Список пуст, ничего не делаем
    // Если первый элемент был активным, деактивируем список
    if (list->active_element == list->first_element)
        list->active_element = NULL;

    // Вспомогательная переменная для освобождения элемента
    TACDLLElementPtr tmp = list->first_element;
    // Сдвигаем указатель на первый элемент на следующий элемент
    list->first_element = tmp->next_element;
    // Если новый первый элемент существует, устанавливаем его предыдущий элемент в NULL
    if (list->first_element != NULL)
        list->first_element->prev_element = NULL;
    // Если список состоял из одного элемента, также устанавливаем указатель на последний элемент в NULL
    else
        list->last_element = NULL;
    free(tmp->tac_instruction);
    // Освобождаем память исходного первого элемента
    free(tmp);
}

void TACDLL_DeleteLast(TACDLList *list) {
    // Список пуст, ничего не делаем
    if (list->last_element == NULL) return;
    // Если последний элемент был активным, деактивируем список
    if (list->active_element == list->last_element)
        list->active_element = NULL;

    // Вспомогательная переменная для освобождения элемента
    TACDLLElementPtr tmp = list->last_element;
    list->last_element = tmp->prev_element;
    // Если новый последний элемент существует, устанавливаем его следующий элемент в NULL
    if (list->last_element != NULL)
        list->last_element->next_element = NULL;
    // Если список состоял из одного элемента, также устанавливаем указатель на первый элемент в NULL
    else
        list->first_element = NULL;
    free(tmp->tac_instruction);
    free(tmp);
}

void TACDLL_DeleteAfter(TACDLList *list) {
    // Вспомогательная переменная для сокращения записи
    TACDLLElementPtr active_element = list->active_element;

    // Если список не активен или активный элемент последний, ничего не делаем
    if (active_element == NULL || active_element->next_element == NULL)
        return;

    // Элемент после активного элемента
    TACDLLElementPtr afterActiveElememt = active_element->next_element;
    // Пропускаем элемент после активного
    active_element->next_element = afterActiveElememt->next_element;
    // Если элемент после активного не последний, устанавливаем его предыдущий элемент на активный элемент
    if (active_element->next_element != NULL)
        active_element->next_element->prev_element = active_element;
    // Если элемент после активного был последним, обновляем указатель на последний элемент
    else
        list->last_element = active_element;
    free(afterActiveElememt->tac_instruction);
    free(afterActiveElememt);
}

void TACDLL_DeleteBefore(TACDLList *list) {
    // Вспомогательная переменная для сокращения записи
    TACDLLElementPtr active_element = list->active_element;

    if (active_element == NULL || active_element->prev_element == NULL)
        return;

    // Элемент перед активным элементом
    TACDLLElementPtr beforeActiveElememt = active_element->prev_element;
    // Пропускаем элемент перед активным
    active_element->prev_element = beforeActiveElememt->prev_element;
    // Если элемент перед активным не первый, устанавливаем его следующий элемент на активный элемент
    if (active_element->prev_element != NULL)
        active_element->prev_element->next_element = active_element;
    // Если элемент перед активным был первым, обновляем указатель на первый элемент
    else
        list->first_element = active_element;
    free(beforeActiveElememt->tac_instruction);
    free(beforeActiveElememt);
}

void TACDLL_InsertAfter(TACDLList *list, TacInstruction *tac_instruction) {
    // Вспомогательная переменная для сокращения записи
    TACDLLElementPtr active_element = list->active_element;

    if (active_element == NULL) return;
    // Указатель на новый элемент
    TACDLLElementPtr newElement = (TACDLLElementPtr)malloc(sizeof(struct TACDLLElement));
    if (newElement == NULL) {
        TACDLL_Error();
    }
    // Значение нового элемента
    newElement->tac_instruction = tac_instruction;
    // Новый элемент будет указывать на активный элемент как на предшественника
    newElement->prev_element = active_element;
    // Новый элемент будет указывать на следующий элемент активного элемента
    newElement->next_element = active_element->next_element;
    // Если активный элемент последний, обновляем указатель на последний элемент
    if (active_element->next_element == NULL)
        list->last_element = newElement;
    // Если активный элемент не последний, устанавливаем предшественника следующего элемента на новый элемент
    else
        active_element->next_element->prev_element = newElement;
    active_element->next_element = newElement; // Активный элемент будет указывать на новый элемент как на следующий
}

void TACDLL_InsertBefore(TACDLList *list, TacInstruction *tac_instruction) {
    TACDLLElementPtr active_element = list->active_element;

    if (active_element == NULL) return;

    TACDLLElementPtr newElement = (TACDLLElementPtr)malloc(sizeof(struct TACDLLElement));
    if (newElement == NULL) {
        TACDLL_Error();
    }

    newElement->tac_instruction = tac_instruction;
    newElement->next_element = active_element;
    newElement->prev_element = active_element->prev_element;
    if (active_element->prev_element == NULL)
        list->first_element = newElement;
    else
        active_element->prev_element->next_element = newElement;
    active_element->prev_element = newElement;
}

void TACDLL_GetValue(TACDLList *list, TacInstruction **tac_instruction_ptr) {
    if (list->active_element == NULL) {
        TACDLL_Error();
    }
    *tac_instruction_ptr = list->active_element->tac_instruction;
}

void TACDLL_SetValue(TACDLList *list, TacInstruction *tac_instruction) {
    if (list->active_element == NULL) return;
    list->active_element->tac_instruction = tac_instruction;
}

void TACDLL_Next(TACDLList *list) {
    if (list->active_element == NULL) return;
    list->active_element = list->active_element->next_element;
}

void TACDLL_Previous(TACDLList *list) {
    if (list->active_element == NULL) return;
    list->active_element = list->active_element->prev_element;
}

bool TACDLL_IsActive(TACDLList *list) {
    return (list->active_element != NULL);
}

/* ======================================*/
/* ===== Имплементация приватных функций =====*/
/* ======================================*/

static void tac_gen_children_list(AstNode *node, TACDLList *tac_list,
    Symtable *symtable) {
    // Проходим по всем детям узла
    AstNode *current = node->child;
    while (current != NULL) {
        tac_gen_recursive(current, tac_list, symtable);
        // Переходим к следующему брату
        current = current->sibling;
    }
}

static void generate_instruction(TACDLList *list, TacOperationCode op,
    Operand *res, Operand *arg1, Operand *arg2) {
    // Выделяем память под инструкцию
    TacInstruction *instruction =
        (TacInstruction *)malloc(sizeof(TacInstruction));
    // Заполняем поля инструкции
    instruction->operation_code = op;
    instruction->result = res;
    instruction->arg1 = arg1;
    instruction->arg2 = arg2;
    // Вставляем инструкцию в конец списка
    TACDLL_InsertLast(list, instruction);
}

static Operand *create_operand(OperandType type) {
    // Выделяем память под операнд
    Operand *op = (Operand *)malloc(sizeof(Operand));
    if (!op) {
        raise_tac_error("Memory allocation failed for operand",
            INTERNAL_ERROR);
    }
    op->type = type;
    return op;
}

static Operand *create_label_operand(const char *label_name) {
    // Создаем операнд типа LABEL
    Operand *op = create_operand(OPERAND_TYPE_LABEL);
    // Копируем имя метки
    op->data.label_name = malloc(strlen(label_name) + 1);
    if (!op->data.label_name) {
        raise_tac_error("Memory allocation failed for label operand",
            INTERNAL_ERROR);
        return NULL;
    }
    // Копируем строку
    strcpy(op->data.label_name, label_name);
    return op;
}

static Operand *create_temp_operand(TACDLList *tac_list) {
    Operand *op = create_operand(OPERAND_TYPE_TEMP);
    // Присваиваем уникальный ID
    op->data.temp_id = global_temp_counter++;
    generate_instruction(tac_list, OP_DECLARE, op, NULL, NULL);
    return op;
}

static Operand *create_symbol_operand(TableEntry *entry) {
    Operand *op = create_operand(OPERAND_TYPE_SYMBOL);
    op->data.symbol_entry = entry;
    return op;
}

static Operand *tac_gen_recursive(AstNode *node, TACDLList *tac_list,
    Symtable *symtable) {
    // Базовый случай рекурсии
    // Если узел пустой, возвращаем NULL
    if (node == NULL) {
        return NULL;
    }
    /* Обработка узла AST и генерация соответствующей инструкции TAC */

    switch (node->type) {

    /* ===== Обработка структурных узлов ===== */
    // Эти узлы проходят по всем детям и генерируют инструкции для
        // каждого из них

    case NODE_PROGRAM:
    case NODE_BLOCK:
    case NODE_PARAM_LIST: {

        // Сохраняем текущее значение глобального счетчика временных
        int temp = global_temp_counter;
        global_temp_counter = 0;

        // Обработка корневого узла блока
        // Просто обрабатываем всех детей
        tac_gen_children_list(node, tac_list, symtable);
        // Восстанавливаем глобальный счетчик временных
        global_temp_counter = temp;

        // Не возвращает значения
        return NULL;
    }

    /* ===== Обработка узлов функций ===== */

    case NODE_FUNCTION_DEF: {
        // Параметры функции NODE_PARAM_LIST
        AstNode *params = node->child;
        // Тело функции NODE_BLOCK
        AstNode *body = node->child->sibling;

        // Получаем имя функции из записи таблицы символов
        TableEntry *func_entry = node->table_entry;
        // Генерируем метку функции
        // Имя функции как метка
        char *func_label = func_entry->key;

        Operand *label_op = create_label_operand(func_label);
        generate_instruction(tac_list, OP_LABEL, NULL, label_op, NULL);

        // Генерируем инструкцию начала функции, FRAME и т.д.
        Operand *func_entry_op = create_symbol_operand(func_entry);
        generate_instruction(tac_list, OP_FUNCTION_BEGIN, NULL,
            func_entry_op, NULL);

        // Генерируем код для параметров функции (NODE_PARAM_LIST)
        tac_gen_recursive(params, tac_list, symtable);

        // Генерируем код для тела функции (NODE_BLOCK)
        tac_gen_recursive(body, tac_list, symtable);

        // Делаем новый операнд для конца функции
        Operand *func_entry_op_end = create_label_operand(func_entry->key);
        // Генерируем инструкцию конца функции
        generate_instruction(tac_list, OP_FUNCTION_END, NULL,
            func_entry_op_end, NULL);
        // Определение функции не возвращает значения
        return NULL;
    }

    /* ===== Обработка узлов параметров функций ===== */

    case NODE_PARAM: {
        // Параметр функции
        // Получаем запись таблицы символов параметра
        TableEntry *param_entry = node->table_entry;
        // Генерируем инструкцию передачи параметра
        Operand *param_op = create_symbol_operand(param_entry);
        generate_instruction(tac_list, OP_PARAM, param_op, NULL, NULL);

        // Параметр не возвращает значения
        return NULL;
    }

    /* ===== Обработка узлов сеттеров ===== */

    case NODE_SETTER_DEF: {
        // Обработка узла определения сеттера

        // NODE_PARAM
        AstNode *setter_param = node->child;
        // NODE_BLOCK
        AstNode *setter_body = node->child->sibling;

        // Получаем имя сеттера из записи таблицы символов
        TableEntry *setter_entry = node->table_entry;
        // Генерируем метку начала сеттера
        char *setter_label = setter_entry->key;  // Имя сеттера как метка

        // Метка сеттера 
        Operand *setter_label_op = create_label_operand(setter_label);
        generate_instruction(tac_list, OP_LABEL, NULL, setter_label_op,
            NULL);

        // Генерируем инструкцию начала сеттера
        Operand *setter_entry_op = create_symbol_operand(setter_entry);
        generate_instruction(tac_list, OP_FUNCTION_BEGIN, NULL,
            setter_entry_op, NULL);

        // Генерируем код для параметра сеттера (NODE_PARAM)
        tac_gen_recursive(setter_param, tac_list, symtable);

        // Генерируем код для тела сеттера (NODE_BLOCK)
        tac_gen_recursive(setter_body, tac_list, symtable);

        Operand *setter_entry_op_end = create_symbol_operand(setter_entry);
        // Генерируем инструкцию конца сеттера
        generate_instruction(tac_list, OP_FUNCTION_END, NULL,
            setter_entry_op_end, NULL);
        return NULL;  // Определение сеттера не возвращает значения
    }

    /* ===== Обработка узлов геттеров ===== */

    case NODE_GETTER_DEF: {
        // Обработка узла определения геттера
        // NODE_BLOCK
        AstNode *getter_body = node->child;
        // Получаем имя геттера из записи таблицы символов
        TableEntry *getter_entry = node->table_entry;

        // Генерируем метку начала геттера
        char *getter_label = getter_entry->key;  // Имя геттера как метка
        Operand *getter_label_op = create_label_operand(getter_label);
        generate_instruction(tac_list, OP_LABEL, NULL, getter_label_op,
            NULL);

        // Генерируем инструкцию начала геттера
        Operand *getter_entry_op = create_symbol_operand(getter_entry);
        generate_instruction(tac_list, OP_FUNCTION_BEGIN, NULL,
            getter_entry_op, NULL);
        // Генерируем код для тела геттера (NODE_BLOCK)
        tac_gen_recursive(getter_body, tac_list, symtable);

        Operand *getter_entry_op_end = create_symbol_operand(getter_entry);
        // Генерируем инструкцию конца геттера
        generate_instruction(tac_list, OP_FUNCTION_END, NULL,
            getter_entry_op_end, NULL);
        return NULL;  // Определение геттера не возвращает значения
    }

    /* ===== Обработка узлов условных операторов ===== */

    case NODE_IF: {
        // Обработка узла if
        // child -> Условие (<expression>)
        AstNode *condition = node->child;
        // child->sibling -> Блок if (NODE_BLOCK)
        AstNode *if_block = node->child->sibling;
        // child->sibling->sibling -> Блок else (NODE_BLOCK) (может быть
        // NULL)
        AstNode *else_block = node->child->sibling->sibling;

        // Создаем уникальные метки для веток if и else
        char *else_label = create_unique_label("L_ELSE");
        char *end_if_label = create_unique_label("L_ENDIF");

        // Генерируем код для условия
        // Он возвращает результат в виде операнда (например, $t1) где
        // хранится true/false
        Operand *condition_result =
            tac_gen_recursive(condition, tac_list, symtable);

        // Генерируем инструкцию перехода, если условие ложно
        // Если условие ложно, прыгаем к метке else_label но если else_block
        // существует
        char *jump_label = (else_block == NULL) ? end_if_label : else_label;

        // Создаем операнд для метки прыжка в условии лжи
        Operand *jump_if_false = create_label_operand(jump_label);
        generate_instruction(tac_list, OP_JUMP_IF_FALSE, NULL, condition_result,
            jump_if_false);

        // Генерируем код для блока if
        tac_gen_recursive(if_block, tac_list, symtable);

        // Обработка блока else, если он существует
        if (else_block != NULL) {
            // Генерируем JUMP в конец, чтобы пропустить 'else' если if
            // был выполнен
            Operand *jump_if_done = create_label_operand(end_if_label);
            generate_instruction(tac_list, OP_JUMP, NULL, NULL,
                jump_if_done);

            // Генерируем метку что начался else блок:
            Operand *else_label_op = create_label_operand(else_label);
            generate_instruction(tac_list, OP_LABEL, NULL, else_label_op,
                NULL);

            // Генерируем код для блока else
            tac_gen_recursive(else_block, tac_list, symtable);
        }
        // Если елса не было, то просто генерируем метку end_if_label:
        Operand *end_if_label_op = create_label_operand(end_if_label);
        generate_instruction(tac_list, OP_LABEL, NULL, end_if_label_op,
            NULL);

        // Чистим временные метки
        free(else_label);
        free(end_if_label);

        return NULL;  // Узел if не возвращает значения
    }

    /*===== Обработка узла while =====*/

    case NODE_WHILE: {
        // Обработка узла while
        // child -> Условие (<expression>)
        AstNode *condition = node->child;
        // child->sibling -> Тело цикла (NODE_BLOCK)
        AstNode *while_body = node->child->sibling;

        // Создаем уникальные метки для начала условия и конца цикла
        char *condition_label = create_unique_label("L_WHILE_CONDITION");
        char *endwhile_label = create_unique_label("L_ENDWHILE");
        // Генерируем метку начала условия
        Operand *condition_label_op = create_label_operand(condition_label);
        generate_instruction(tac_list, OP_LABEL, NULL, condition_label_op,
            NULL);

        // Генерируем код для условия
        Operand *condition_result =
            tac_gen_recursive(condition, tac_list, symtable);

        // Генерируем инструкцию перехода, если условие ложно

        Operand *jump_if_false = create_label_operand(endwhile_label);
        generate_instruction(tac_list, OP_JUMP_IF_FALSE, NULL, condition_result,
            jump_if_false);
        // Генерируем код для тела цикла
        tac_gen_recursive(while_body, tac_list, symtable);
        // Генерируем прыжок обратно к началу условия
        Operand *jump_at_beginning = create_label_operand(condition_label);
        generate_instruction(tac_list, OP_JUMP, NULL, jump_at_beginning, NULL);
        // Генерируем метку конца цикла
        Operand *end_while_op = create_label_operand(endwhile_label);
        generate_instruction(tac_list, OP_LABEL, NULL, end_while_op, NULL);
        // Чистим временные метки
        free(condition_label);
        free(endwhile_label);

        return NULL;  // Узел while не возвращает значения
    }

    /* ===== Обработка узла определения переменной ===== */

    case NODE_VAR_DEF: {
        // NODE_VAR_DEF (для var id) ->  id = nil.
        // NODE_ASSIGNMENT (для id = 10) ->  id = 10 (и это перезапишет
        // nil).

        //  Получаем 'id' переменной из symtable
        TableEntry *var_entry = node->table_entry;

        // Создаем операнд-символ для 'id'
        // Это будет 'result' (LHS - левая часть)
        Operand *lhs_op = create_symbol_operand(var_entry);

        generate_instruction(tac_list, OP_DECLARE, lhs_op, NULL, NULL);

        return NULL;  // 'var' - это стейтмент, он не возвращает значения
    }

    /* ===== Обработка узла return ===== */

    case NODE_RETURN: {
        // Оператор return
        AstNode *return_expr = node->child;  // Выражение для возврата

        Operand *ret_op;
        if (return_expr != NULL) {
            // Генерируем код для выражения возврата
            ret_op = tac_gen_recursive(return_expr, tac_list, symtable);
        }
        else {
            // Пустой операнд для 'return;' без выражения
            ret_op = NULL;
        }

        // Генерируем инструкцию возврата   
        // Она будет либо результатом выражения, либо nil.
        generate_instruction(tac_list, OP_RETURN, ret_op, NULL, NULL);
        return NULL;  // Оператор return не возвращает значения
    }

    /* ===== Обработка узлов выражений ===== */

    case NODE_ASSIGNMENT: {
        // 'id = expression'
        // child -> Выражение (LHS)
        AstNode *lhs_node = node->child;
        // child->sibling -> Выражение (RHS)
        AstNode *expr_node = node->child->sibling;

        // Получаем 'id' переменной из symtable
        TableEntry *var_entry = lhs_node->table_entry;

        // Генерируем код для выражения (RHS)
        Operand *rhs_op = tac_gen_recursive(expr_node, tac_list, symtable);

        // Создаем операнд-символ для 'id' (LHS)
        Operand *lhs_op = create_symbol_operand(var_entry);
        // Генерируем инструкцию присваивания
        generate_instruction(tac_list, OP_ASSIGN, lhs_op, rhs_op, NULL);
        return NULL;  // Оператор присваивания не возвращает значения
    }

    /* ===== Обработка узла вызова функции ===== */

    case NODE_CALL_STATEMENT: {
        // Получаем детей 
        // Имя функции
        AstNode *func_name_node = node->child;
        // Список аргументов (NODE_ARGUMENT_LIST)
        AstNode *arg_list_node =
            node->child->sibling;

        TableEntry *func_entry = func_name_node->table_entry;
        if (func_entry == NULL) {
            // Ошибка
            return NULL;
        }

        // Генерируем инструкцию вызова функции
        Operand *func_op = create_label_operand(func_entry->key);
        generate_instruction(tac_list, OP_CALL, NULL, func_op, NULL);

        if (arg_list_node != NULL) {
            // Генерируем код для списка аргументов
            tac_gen_recursive(arg_list_node, tac_list, symtable);
        }

        return func_op;  // Возвращаем операнд функции (может быть нужен)
    }

    /* ===== Обработка узла списка аргументов ===== */

    case NODE_ARGUMENT_LIST: {
        // Берем ПЕРВЫЙ аргумент (первого ребенка NODE_ARGUMENT_LIST)
        AstNode *current_arg_expr = node->child;

        // Идем по списку братьев (sibling)
        while (current_arg_expr != NULL) {
            // Генерируем код для выражения ('a + b' -> $t0)
            Operand *arg_op =
                tac_gen_recursive(current_arg_expr, tac_list, symtable);

            // Генерируем OP_PARAM, чтобы запушить результат
            // (Предполагаем, что параметр идет в arg1)
            generate_instruction(tac_list, OP_PARAM, NULL, arg_op, NULL);

            // Переходим к следующему аргументу
            current_arg_expr = current_arg_expr->sibling;
        }

        return NULL;  // Список аргументов не возвращает значения
    }

    /* ===== Обработка узлов бинарных операций ===== */
    // Эти узлы возвращают результат выражения в виде временного операнда
    // Например, для 'a + b' вернется '$t1', в котором будет храниться
    // результат сложения.
    // Для плюс это может быть либо ADD (числа), либо CONCAT (строки)

    case NODE_OP_PLUS: {
        // Получаем детей
        AstNode *left_node = node->child;
        AstNode *right_node = node->child->sibling;

        // Рекурсивно генерируем код
        Operand *left_op = tac_gen_recursive(left_node, tac_list, symtable);
        Operand *right_op =
            tac_gen_recursive(right_node, tac_list, symtable);

        // Создаем временный результат
        Operand *result_op = create_temp_operand(tac_list);

        // Смотрим на тип, который нам дала семантика (Pass 2)
        TacOperationCode op_code;
        if (node->data_type == TYPE_STR) {
            op_code = OP_CONCAT;
        }
        else {
            // (Если это TYPE_NUM или что-то еще, считаем, что это ADD)
            op_code = OP_ADD;
        }



        // Генерируем инструкцию (то же самое)
        generate_instruction(tac_list, op_code, result_op, left_op,
            right_op);
        return result_op;
    }

    case NODE_OP_MUL: {
        // Получаем детей
        AstNode *left_node = node->child;
        AstNode *right_node = node->child->sibling;

        // Рекурсивно генерируем код
        Operand *left_op = tac_gen_recursive(left_node, tac_list, symtable);
        Operand *right_op =
            tac_gen_recursive(right_node, tac_list, symtable);
        // Создаем временный операнд для результата
        Operand *result_op = create_temp_operand(tac_list);

        // Определяем код операции
        TacOperationCode op_code;
        if (node->data_type == TYPE_STR) {
            op_code = OP_MULTIPLY_STRING;

        }
        else {
            op_code = OP_MULTIPLY;
        }

        // Генерируем инструкцию TAC
        generate_instruction(tac_list, op_code, result_op, left_op,
            right_op);

        // Возвращаем результат ($tN)
        return result_op;
    }

    /* ===== Обработка узлов операций ===== */
    // Эти узлы возвращают результат выражения в виде временного операнда
    // Так как они все похожи, мы обрабатываем их в одном кейсе
    case NODE_OP_MINUS:
    case NODE_OP_DIV:
    case NODE_OP_LT:
    case NODE_OP_GT:
    case NODE_OP_LTE:
    case NODE_OP_GTE:
    case NODE_OP_EQ:
    case NODE_OP_NEQ: {
        // Получаем детей
        AstNode *left_node = node->child;
        AstNode *right_node = node->child->sibling;

        // Рекурсивно генерируем код
        Operand *left_op = tac_gen_recursive(left_node, tac_list, symtable);
        Operand *right_op =
            tac_gen_recursive(right_node, tac_list, symtable);

        // Создаем временный операнд для результата
        Operand *result_op = create_temp_operand(tac_list);

        // Определяем код операции
        TacOperationCode op_code;
        switch (node->type) {
            // Арифметика
        case NODE_OP_MINUS: op_code = OP_SUBTRACT; break;
        case NODE_OP_MUL: op_code = OP_MULTIPLY; break;
        case NODE_OP_DIV: op_code = OP_DIVIDE; break;
            // Сравнение
        case NODE_OP_LT: op_code = OP_LESS; break;
        case NODE_OP_GT: op_code = OP_GREATER; break;
        case NODE_OP_LTE: op_code = OP_LESS_EQUAL; break;
        case NODE_OP_GTE: op_code = OP_GREATER_EQUAL; break;
        case NODE_OP_EQ: op_code = OP_EQUAL; break;
        case NODE_OP_NEQ: op_code = OP_NOT_EQUAL; break;
        default: raise_tac_error("Unexpected node type", INTERNAL_ERROR); return NULL;  // Сюда не должно попасть
        }

        // Генерируем инструкцию TAC
        generate_instruction(tac_list, op_code, result_op, left_op,
            right_op);

        // Возвращаем результат ($tN)
        return result_op;
    }

    /* ===== Обработка узла оператора "is" ===== */

    case NODE_OP_IS: {
        // Оператор "is" для проверки типа
        AstNode *expr_node = node->child;           // Выражение слева
        AstNode *type_node = node->child->sibling;  // Тип справа

        // Генерируем код для выражения
        Operand *expr_op = tac_gen_recursive(expr_node, tac_list, symtable);

        // Генерируем код для типа
        // Создаем операнд для типа (RHS)
        //    Мы берем 'data.identifier' из узла NODE_TYPE_NAME
        //    и превращаем его в *строковую константу*.

        Operand *type_op = tac_gen_recursive(type_node, tac_list, symtable);

        Operand *result_op =
            create_temp_operand(tac_list);  // Результат проверки типа $t1

        // Генерируем инструкцию
        // OP_IS, $t1, $t0, "Num"
        generate_instruction(tac_list, OP_IS, result_op, expr_op, type_op);

        // Возвращаем результат ($tN)
        return result_op;
    }

    /* ===== Обработка узлов операндов ===== */

    case NODE_ID: {
        // Переменная (идентификатор)
        TableEntry *var_entry = node->table_entry;
        if (var_entry == NULL) {
            // Ошибка: переменная не найдена в таблице символов
            return NULL;
        }
        // Создаем операнд-символ для переменной
        return create_symbol_operand(var_entry);
    }

    case NODE_LITERAL_NUM: {
        // Литерал числа
        TacConstant num_const;
        // Определяем тип числа (int или float)
        if (node->data_type == TYPE_FLOAT) {
            num_const.type = TYPE_FLOAT;
            num_const.value.float_value = node->data.literal_num;
        }
        else {
            num_const.type = TYPE_NUM;
            num_const.value.int_value = (int) node->data.literal_num;
        }
        return create_constant_operand(num_const);
    }

    case NODE_LITERAL_NULL: {
        // Литерал null
        TacConstant nil_const;
        nil_const.type = TYPE_NIL;
        return create_constant_operand(nil_const);
    }

    case NODE_LITERAL_STRING: {
        // Литерал строки
        TacConstant str_const;
        str_const.type = TYPE_STR;
        str_const.value.str_value = node->data.literal_string;
        return create_constant_operand(str_const);
    }

    case NODE_TYPE_NAME: {
        // Узел типа (например, в операторе "is")
        // Создаем операнд-константу для типа
        TacConstant type_const;
        type_const.type = TYPE_STR;
        type_const.value.str_value = node->data.identifier;  // e.g., "Num"
        return create_constant_operand(type_const);
    }

    default:
        // Если мы попали сюда, значит, мы забыли
        // реализовать какой-то NodeType в этом switch'e.
        raise_tac_error("Unimplemented AST node type in TAC generation", INTERNAL_ERROR);


    }  // --- Конец 'switch' ---

    // Эта строка НИКОГДА не выполнится, если default делает exit().
    raise_tac_error("Reached unreachable code in TAC generation", INTERNAL_ERROR);
    return NULL;
}

/* ======================================*/
/* ===== Имплементация публичных функций =====*/
/* ======================================*/

char *create_unique_label(const char *prefix) {
    // Создаем буфер для строки. 256 байт хватит с запасом
    // (Например, для L_WHILE_CONDITION_12345)
    char buffer[256];

    // Форматируем строку: "PREFIX_COUNTER"
    // sprintf работает как printf, но пишет в 'buffer'
    sprintf(buffer, "%s_%d", prefix, global_label_counter);

    global_label_counter++;

    // Копируем строку из буфера (стек) в кучу (heap)
    // strdup() = это malloc() + strcpy()
    // Мы должны вернуть строку из кучи, т.к. 'buffer' умрет
    // сразу после выхода из этой функции.
    return strdup_c99(buffer);
}

Operand *create_constant_operand(TacConstant constant) {
    // Выделяем память под сам операнд
    Operand *op = (Operand *)malloc(sizeof(Operand));
    if (op == NULL) {
        // Ошибка памяти
        raise_tac_error("Memory allocation failed for constant operand",
            INTERNAL_ERROR);
        return NULL;
    }

    op->type = OPERAND_TYPE_CONSTANT;

    // Копируем всю структуру constant
    op->data.constant = constant;

    // Если это строка, нам нужно скопировать ее содержимое в новую память.
    if (constant.type == TYPE_STR) {
        // constant.value.str_value - это указатель на строку в AST.
        // Мы не можем его просто присвоить, т.к. AST будет
        // очищен и указатель станет "висячим".
        op->data.constant.value.str_value = strdup_c99(constant.value.str_value);

        if (op->data.constant.value.str_value == NULL) {
            // Ошибка памяти при копировании строки
            free(op);
            raise_tac_error("Memory allocation failed for constant string",
                INTERNAL_ERROR);
            return NULL;
        }
    }

    // Для TYPE_NUM и TYPE_NIL ничего больше делать не надо,
    // т.к. int_value или float_value уже скопировались
    return op;
}

void generate_tac(AstNode *ast_root, TACDLList *tac_list, Symtable *global_table) {
    // Сбрасываем счетчики на случай повторного вызова
    global_temp_counter = 0;
    global_label_counter = 0;

    // Запускаем рекурсию
    tac_gen_recursive(ast_root, tac_list, global_table);
}

void raise_tac_error(const char *message,
    ErrorCode error_code) {
    // Печатаем шапку ошибки (Жирный, Красный)
    fprintf(stderr, "%s%s--- TAC GENERATION ERROR ---%s\n\n", ANSI_STYLE_BOLD,
        ANSI_COLOR_RED, ANSI_COLOR_RESET);

    // Печатаем детали (Просто Красный)
    fprintf(stderr, "%s[!] Fatal Error (Code: %d)%s\n", ANSI_COLOR_RED,
        error_code, ANSI_COLOR_RESET);
    fprintf(stderr, "    Во время 3-Адресной Код-генерации:\n\n");

    // Печатаем само сообщение об ошибке (Нормальный)
    fprintf(stderr, "    %s\n\n", message);
    // Сообщение о выходе (Тусклый)
    fprintf(stderr, "%s    Завершаем программу.%s\n", ANSI_STYLE_DIM,
        ANSI_COLOR_RESET);

    exit(error_code);  // Завершаем программу с ошибкой
}
