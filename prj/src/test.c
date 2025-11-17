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
 * TEST 5: Sibling Scope Isolation (Expecting FAIL - Error 3)
 *
 * static main() {
 * {
 * var x = 1
 * }
 * 'x' здесь уже умерла
 * {
 * x = 2 // ОШИБКА 3: 'x' не определена
 * }
 * }
 */
static AstNode *create_test_ast() {
    printf("Test 5: Sibling Isolation (Expecting FAIL with Error 3)...\n");

    AstNode *program = ast_node_create(NODE_PROGRAM, 0);
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    AstNode *main_block = ast_node_create(NODE_BLOCK, 1);
    
    ast_node_add_child(main_def, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(main_def, main_block);
    ast_node_add_child(program, main_def);

    // 1. Первый блок { var x }
    AstNode *block_1 = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(block_1, ast_new_id_node(NODE_VAR_DEF, 3, "x"));
    ast_node_add_child(main_block, block_1);

    // 2. Второй блок { x = 2 }
    AstNode *block_2 = ast_node_create(NODE_BLOCK, 5);
    
    AstNode *assign = ast_node_create(NODE_ASSIGNMENT, 6);
    ast_node_add_child(assign, ast_new_id_node(NODE_ID, 6, "x")); // !!! ОШИБКА !!!
    ast_node_add_child(assign, ast_new_num_node(2.0, 6));
    
    ast_node_add_child(block_2, assign);
    ast_node_add_child(main_block, block_2);

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