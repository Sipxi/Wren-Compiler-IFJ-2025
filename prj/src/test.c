#include <stdio.h>
#include <stdlib.h>
#include "ast.h"         // <- Уже содержит ВСЕ нужные объявления
#include "ast_printer.h" // <- Объявляет ast_print_debug

/*
 * Блок с ручными объявлениями прототипов УДАЛЕН.
 * Они больше не нужны, так как ast.h теперь содержит
 * правильные (3-аргументные) версии.
 */

/**
 * @brief Создает "фейковое" AST для тестирования.
 *
 * Дерево представляет:
 * static my_func(a) {
 * var b;
 * b = a + 10;
 * return b;
 * }
 *
 * @return Указатель на корень дерева (NODE_PROGRAM).
 */
AstNode* build_fake_tree()
{
    printf("... Создание фейкового AST ...\n");

    // --- Строим изнутри наружу (от листьев к корню) ---

    // --- Выражение: (a + 10) ---
    // ИСПРАВЛЕНО: убран 4-й аргумент (NULL)
    AstNode* id_a_expr = ast_new_id_node(NODE_ID, 3, "a");
    AstNode* num_10 = ast_new_num_node(10.0, 3);
    AstNode* op_plus = ast_new_bin_op(NODE_OP_PLUS, 3, id_a_expr, num_10);

    // --- Оператор: b = (a + 10) ---
    // ИСПРАВЛЕНО: убран 4-й аргумент (NULL)
    AstNode* id_b_assign = ast_new_id_node(NODE_ID, 3, "b");
    AstNode* assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, id_b_assign); // Левая часть (LHS)
    ast_node_add_child(assignment, op_plus);     // Правая часть (RHS)

    // --- Оператор: var b; ---
    // ИСПРАВЛЕНО: убран 4-й аргумент (NULL)
    AstNode* var_b_def = ast_new_id_node(NODE_VAR_DEF, 2, "b");

    // --- Оператор: return b; ---
    // ИСПРАВЛЕНО: убран 4-й аргумент (NULL)
    AstNode* id_b_return = ast_new_id_node(NODE_ID, 4, "b");
    AstNode* return_stmt = ast_node_create(NODE_RETURN, 4);
    ast_node_add_child(return_stmt, id_b_return);

    // --- Блок: { ... } ---
    AstNode* block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(block, var_b_def);
    ast_node_add_child(block, assignment);
    ast_node_add_child(block, return_stmt);

    // --- Параметры: (a) ---
    // ИСПРАВЛЕНО: убран 4-й аргумент (NULL)
    AstNode* param_a = ast_new_id_node(NODE_PARAM, 1, "a");
    AstNode* param_list = ast_node_create(NODE_PARAM_LIST, 1);
    ast_node_add_child(param_list, param_a);

    // --- Определение функции: static my_func(...) { ... } ---
    // ИСПРАВЛЕНО: убран 4-й аргумент (NULL)
    AstNode* func_def = ast_new_id_node(NODE_FUNCTION_DEF, 1, "my_func");
    ast_node_add_child(func_def, param_list);
    ast_node_add_child(func_def, block);

    // --- Корень: NODE_PROGRAM ---
    AstNode* program = ast_node_create(NODE_PROGRAM, 1);
    ast_node_add_child(program, func_def);

    // Можно добавить вторую функцию для демонстрации сиблингов
    // ИСПРАВЛЕНО: убран 4-й аргумент (NULL)
    AstNode* getter = ast_new_id_node(NODE_GETTER_DEF, 6, "my_getter");
    ast_node_add_child(getter, ast_node_create(NODE_BLOCK, 6)); // Пустой блок
    
    ast_node_add_child(program, getter); // Добавляем как сиблинга к func_def

    printf("... AST создано ...\n\n");
    return program;
}


int main()
{
    // 1. Создаем дерево
    AstNode* root = build_fake_tree();

    // 2. Печатаем его
    ast_print_debug(root);

    // 3. Освобождаем память
    printf("\n... Освобождение AST ...\n");
    ast_node_free_recursive(root);
    printf("... Готово ...\n");

    return 0;
}