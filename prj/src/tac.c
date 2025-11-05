/**
 * @file tac.c
 * 
 * Имплементация генерации трехадресного кода (TAC)
 * 
 * Автор: Serhij Čepil (253038)
 * 
 */

#include "tac.h"
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
 * Создает дубликат строки в динамически выделенной памяти.
 * Аналог стандартной функции strdup.
 * 
 * @param s Исходная строка.
 * @return Указатель на дубликат строки.
 */
static char *my_strdup(const char *s);

/**
 * Создает уникальное имя метки с заданным префиксом(L_IF1, L_ELSE2 и т.д.).
 * 
 * @param prefix Префикс для имени метки (например, "L_IF", "L_ELSE").
 * @return Уникальное имя метки в динамически выделенной памяти.
 * Caller обязан освободить память.
 */
static char *create_unique_label(const char *prefix);

/**
 * Главная рекурсивная функция. Обходит AST и генерирует TAC.
 * 
 * @return Возвращает 'Operand*', в котором лежит результат
 * выражения (e.g., $t1 или константа 5).
 * Для стейтментов (if, var def) возвращает NULL.
 */
Operand *tac_gen_recursive(AstNode *node, DLList *tac_list, Symtable *symtable);

/**
 * Создает операнд для символа (переменной) из таблицы символов.
 * Например для переменной 'a' создает операнд типа SYMBOL,
 *
 * @param entry Запись таблицы символов.
 * @return Указатель на созданный операнд.
 */
static Operand *create_symbol_operand(TableEntry *entry);

/**
 * Создает пустой операнд заданного типа.
 * Вспомогательная функция.
 * 
 * @param type Тип создаваемого операнда.
 * @return Указатель на созданный операнд.
 */
static Operand *create_operand(OperandType type);

/**
 * Создает новый временный операнд. Например, $t1, $t2 и т.д.
 * 
 * @return Указатель на созданный временный операнд.
 */
