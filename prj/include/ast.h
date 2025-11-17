/**
 * ast.h
 * Абстрактное Синтаксическое Дерево (AST).
 *
 * - `child` указывает на первого ребенка узла.
 * - `sibling` указывает на следующего ребенка на том же уровне.
 */

#ifndef AST_H
#define AST_H

#include "symtable.h"

typedef enum {
    // * --- 1. Структурные Узлы (Program structure) ---
    /** * Корень всего дерева.
     * child -> первый NODE_FUNCTION_DEF
     */
    NODE_PROGRAM,

    /**
     * Обычная функция: static id (...) { ... }
     * child -> NODE_PARAM_LIST (список параметров)
     * child->sibling -> NODE_BLOCK (тело функции)
     */
    NODE_FUNCTION_DEF,

    /**
     * Сеттер: static id = (id) { ... }
     * child -> NODE_PARAM (один параметр)
     * child->sibling -> NODE_BLOCK (тело функции)
     */
    NODE_SETTER_DEF,

    /**
     * Геттер: static id { ... }
     * child -> NODE_BLOCK (тело функции)
     */
    NODE_GETTER_DEF,

    /**
     * Узел-контейнер для списка параметров (ребенок NODE_FUNCTION_DEF)
     * child -> первый NODE_PARAM
     */
    NODE_PARAM_LIST,

    /**
     * Один параметр в списке (ребенок NODE_PARAM_LIST)
     * (хранит `id` параметра)
     * sibling -> следующий NODE_PARAM
     */
    NODE_PARAM,

    /**
     * Блок кода: { ... }
     * child -> первый оператор в блоке
     */
    NODE_BLOCK,

    // * --- 2. Узлы Операторов (Statements) ---
    // (Основано на <Operator>)

    /**
     * Оператор if: if (...) { ... } else { ... }
     * child -> Условие (<expression>)
     * child->sibling -> if-блок (NODE_BLOCK)
     * child->sibling->sibling -> else-блок (NODE_BLOCK) (может быть NULL)
     */
    NODE_IF,

    /**
     * Оператор while: while (...) { ... }
     * child -> Условие (<expression>)
     * child->sibling -> while-блок (NODE_BLOCK)
     */
    NODE_WHILE,

    /**
     * Определение переменной: var id
     * (хранит `id` переменной)
     */
    NODE_VAR_DEF,

    /**
     * Оператор return: return ...
     * child -> Возвращаемое выражение (<expression>) (может быть NULL)
     */
    NODE_RETURN,

    /**
     * Присваивание: id = ...
     * child -> NODE_ID (левая часть)
     * child->sibling -> Выражение (<expression>) (правая часть)
     */
    NODE_ASSIGNMENT,

    /**
     * Самостоятельный вызов функции: id(...)
     * child -> NODE_ID (имя функции)
     * child->sibling -> NODE_ARGUMENT_LIST (список аргументов)
     */
    NODE_CALL_STATEMENT,

    /**
     * Узел-контейнер для списка аргументов (ребенок NODE_CALL_STATEMENT)
     * child -> первый узел <expression>
     */
    NODE_ARGUMENT_LIST,

    // * --- 3. Узлы Выражений (Expressions) ---
    // Эти узлы *бинарные*:
    // child -> левый операнд
    // child->sibling -> правый операнд

    // Арифметические/Строковые
    NODE_OP_PLUS,
    NODE_OP_MINUS,
    NODE_OP_MUL,
    NODE_OP_DIV,

    // Реляционные
    NODE_OP_LT,
    NODE_OP_GT,
    NODE_OP_LTE,
    NODE_OP_GTE,

    // Равенство
    NODE_OP_EQ,
    NODE_OP_NEQ,

    // Проверка типа
    NODE_OP_IS, // is()

    // * --- 4. "Листья" Дерева (Termy/Literals) ---
    // (Основано на <term> и других "листьях")
    // Эти узлы не имеют `child`.

    /**
     * Идентификатор (имя переменной, имя функции)
     * (хранит `id` как строку)
     */
    NODE_ID,

    /**
     * Литерал-число (напр. 10, 3.14)
     * (хранит значение double)
     */
    NODE_LITERAL_NUM,

    /**
     * Литерал-строка (напр. "hello")
     * (хранит значение char*)
     */
    NODE_LITERAL_STRING,

    /**
     * Литерал null
     */
    NODE_LITERAL_NULL,

    /**
     * Имя типа, используемое в `is`
     * (напр. `Num`, `String`, `Null`)
     * (хранит имя как строку)
     */
    NODE_TYPE_NAME
} NodeType;


/**
 * Обобщенный узел AST.
 * Представляет *любой* узел в дереве
 */
