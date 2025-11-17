#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "semantics.h"
#include "tac.h"


#include <stdbool.h>
#include <stdio.h>

/*
Игровая площадка для тестирования чего угодно
Пожалуйста, не удаляйте этот файл, он нам еще пригодится
Если хотите запустить этот файл:

make test-pg
*/






/**
 static main() {
    // Блок 1
    {
        var a = 1
    } 
    // Тут 'a' больше нет

    // Блок 2
    {
        var a = 2 // Это ОК, так как это новая 'a' в новой таблице
    }
}
 */
static AstNode *create_test_ast() {
    printf("Test 7: Sibling Blocks (Same variable name in sibling scopes)...\n");

    AstNode *program = ast_node_create(NODE_PROGRAM, 0);
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    AstNode *main_block = ast_node_create(NODE_BLOCK, 1); // Уровень 1
    
    ast_node_add_child(main_def, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(main_def, main_block);
    ast_node_add_child(program, main_def);

    // --- БЛОК 1 (Уровень 2) ---
    AstNode *block_1 = ast_node_create(NODE_BLOCK, 2);
    {
        // var a
        ast_node_add_child(block_1, ast_new_id_node(NODE_VAR_DEF, 3, "a"));
        
        // a = 1
        AstNode *assign_1 = ast_node_create(NODE_ASSIGNMENT, 4);
        ast_node_add_child(assign_1, ast_new_id_node(NODE_ID, 4, "a"));
        ast_node_add_child(assign_1, ast_new_num_node(1.0, 4));
        ast_node_add_child(block_1, assign_1);
    }
    ast_node_add_child(main_block, block_1);


    // --- БЛОК 2 (Уровень 2 - Сосед) ---
    AstNode *block_2 = ast_node_create(NODE_BLOCK, 6);
    {
        // var a (СНОВА! Это должно быть разрешено)
        ast_node_add_child(block_2, ast_new_id_node(NODE_VAR_DEF, 7, "a"));
        
        // a = 2
        AstNode *assign_2 = ast_node_create(NODE_ASSIGNMENT, 8);
        ast_node_add_child(assign_2, ast_new_id_node(NODE_ID, 8, "a"));
        ast_node_add_child(assign_2, ast_new_num_node(2.0, 8));
        ast_node_add_child(block_2, assign_2);
    }
    ast_node_add_child(main_block, block_2);

    return program;
}
void test_gen_code() {

    Symtable global_table;
    symtable_init(&global_table);
    TACDLList tac_list;
    TACDLL_Init(&tac_list);
    AstNode *ast_root = create_test_ast(&global_table);
    generate_tac(ast_root, &tac_list, &global_table);
    
    printf("\n--- Generated Code ---\n");
    generate_code(&tac_list, &global_table);

    printf("\n3. Cleaning up resources...\n");
    ast_node_free_recursive(ast_root);
    symtable_free(&global_table);
    TACDLL_Dispose(&tac_list); // Это вызовет free_tac_instruction

    printf("Done.\n");
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