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
static AstNode *create_test_ast() {
    printf("Test: Nested Scope Access (Reading parent var)...\n");
    
    AstNode *program = ast_node_create(NODE_PROGRAM, 0);
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    AstNode *main_block = ast_node_create(NODE_BLOCK, 1);
    
    ast_node_add_child(main_def, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(main_def, main_block);
    ast_node_add_child(program, main_def);

    // 1. var outer = 100 (Уровень 1)
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 2, "outer"));
    
    AstNode *assign_out = ast_node_create(NODE_ASSIGNMENT, 2);
    ast_node_add_child(assign_out, ast_new_id_node(NODE_ID, 2, "outer"));
    ast_node_add_child(assign_out, ast_new_num_node(100.0, 2));
    ast_node_add_child(main_block, assign_out);

    // 2. Блок { ... } (Уровень 2)
    AstNode *inner_block = ast_node_create(NODE_BLOCK, 3);
    {
        // var inner
        ast_node_add_child(inner_block, ast_new_id_node(NODE_VAR_DEF, 4, "inner"));
        
        // inner = outer
        AstNode *assign_in = ast_node_create(NODE_ASSIGNMENT, 5);
        ast_node_add_child(assign_in, ast_new_id_node(NODE_ID, 5, "inner"));
        // Здесь мы используем 'outer'. Семантический анализ должен пойти
        // в стек: Уровень 2 (нет) -> Уровень 1 (НАШЕЛ!)
        ast_node_add_child(assign_in, ast_new_id_node(NODE_ID, 5, "outer"));
        
        ast_node_add_child(inner_block, assign_in);
    }
    ast_node_add_child(main_block, inner_block);

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