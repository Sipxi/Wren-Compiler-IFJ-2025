#include "ast.h"
#include <stdlib.h>
#include <stdio.h>


/**
 * @brief Создает новый узел AST.
 * Данные (union) и семантические поля инициализируются в NULL/0.
 *
 * @param type Тип узла (NodeType).
 * @param line_number Номер строки для отладки.
 * @return Указатель на новый узел.
 */
AstNode* ast_node_create(NodeType type, int line_number)
{
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    if(node == NULL)
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
void ast_node_add_child(AstNode* parent, AstNode* new_child)
{
    if(parent==NULL || new_child==NULL)
    {
        return;
    }

    if(parent->child==NULL)
    {
        parent->child = new_child;
    }
    else
    {
        // Проходим по списку наших детей пока не находим последнего к которому добавляем сиблинга
        // Я не буду платить алмименты за эту ораву детей
        AstNode* current_child = parent->child;
        while(current_child->sibling != NULL)
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
void ast_node_free_recursive(AstNode* node)
{
    if(node == NULL)
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