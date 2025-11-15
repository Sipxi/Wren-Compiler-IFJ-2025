#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "semantics.h"

#include <stdbool.h>
#include <stdio.h>

/*
Игровая площадка для тестирования чего угодно
Пожалуйста, не удаляйте этот файл, он нам еще пригодится
Если хотите запустить этот файл:

make test-pg
*/


/**
 * @brief Строит "сырой" фейковый AST (Симуляция ТОЛЬКО Pass 1 - Парсинг)
 * (Содержит СЕМАНТИЧЕСКУЮ ОШИБКУ для Pass 2)
 *
 * static main() {
 * var a
 * a = 10 + 20
 * }
 */
static AstNode *create_test_ast() {
    // (Мы ИГНОРИРУЕМ global_table, т.к. это Pass 1)
    
    printf("1. Building Fake AST (Pass 1 ONLY, with SEMANTIC ERROR)...\n");

    // --- 1. Строим 'main()' ---
    // (Pass 1)
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    {
        // (Pass 1)
        AstNode *main_block = ast_node_create(NODE_BLOCK, 1);
        
        // --- !!! ОШИБКА !!! ---
        // (Мы ПРОПУСКАЕМ 'NODE_VAR_DEF' для 'a')
        AstNode* def_a = ast_new_id_node(NODE_VAR_DEF, 2, "a");
        ast_node_add_child(main_block, def_a);

        // --- 1. a = 10 + 20 ---
        // (Pass 1)
        AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 3);
        
        // (Pass 1) LHS: 'a'
        AstNode* a_id_1 = ast_new_id_node(NODE_ID, 3, "a");
        // (Pass 2 должен будет проверить 'a_id_1->table_entry'
        // и не найдет его, вызвав ошибку)
        
        // (Pass 1) RHS: 10 + 20
        AstNode* num_10 = ast_new_num_node(10.0, 3);
        AstNode* num_20 = ast_new_num_node(20.0, 3);
        AstNode *plus_1 = ast_new_bin_op(NODE_OP_PLUS, 3, num_10, num_20);

        ast_node_add_child(assign_a, a_id_1);
        ast_node_add_child(assign_a, plus_1);
        ast_node_add_child(main_block, assign_a);
        
        // --- Собираем 'main' ---
        // (Pass 1)
        ast_node_add_child(main_def, ast_node_create(NODE_PARAM_LIST, 1));
        ast_node_add_child(main_def, main_block);
    }

    // --- 2. Собираем программу ---
    // (Pass 1)
    AstNode *program = ast_node_create(NODE_PROGRAM, 0);
    ast_node_add_child(program, main_def);

    printf("   ...Raw AST Built (Error: 'a' is not defined).\n");
    return program;
}

int main() {
    AstNode *root = create_test_ast();
    if (analyze_semantics(root) == false) {
        printf("Semantic analysis failed.\n");
        return 0;
    }

    symtable_print(&global_table);
    printf("Semantic analysis completed successfully.\n");
    return 0;
}