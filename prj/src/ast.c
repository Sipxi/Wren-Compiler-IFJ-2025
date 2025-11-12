#include "ast.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/**
 * @brief Создает новый узел AST.
 * Данные (union) и семантические поля инициализируются в NULL/0.
 *
 * @param type Тип узла (NodeType).
 * @param line_number Номер строки для отладки.
 * @return Указатель на новый узел.
 */
AstNode *ast_node_create(NodeType type, int line_number)
{
    AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
    if (node == NULL)
    {
        return NULL;
    }

    node->type = type;
    node->line_number = line_number;

    return node;
}

/**
 * @brief Добавляет 'new_child' в конец списка детей 'parent'.
 *
 * @param parent Родительский узел (не должен быть NULL).
 * @param new_child Новый дочерний узел (не должен быть NULL).
 */
void ast_node_add_child(AstNode *parent, AstNode *new_child)
{
    if (parent == NULL || new_child == NULL)
    {
        return;
    }

    if (parent->child == NULL)
    {
        parent->child = new_child;
    }
    else
    {
        // Проходим по списку наших детей пока не находим последнего к которому добавляем сиблинга
        // Я не буду платить алмименты за эту ораву детей
        AstNode *current_child = parent->child;
        while (current_child->sibling != NULL)
        {
            current_child = current_child->sibling;
        }
        current_child->sibling = new_child;
    }
}

/**
 * @brief Рекурсивно освобождает узел и всех его детей и сиблингов.
 *
 * @param node Узел для освобождения (может быть NULL).
 */
void ast_node_free_recursive(AstNode *node)
{
    if (node == NULL)
    {
        return;
    }

    ast_node_free_recursive(node->child);   // Освобождаем горизонтально
    ast_node_free_recursive(node->sibling);  // Освобождаем вертикально

    // Освобождаем данные внутри узла
    // Они есть только у некоторых, остальные -> default
    switch (node->type)
    {
    case NODE_ID:
    case NODE_VAR_DEF:
    case NODE_PARAM:
    case NODE_FUNCTION_DEF:
    case NODE_SETTER_DEF:
    case NODE_GETTER_DEF:
    case NODE_TYPE_NAME:
        free(node->data.identifier);
        break;
    case NODE_LITERAL_STRING:
        free(node->data.literal_string);
        break;
    default:
        break;
    }

    free(node);
}

// --- Вспомогательные функции для AST (Парсер-API) ---
// Эти функции "прячут" грязную работу по созданию
// узлов с данными. Парсеру не нужно помнить,

/**
 * @brief (Помощник) Создает узел, хранящий 'identifier' (копируя его).
 *
 * Используется для: NODE_ID, NODE_VAR_DEF, NODE_PARAM,
 * NODE_FUNCTION_DEF, NODE_TYPE_NAME и т.д.
 * @param type Тип узла.
 * @param line Номер строки.
 * @param id Строка (имя), которую нужно скопировать.
 * @param entry Это поле здесь лишнее, убираем его.
 * @return Указатель на новый узел.
 */
AstNode *ast_new_id_node(NodeType type, int line, const char *id) {
    // 1. Создаем "пустой" узел (calloc обнуляет data_type и table_entry)
    AstNode *node = ast_node_create(type, line);
    if (node == NULL) return NULL; // Ошибка в ast_node_create

    // 2. Копируем строку
    node->data.identifier = strdup_c99(id);
    if (node->data.identifier == NULL && id != NULL) {
        free(node);
        return NULL;
    }

    // 3. Семантические поля (data_type, table_entry) остаются NULL.
    return node;
}

/**
 * @brief (Помощник) Создает узел-литерал (число).
 * @param value Числовое значение.
 * @param line Номер строки.
 * @return Указатель на новый узел.
 */
AstNode *ast_new_num_node(double value, int line) {
    // 1. Создаем "пустой" узел
    AstNode *node = ast_node_create(NODE_LITERAL_NUM, line);
    if (node == NULL) return NULL;

    // 2. Кладем *значение* (не указатель!) прямо в union.
    node->data.literal_num = value;

    // 3. Семантические поля остаются NULL.
    return node;
}

/**
 * @brief (Помощник) Создает узел-литерал (строка) (копируя ее).
 * @param value Строка, которую нужно скопировать.
 * @param line Номер строки.
 * @return Указатель на новый узел.
 */
AstNode *ast_new_string_node(const char *value, int line) {
    // 1. Создаем "пустой" узел
    AstNode *node = ast_node_create(NODE_LITERAL_STRING, line);
    if (node == NULL) return NULL;

    // 2. Копируем строку
    node->data.literal_string = strdup_c99(value);
    if (node->data.literal_string == NULL && value != NULL) {
        free(node); // Ошибка в strdup_c99
        return NULL;
    }

    // 3. Семантические поля остаются NULL.
    return node;
}

/**
 * @brief (Помощник) Создает узел-литерал (null).
 * @param line Номер строки.
 * @return Указатель на новый узел.
 */
AstNode *ast_new_null_node(int line) {
    // 1. Создаем "пустой" узел
    AstNode *node = ast_node_create(NODE_LITERAL_NULL, line);

    // 2. Семантические поля остаются NULL.
    return node;
}


/**
 * @brief (Помощник) Создает бинарную операцию (ярлык).
 * @param type Тип узла (NODE_OP_PLUS, NODE_OP_GT и т.д.).
 * @param line Номер строки.
 * @param left Левый дочерний узел.
 * @param right Правый дочерний узел.
 * @return Указатель на новый узел.
 */
AstNode *ast_new_bin_op(NodeType type, int line, AstNode *left, AstNode *right) {
    // 1. Создаем узел-оператор
    AstNode *node = ast_node_create(type, line);
    if (node == NULL) return NULL;

    // 2. Добавляем левого и правого ребенка
    ast_node_add_child(node, left);
    ast_node_add_child(node, right);

    // 3. Семантические поля остаются NULL.
    return node;
}