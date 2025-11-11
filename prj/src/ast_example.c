/** @file ast.c
 * @brief Пример использования AST
 *
 * ! Этот файл был полностью написан GEMINI,
 * ! УБРАТЬ ЕГО КОГДА СДЕЛАЕТСЯ ГОТОВАЯ ВЕРСИЯ AST
 * 
 * @author
 *      - Gemini
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "utils.h"

AstNode* ast_node_create(NodeType type, int line_number) {
    // calloc сразу обнуляет всю память (ставит NULL, 0.0, и т.д.)
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    if (node == NULL) {
        fprintf(stderr, "FATAL: Failed to allocate AstNode\n");
        exit(1); // В реальном компиляторе тут будет код ошибки
    }
    
    node->type = type;
    node->line_number = line_number;
    
    // Все указатели (child, sibling, table_entry) уже NULL
    // Все data (union) уже 0/NULL
    // data_type уже 0 (TYPE_NUM, если у тебя 0 - не TYPE_UNKNOWN)
    
    return node;
}

void ast_node_free_recursive(AstNode* node) {
    if (node == NULL) {
        return;
    }
    
    // 1. Рекурсивно чистим *детей*
    ast_node_free_recursive(node->child);
    
    // 2. Рекурсивно чистим *братьев*
    ast_node_free_recursive(node->sibling);
    
    // 3. Чистим данные (строки), которыми *владеет* этот узел
    switch (node->type) {
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
            // У других узлов (NODE_LITERAL_NUM, NODE_PROGRAM и т.д.)
            // нет данных в `union`, которые надо free'кать.
            break;
    }
    
    // 4. Чистим сам узел
    free(node);
}

void ast_node_add_child(AstNode* parent, AstNode* new_child) {
    if (parent == NULL || new_child == NULL) {
        return;
    }
    
    // 1. Если детей нет, 'new_child' становится 'child'
    if (parent->child == NULL) {
        parent->child = new_child;
    } else {
        // 2. Если дети есть, идем в конец списка "братьев"
        AstNode* current = parent->child;
        while (current->sibling != NULL) {
            current = current->sibling;
        }
        // 3. Добавляем 'new_child' как "брата"
        current->sibling = new_child;
    }
}


// --- Вспомогательные функции для "фейкового" AST в test.c ---
// (Они нужны, чтобы test.c был чище)

// Создает узел и сразу копирует 'identifier'
AstNode* ast_new_id_node(NodeType type, int line, const char* id, TableEntry* entry) {
    AstNode* node = ast_node_create(type, line);
    node->data.identifier = strdup_c99(id);
    node->table_entry = entry; // Сразу линкуем (симуляция семантики)
    return node;
}

// Создает узел-литерал (число)
AstNode* ast_new_num_node(double value, int line) {
    AstNode* node = ast_node_create(NODE_LITERAL_NUM, line);
    node->data.literal_num = value;
    node->data_type = TYPE_NUM; // Симуляция семантики
    return node;
}

// Создает бинарную операцию
AstNode* ast_new_bin_op(NodeType type, int line, AstNode* left, AstNode* right) {
    AstNode* node = ast_node_create(type, line);
    ast_node_add_child(node, left);
    ast_node_add_child(node, right);
    node->data_type = TYPE_NUM; // Симуляция семантики (для '+')
    return node;
}