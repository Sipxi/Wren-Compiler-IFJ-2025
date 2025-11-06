/* sample_ast.c
 * Реализация построения "фейкового" AST.
 */
#include <stdio.h>
#include "sample_ast.h"

// Вспомогательная функция для создания узла
static AstNode* create_node(AstNodeType type) {
    AstNode *node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = type;
    return node;
}

// Вспомогательная функция для создания узла-литерала (число)
static AstNode* create_num_literal(int value) {
    AstNode *node = create_node(AST_LITERAL);
    node->data.literal_value.type = TYPE_NUM;
    node->data.literal_value.value.int_value = value;
    return node;
}

// Вспомогательная функция для создания узла-идентификатора
// Он сразу связывается с symtable
static AstNode* create_identifier(TableEntry *entry) {
    AstNode *node = create_node(AST_IDENTIFIER);
    node->data.symbol_entry = entry;
    return node;
}

// Вспомогательная функция для создания бинарной операции
static AstNode* create_bin_op(TacOperationCode op, AstNode *left, AstNode *right) {
    AstNode *node = create_node(AST_BIN_OP);
    node->data.op_code = op;
    node->child = left; // Левый операнд
    left->next = right; // Правый операнд
    return node;
}

// Вспомогательная функция для регистрации переменной в symtable
static TableEntry* define_variable(Symtable *table, const char *name, DataType type) {
    SymbolData *data = (SymbolData*)malloc(sizeof(SymbolData));
    data->kind = KIND_VAR;
    data->data_type = type;
    data->is_defined = true;
    data->local_table = NULL;
    
    if (!symtable_insert(table, name, data)) {
        fprintf(stderr, "Failed to insert '%s' into symtable.\n", name);
        free(data); // Free before exit
        exit(1);
    }
    
    // symtable_insert makes its own copy, so we need to free the original
    free(data);
    
    return symtable_lookup(table, name);
}


// Главная функция, строящая дерево
AstNode* create_sample_program(Symtable *global_table) {
    
    // --- Шаг 1: Заполняем Symtable ---
    // Это делает семантический анализ до вас
    TableEntry *var_a = define_variable(global_table, "a", TYPE_NUM);
    TableEntry *var_b = define_variable(global_table, "b", TYPE_NUM);

    // --- Шаг 2: Строим AST "снизу вверх" ---

    // Стейтмент 1: var a = 10
    AstNode *stmt1 = create_node(AST_VAR_DEF);
    stmt1->data.symbol_entry = var_a;      // Ссылка на 'a'
    stmt1->child = create_num_literal(10); // Выражение (RHS)

    // Стейтмент 2: var b = 20
    AstNode *stmt2 = create_node(AST_VAR_DEF);
    stmt2->data.symbol_entry = var_b;
    stmt2->child = create_num_literal(20);

    // Стейтмент 3: if (a < b) { ... } else { ... }
    AstNode *stmt3 = create_node(AST_IF);

    // Условие: a < b
    stmt3->data.if_stmt.condition = create_bin_op(OP_LESS,
                                                  create_identifier(var_a),
                                                  create_identifier(var_b));
    
    // 'Then' блок: { a = a + 1 }
    // Выражение 'a + 1'
    AstNode *then_rhs = create_bin_op(OP_ADD,
                                      create_identifier(var_a),
                                      create_num_literal(1));
    // Стейтмент 'a = ...'
    AstNode *then_stmt = create_node(AST_ASSIGN);
    then_stmt->data.symbol_entry = var_a; // LHS ('a')
    then_stmt->child = then_rhs;          // RHS ('a + 1')
    
    // Оборачиваем в "список стейтментов" (даже если он один)
    AstNode *then_list = create_node(AST_STATEMENT_LIST);
    then_list->child = then_stmt;
    stmt3->data.if_stmt.then_branch = then_list;

    // 'Else' блок: { b = b + 1 }
    AstNode *else_rhs = create_bin_op(OP_ADD,
                                      create_identifier(var_b),
                                      create_num_literal(1));
    AstNode *else_stmt = create_node(AST_ASSIGN);
    else_stmt->data.symbol_entry = var_b;
    else_stmt->child = else_rhs;
    
    AstNode *else_list = create_node(AST_STATEMENT_LIST);
    else_list->child = else_stmt;
    stmt3->data.if_stmt.else_branch = else_list;


    // --- Шаг 3: Собираем все в одну программу ---
    
    // Список стейтментов верхнего уровня
    AstNode *main_list = create_node(AST_STATEMENT_LIST);
    main_list->child = stmt1;
    stmt1->next = stmt2;
    stmt2->next = stmt3;
    
    // Корень
    AstNode *program_root = create_node(AST_PROGRAM);
    program_root->child = main_list;

    return program_root;
}

// Рекурсивная очистка
void free_ast(AstNode *node) {
    if (node == NULL) {
        return;
    }

    // Рекурсивно чистим в зависимости от типа узла
    switch (node->type) {
        case AST_PROGRAM:
        case AST_VAR_DEF:
        case AST_ASSIGN:
        case AST_STATEMENT_LIST:
            free_ast(node->child);
            break;
        
        case AST_BIN_OP:
            if (node->child) {
                AstNode *right = node->child->next; // Save pointer to right child
                node->child->next = NULL;           // Break the link to prevent double-free
                free_ast(node->child);              // Free left child (won't free right)
                free_ast(right);                    // Free right child
            }
            break;

        case AST_IF:
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.then_branch);
            free_ast(node->data.if_stmt.else_branch);
            break;

        case AST_LITERAL: // Чистим строку, если есть
            if (node->data.literal_value.type == TYPE_STR) {
                free(node->data.literal_value.value.str_value);
            }
            break;
        
        case AST_IDENTIFIER:
            // Ничего не чистим, symtable владеет TableEntry
            break;
    }

    // Чистим следующий по списку
    free_ast(node->next);
    
    // Чистим сам узел
    free(node);
}