typedef struct AstNode {
    //* --- 1. Общая информация (Заполняется в Pass 1) ---
    NodeType type;
    int line_number; // Номер строки для сообщений об ошибках

    //* --- 2. Структура Дерева (Заполняется в Pass 1) ---
    struct AstNode *child;
    struct AstNode *sibling;

    //* --- 3. Данные Узла (Заполняется в Pass 1) ---
    // Данные, специфичные для узла (в основном для листьев)
    union {
        /**
         * Используется для:
         * NODE_ID, NODE_VAR_DEF, NODE_PARAM, NODE_FUNCTION_DEF,
         * NODE_SETTER_DEF, NODE_GETTER_DEF, NODE_TYPE_NAME
         */
        char *identifier;

        /** Используется для: NODE_LITERAL_NUM */
        double literal_num;

        /** Используется для: NODE_LITERAL_STRING */
        char *literal_string;

    } data;

    //* --- 4. Семантическая Информация (Заполняется в Pass 2) ---

    /**
     * Тип данных, который *возвращает* это выражение.
     * (напр., узел NODE_OP_PLUS будет иметь data_type = TYPE_INT).
     * Для узлов-операторов (statements) это поле не используется.
     */
     //==================================================================
     //
     // ПРИМЕР КАК ЭТО РАБОТАЕТ
     //
     // Pass 1 (Парсер)   : Устанавливает это в TYPE_UNKNOWN.
     // Pass 2 (Семантика): *Вычисляет* и *заполняет* это поле
     //                     (напр., узел 'a+b' получит TYPE_NUM).
     // Pass 3 (Генератор): *Читает* это поле, чтобы генерировать
     //                     правильный код (напр., ADD для Num).
     //==================================================================
    DataType data_type;

    /**
     * Прямая ссылка на запись в таблице символов.
     * Используется для:
     * - NODE_ID (ссылка на переменную)
     * - NODE_CALL_STATEMENT (ссылка на определение функции)
     * - NODE_VAR_DEF (ссылка на новую запись в symtable)
     * - NODE_FUNCTION_DEF (ссылка на новую запись в symtable)
     * ... и т.д.
     */
     //==================================================================
     //
     // ПРИМЕР КАК ЭТО РАБОТАЕТ
     //
     // Pass 1 (Парсер)   : Устанавливает это в NULL.
     //                     (Парсер знает только имя "a", но не
     //                     что это "a" означает).
     // Pass 2 (Семантика): Находит символ (напр., 'a') в symtable
     //                     и заполняет это поле, устанавливая
     //                     прямую ссылку на его TableEntry.
     // Pass 3 (Генератор): Читает это поле, чтобы мгновенно
     //                     получить данные о 'a' (напр., ее тип
     //                     или где она хранится), без поиска по имени.
     //==================================================================
    TableEntry *table_entry;
} AstNode;


/**
 * @brief Создает новый узел AST.
 * Данные (union) и семантические поля инициализируются в NULL/0.
 * @param type Тип узла (NodeTypgit coe).
 * @param line_number Номер строки для отладки.
 * @return Указатель на новый узел.
 */
AstNode *ast_node_create(NodeType type, int line_number);

/**
 * @brief Рекурсивно освобождает узел и всех его детей и сиблингов.
 * !ВНИМАНИЕ: Эта функция также освобождает `data.identifier` и
 * !`data.literal_string`, если они не NULL.
 *
 *
 * @param node Узел для освобождения (может быть NULL).
 */
void ast_node_free_recursive(AstNode *node);

/**
 * @brief Добавляет 'new_child' в *конец* списка детей 'parent'.
 *
 * Если у 'parent' нет детей, `new_child` становится `parent->child`.
 * Если у 'parent' есть дети, `new_child` становится `sibling`'ом
 * последнего ребенка.
 *
 * @param parent Родительский узел (не должен быть NULL).
 * @param new_child Новый дочерний узел (не должен быть NULL).
 */
void ast_node_add_child(AstNode *parent, AstNode *new_child);


/**
 * @brief (Для отладки) Печатает дерево, начиная с 'node'.
 * @param node Корневой узел для печати.
 */
void ast_print_debug(AstNode *node);


/* ======================================*/
/* ===== 2. ПАРСЕР-ПОМОЩНИКИ (API) =====*/
/* ======================================*/

<<<<<<< HEAD
=======
AstNode *ast_new_id_node(NodeType type, int line, const char *id);

AstNode *ast_new_num_node(double value, int line);

AstNode *ast_new_string_node(const char *value, int line);

AstNode *ast_new_null_node(int line);

/* ======================================*/
/* ===== 2. ПАРСЕР-ПОМОЩНИКИ (API) =====*/
/* ======================================*/

>>>>>>> main
AstNode* ast_new_id_node(NodeType type, int line, const char* id);

AstNode* ast_new_num_node(double value, int line);

AstNode* ast_new_string_node(const char* value, int line);

AstNode* ast_new_null_node(int line);

AstNode* ast_new_bin_op(NodeType type, int line, AstNode* left, AstNode* right);
#endif