static Operand *create_temp_operand();

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
static Operand *tac_gen_children_list(AstNode *node, DLList *tac_list,
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
static void generate_instruction(DLList *list, TacOperationCode op,
                                 Operand *res, Operand *arg1, Operand *arg2);


/* ======================================*/
/* ===== Имплементация приватных функций =====*/
/* ======================================*/


static char *my_strdup(const char *s) {
    if (s == NULL) {
        return NULL;
    }

    // Получаем длину строки
    size_t len = strlen(s);

    // Выделяем память: (длина + 1) (для символа '\0')
    char *new_s = (char *)malloc(len + 1);
    if (new_s == NULL) {
        // Ошибка: не удалось выделить память
        return NULL;
    }

    // Копируем данные из старой строки в новую
    memcpy(new_s, s, len + 1); // memcpy быстрее, чем strcpy,
                              // т.к. мы уже знаем длину
    
    return new_s;
}

static Operand *tac_gen_children_list(AstNode *node, DLList *tac_list,
                                      Symtable *symtable) {
    // Проходим по всем детям узла
    AstNode *current = node->child;
    while (current != NULL) {
        tac_gen_recursive(current, tac_list, symtable);
        // Переходим к следующему брату
        current = current->sibling; 
    }
}

static void generate_instruction(DLList *list, TacOperationCode op,
                                 Operand *res, Operand *arg1, Operand *arg2) {
    TacInstruction *instruction =
        (TacInstruction *)malloc(sizeof(TacInstruction));
    instruction->operation_code = op;
    instruction->result = res;
    instruction->arg1 = arg1;
    instruction->arg2 = arg2;
    DLL_InsertLast(list, (void*)instruction);
}

static Operand *create_operand(OperandType type) {
    // Выделяем память под операнд
    Operand *op = (Operand *)malloc(sizeof(Operand));
    if (!op) {
        // TODO: Обработка ошибки выделения памяти
        return NULL;
    }
    op->type = type;
    return op;
}

static Operand *create_label_operand(const char *label_name) {
    Operand *op = create_operand(OPERAND_TYPE_LABEL);
    // Копируем имя метки
    op->data.label_name = malloc(strlen(label_name) + 1);
    if (!op->data.label_name) {
        // TODO: Обработка ошибки выделения памяти
        free(op);
        return NULL;
    }
    strcpy(op->data.label_name, label_name);
    return op;
}

static Operand *create_temp_operand() {
    Operand *op = create_operand(OPERAND_TYPE_TEMP);
    // Присваиваем уникальный ID
    op->data.temp_id = global_temp_counter++;
    return op;
}

static Operand *create_symbol_operand(TableEntry *entry) {
    Operand *op = create_operand(OPERAND_TYPE_SYMBOL);
    op->data.symbol_entry = entry;
    return op;
}

static char *create_unique_label(const char *prefix) {
    // Создаем буфер для строки. 256 байт хватит с запасом
    // (Например, для "L_WHILE_CONDITION_12345")
    char buffer[256]; 

    // Форматируем строку: "PREFIX_COUNTER"
    // sprintf работает как printf, но пишет в 'buffer'
    sprintf(buffer, "%s_%d", prefix, global_label_counter);
    
    // УВЕЛИЧИВАЕМ СЧЕТЧИК!
    global_label_counter++;
    
    // Копируем строку из буфера (стек) в кучу (heap)
    // strdup() = это malloc() + strcpy()
    // Мы должны вернуть строку из кучи, т.к. 'buffer' умрет
    // сразу после выхода из этой функции.
    return my_strdup(buffer);
}

/* ======================================*/
/* ===== Имплементация публичных функций =====*/
/* ======================================*/


Operand *tac_gen_recursive(AstNode *node, DLList *tac_list,
                           Symtable *symtable) {
    // Базовый случай рекурсии
    // Если узел пустой, возвращаем NULL
    if (node == NULL) {
        return NULL;
    }
    // Обработка узла AST и генерация соответствующей инструкции TAC

    switch (node->type) {
        case NODE_PROGRAM:
        case NODE_BLOCK:
        case NODE_PARAM_LIST:
            // Обработка корневого узла программы
            // Просто обрабатываем всех детей
            tac_gen_children_list(node, tac_list, symtable);

            return NULL;  // Не возвращает значения

        case NODE_FUNCTION_DEF:{
            // Параметры функции NODE_PARAM_LIST
            AstNode *params = node->child;
            // Тело функции NODE_BLOCK
            AstNode *body = node->child->sibling;

            // Получаем имя функции из записи таблицы символов
            TableEntry *func_entry = node->table_entry;
            if (func_entry == NULL) {
                // Ошибка: функция не найдена в таблице символов
                return NULL;
            }
            // Генерируем метку начала функции
            char *func_label = func_entry->key;  // Имя функции как метка
            // LABEL func_label:
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
            Operand *func_entry_op_end = create_symbol_operand(func_entry);
            // Генерируем инструкцию конца функции
            generate_instruction(tac_list, OP_FUNCTION_END, NULL,
                                 func_entry_op_end, NULL);
            return NULL;  // Определение функции не возвращает значения
            }
    case NODE_PARAM:{
            // Параметр функции
            // Получаем запись таблицы символов параметра
            TableEntry *param_entry = node->table_entry;
            if (param_entry == NULL) {
                // Ошибка: параметр не найден в таблице символов
                return NULL;
            }
            // Генерируем инструкцию передачи параметра
            Operand *param_op = create_symbol_operand(param_entry);
            generate_instruction(tac_list, OP_PARAM, param_op, NULL, NULL);
            return NULL;  // Параметр не возвращает значения

        case NODE_SETTER_DEF:{
            // Обработка узла определения сеттера

            // NODE_PARAM 
            AstNode *setter_param = node->child;
            // NODE_BLOCK
            AstNode *setter_body = node->child->sibling;

            // Получаем имя сеттера из записи таблицы символов
            TableEntry *setter_entry = node->table_entry;
            if (!setter_entry) {
                // Ошибка: сеттер не найден в таблице символов
                return NULL;
            }
            // Генерируем метку начала сеттера
            char *setter_label = setter_entry->key;  // Имя сеттера как метка

            // LABEL
            Operand *setter_label_op = create_label_operand(setter_label);
            generate_instruction(tac_list, OP_LABEL, NULL, setter_label_op, NULL);

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
    }
        case NODE_GETTER_DEF:{
            // Обработка узла определения геттера

            AstNode *getter_body = node->child;
            // Получаем имя геттера из записи таблицы символов
            TableEntry *getter_entry = node->table_entry;
            if (!getter_entry) {
                // Ошибка: геттер не найден в таблице символов
                return NULL;
            }
            // Генерируем метку начала геттера
            char *getter_label = getter_entry->key;  // Имя геттера как метка
            Operand *getter_label_op = create_label_operand(getter_label);
            generate_instruction(tac_list, OP_LABEL, NULL, getter_label_op, NULL);

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
        
        case NODE_IF:{
            // Обработка узла if
            // child -> Условие (<expression>)
            AstNode *condition = node->child;
            // child->sibling -> Блок if (NODE_BLOCK)
            AstNode *if_block = node->child->sibling;
            // child->sibling->sibling -> Блок else (NODE_BLOCK) (может быть NULL)
            AstNode *else_block = node->child->sibling->sibling;

            // Создаем уникальные метки для веток if и else
            char *else_label = create_unique_label("L_ELSE");   
            char *endif_label = create_unique_label("L_ENDIF");

            // Генерируем код для условия
            // Он возвращает результат в виде операнда (например, $t1) где хранится true/false
            Operand *cond_result = tac_gen_recursive(condition, tac_list, symtable);

            // Генерируем инструкцию перехода, если условие ложно
            // Если условие ложно, прыгаем к метке else_label но если else_block существует
            char *jump_label = (else_block != NULL) ? else_label : endif_label;

            // Создаем операнд для метки прыжка
            Operand *jump_label_op = create_label_operand(jump_label);
            generate_instruction(tac_list, OP_JUMP_IF_FALSE, NULL,
                                 cond_result, jump_label_op);

            // Генерируем код для блока if
            tac_gen_recursive(if_block, tac_list, symtable);

            // Обработка блока else, если он существует
            if (else_block != NULL) {
                // Генерируем JUMP в конец, чтобы пропустить 'else'
                Operand *endif_label_op = create_label_operand(endif_label);
                generate_instruction(tac_list, OP_JUMP, NULL,
                                     NULL, endif_label_op);

                // Генерируем метку else_label:
                Operand *else_label_op = create_label_operand(else_label);
                generate_instruction(tac_list, OP_LABEL, NULL,
                                     else_label_op, NULL);

                // Генерируем код для блока else
                tac_gen_recursive(else_block, tac_list, symtable);
            }
            // Если елса не было, то просто генерируем метку endif_label:
            Operand *endif_label_op = create_label_operand(endif_label);
            generate_instruction(tac_list, OP_LABEL, NULL,
                                 endif_label_op, NULL);

            // Чистим временные метки
            free(else_label);
            free(endif_label);

            return NULL;  // Узел if не возвращает значения
        }
    }


    return NULL;  //! Временно
}