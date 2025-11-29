#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "semantics.h"
#include "symtable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/*
  Код для ручного создания AST и тестирования семантики.
  Для компиляции: make test-pg (или gcc -o test test.c parser.c ast.c semantics.c symtable.c lexer.c utils.c -I.)
*/

/**
 * Строит AST для следующего кода IFJ25:
 * * import "ifj25" for Ifj
 * class Program {
 * static fun(){
 * var a
 * a = 4
 * return
 * }
 * static main(){
 * var a
 * a = fun()
 * a = Ifj.write(a)
 * }
 * }
 */
static AstNode *create_test_ast() {
    printf("Building AST for function call test...\n");

    // 1. Kорень программы
    AstNode *program = ast_node_create(NODE_PROGRAM, 0);

    // ==========================================
    // FUNCTION: static fun()
    // ==========================================
    AstNode *fun_def = ast_new_id_node(NODE_FUNCTION_DEF, 2, "fun");
    AstNode *fun_params = ast_node_create(NODE_PARAM_LIST, 2); // Нет параметров
    AstNode *fun_block = ast_node_create(NODE_BLOCK, 2);

    ast_node_add_child(fun_def, fun_params);
    ast_node_add_child(fun_def, fun_block);
    ast_node_add_child(program, fun_def);

    // --- BODY of fun ---

    // var a
    ast_node_add_child(fun_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = 4
    AstNode *assign_a_fun = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign_a_fun, ast_new_id_node(NODE_ID, 4, "a")); // Left: a
    ast_node_add_child(assign_a_fun, ast_new_num_node(4.0, 4));         // Right: 4
    ast_node_add_child(fun_block, assign_a_fun);

    // return (неявный return null, так как нет выражения)
    AstNode *return_node = ast_node_create(NODE_RETURN, 5);
    ast_node_add_child(return_node, ast_new_null_node(5)); // return null
    ast_node_add_child(fun_block, return_node);

    // ==========================================
    // FUNCTION: static main()
    // ==========================================
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 7, "main");
    AstNode *main_params = ast_node_create(NODE_PARAM_LIST, 7); // Нет параметров
    AstNode *main_block = ast_node_create(NODE_BLOCK, 7);

    ast_node_add_child(main_def, main_params);
    ast_node_add_child(main_def, main_block);
    ast_node_add_child(program, main_def);

    // --- BODY of main ---

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 8, "a"));

    // a = fun()
    AstNode *assign_call_fun = ast_node_create(NODE_ASSIGNMENT, 9);
    ast_node_add_child(assign_call_fun, ast_new_id_node(NODE_ID, 9, "a")); // Left: a

    // Right: Call Statement
    AstNode *call_fun = ast_node_create(NODE_CALL_STATEMENT, 9);
    ast_node_add_child(call_fun, ast_new_id_node(NODE_ID, 9, "fun"));     // Func name
    ast_node_add_child(call_fun, ast_node_create(NODE_ARGUMENT_LIST, 9)); // Args (empty)
    
    ast_node_add_child(assign_call_fun, call_fun);
    ast_node_add_child(main_block, assign_call_fun);

    // a = Ifj.write(a)
    AstNode *assign_write = ast_node_create(NODE_ASSIGNMENT, 10);
    ast_node_add_child(assign_write, ast_new_id_node(NODE_ID, 10, "a")); // Left: a

    // Right: Call Ifj.write
    AstNode *call_write = ast_node_create(NODE_CALL_STATEMENT, 10);
    ast_node_add_child(call_write, ast_new_id_node(NODE_ID, 10, "Ifj.write")); // Func name

    // Args: (a)
    AstNode *args_write = ast_node_create(NODE_ARGUMENT_LIST, 10);
    ast_node_add_child(args_write, ast_new_id_node(NODE_ID, 10, "a"));
    ast_node_add_child(call_write, args_write);

    ast_node_add_child(assign_write, call_write);
    ast_node_add_child(main_block, assign_write);

    return program;
}

int main() {
    // 1. Создаем AST вручную
    AstNode *root = create_test_ast();

    printf("--------------------------------------------------\n");
    printf("Starting Semantic Analysis...\n");

    // 2. Запускаем семантический анализ
    if (analyze_semantics(root) == false) {
        printf("\033[1;31mSemantic analysis failed.\033[0m\n");
        // Освобождение ресурсов (опционально для теста, но полезно)
        // ast_node_free_recursive(root);
        // symtable_free(&global_table);
        return 1; // Возвращаем код ошибки
    }

    printf("\033[1;32mSemantic analysis completed successfully.\033[0m\n");
    printf("--------------------------------------------------\n");

    // 3. Печатаем таблицу символов для проверки
    symtable_print(&global_table);

    // 4. Очистка (в реальном компиляторе это делается в конце)
    // ast_node_free_recursive(root);
    symtable_free(&global_table);

    return 0;
}