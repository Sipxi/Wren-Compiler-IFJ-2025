/**
 * @file test.c
 * Главный тестовый файл для проекта IFJ-2025.
 */

#include "ast.h"
#include "dll.h"
#include "symtable.h" 
#include "tac.h"      
#include "printer.h"  
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 


/*=======================================*/
/*===== ПРОТОТИПЫ ФУНКЦИЙ ===============*/
/*=======================================*/



/*=======================================*/
/*===== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ========*/
/*=======================================*/

/**
 * Вспомогательная функция для определения символа для симуляции семантики.
 * 
 * @param table Таблица символов.
 * @param name Имя символа.
 * @param kind Вид символа (переменная, функция и т.д).
 * @return Указатель на созданную запись таблицы символов.
 * @note Вызывает exit(1) при ошибке вставки.
 */
static TableEntry *define_symbol(Symtable *table, const char *name,
    SymbolKind kind) {
    SymbolData *data = (SymbolData *)calloc(1, sizeof(SymbolData));
    data->kind = kind;
    data->data_type = (kind == KIND_VAR) ? TYPE_NIL : TYPE_NUM; // По умолчанию
    data->is_defined = true;

    if (!symtable_insert(table, name, data)) {
        fprintf(stderr, "Failed to insert '%s' into symtable.\n", name);
        free(data); // symtable не завладел 'data', чистим
        exit(1);
    }

    // Free data after successful insertion, since symtable makes its own copy
    free(data);
    return symtable_lookup(table, name);
}

// =================================================================
//
// НОВАЯ ФУНКЦИЯ create_test_ast
//
// =================================================================
/**
 * @brief Строит фейковый AST для кода:
 * (Использует НОВОЕ API из ast.c)
 *
 * static func(p1, p2) {
 * return p1 + p2
 * }
 *
 * static main() {
 * var a
 * var b
 * a = 10
 * b = 20
 * func(a, b)
 * if (a > b) {
 * a = 20
 * } else {
 * b = 30
 * }
 * }
 */
static AstNode *create_test_ast(Symtable *global_table) {
    printf("1. Building Fake AST and Symtable...\n");

    // --- 1. Сначала заполняем Symtable (Симуляция Pass 2) ---
    TableEntry *func_main = define_symbol(global_table, "main", KIND_FUNC);
    TableEntry *func_func = define_symbol(global_table, "func", KIND_FUNC);
    TableEntry *var_a = define_symbol(global_table, "a", KIND_VAR);
    TableEntry *var_b = define_symbol(global_table, "b", KIND_VAR);
    TableEntry *param_p1 = define_symbol(global_table, "p1", KIND_VAR);
    TableEntry *param_p2 = define_symbol(global_table, "p2", KIND_VAR);

    // --- 2. Строим функцию 'func(p1, p2)' (Симуляция Pass 1 + Pass 2) ---
    
    // (Pass 1) Создаем узел 'func', используя НОВЫЙ ast.c API
    AstNode *func_def = ast_new_id_node(NODE_FUNCTION_DEF, 2, "func");
    // (Pass 2) ВРУЧНУЮ "линкуем" symtable
    func_def->table_entry = func_func;
    
    {
        // (Pass 1) Список параметров
        AstNode *param_list = ast_node_create(NODE_PARAM_LIST, 2);
        
        // (Pass 1) Узел 'p1'
        AstNode* p1_node = ast_new_id_node(NODE_PARAM, 2, "p1");
        // (Pass 2) Линкуем 'p1'
        p1_node->table_entry = param_p1;
        
        // (Pass 1) Узел 'p2'
        AstNode* p2_node = ast_new_id_node(NODE_PARAM, 2, "p2");
        // (Pass 2) Линкуем 'p2'
        p2_node->table_entry = param_p2;

        ast_node_add_child(param_list, p1_node);
        ast_node_add_child(param_list, p2_node);
        
        // Тело функции: { return p1 + p2; }
        AstNode *func_body = ast_node_create(NODE_BLOCK, 3);
        
        // Выражение: p1 + p2
        AstNode* p1_id = ast_new_id_node(NODE_ID, 3, "p1");
        p1_id->table_entry = param_p1; // (Pass 2)
        
        AstNode* p2_id = ast_new_id_node(NODE_ID, 3, "p2");
        p2_id->table_entry = param_p2; // (Pass 2)

        AstNode *op_plus = ast_new_bin_op(NODE_OP_PLUS, 3, p1_id, p2_id);
        
        // Стейтмент: return ...
        AstNode *return_stmt = ast_node_create(NODE_RETURN, 3);
        ast_node_add_child(return_stmt, op_plus);
        ast_node_add_child(func_body, return_stmt);

        // Собираем функцию
        ast_node_add_child(func_def, param_list);
        ast_node_add_child(func_def, func_body);
    }

    // --- 3. Строим функцию 'main()' ---
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 6, "main");
    main_def->table_entry = func_main; // (Pass 2)
    {
        AstNode *main_block = ast_node_create(NODE_BLOCK, 6);

        // var a
        AstNode* def_a = ast_new_id_node(NODE_VAR_DEF, 7, "a");
        def_a->table_entry = var_a; // (Pass 2)
        ast_node_add_child(main_block, def_a);

        // var b
        AstNode* def_b = ast_new_id_node(NODE_VAR_DEF, 8, "b");
        def_b->table_entry = var_b; // (Pass 2)
        ast_node_add_child(main_block, def_b);

        // a = 10
        AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 9);
        AstNode* a_id_1 = ast_new_id_node(NODE_ID, 9, "a");
        a_id_1->table_entry = var_a; // (Pass 2)
        ast_node_add_child(assign_a, a_id_1);
        ast_node_add_child(assign_a, ast_new_num_node(10.0, 9));
        ast_node_add_child(main_block, assign_a);

        // b = 20
        AstNode *assign_b = ast_node_create(NODE_ASSIGNMENT, 10);
        AstNode* b_id_1 = ast_new_id_node(NODE_ID, 10, "b");
        b_id_1->table_entry = var_b; // (Pass 2)
        ast_node_add_child(assign_b, b_id_1);
        ast_node_add_child(assign_b, ast_new_num_node(20.0, 10));
        ast_node_add_child(main_block, assign_b);

        // func(a, b)
        AstNode *call_stmt = ast_node_create(NODE_CALL_STATEMENT, 11);
        AstNode* func_id = ast_new_id_node(NODE_ID, 11, "func");
        func_id->table_entry = func_func; // (Pass 2)
        ast_node_add_child(call_stmt, func_id);

        AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, 11);
        AstNode* a_id_2 = ast_new_id_node(NODE_ID, 11, "a");
        a_id_2->table_entry = var_a; // (Pass 2)
        ast_node_add_child(arg_list, a_id_2);
        
        AstNode* b_id_2 = ast_new_id_node(NODE_ID, 11, "b");
        b_id_2->table_entry = var_b; // (Pass 2)
        ast_node_add_child(arg_list, b_id_2);
        
        ast_node_add_child(call_stmt, arg_list);
        ast_node_add_child(main_block, call_stmt);

        // if (a > b) { a = 20 } else { b = 30 }
        AstNode *if_stmt = ast_node_create(NODE_IF, 12);
        {
            // Условие: a > b
            AstNode* a_id_3 = ast_new_id_node(NODE_ID, 12, "a");
            a_id_3->table_entry = var_a; // (Pass 2)
            AstNode* b_id_3 = ast_new_id_node(NODE_ID, 12, "b");
            b_id_3->table_entry = var_b; // (Pass 2)
            AstNode *cond_gt = ast_new_bin_op(NODE_OP_GT, 12, a_id_3, b_id_3);
            
            // then-блок: { a = 20 }
            AstNode *then_block = ast_node_create(NODE_BLOCK, 13);
            AstNode *assign_a_20 = ast_node_create(NODE_ASSIGNMENT, 13);
            AstNode* a_id_4 = ast_new_id_node(NODE_ID, 13, "a");
            a_id_4->table_entry = var_a; // (Pass 2)
            ast_node_add_child(assign_a_20, a_id_4);
            ast_node_add_child(assign_a_20, ast_new_num_node(20.0, 13));
            ast_node_add_child(then_block, assign_a_20);

            // else-блок: { b = 30 }
            AstNode *else_block = ast_node_create(NODE_BLOCK, 15);
            AstNode *assign_b_30 = ast_node_create(NODE_ASSIGNMENT, 15);
            AstNode* b_id_4 = ast_new_id_node(NODE_ID, 15, "b");
            b_id_4->table_entry = var_b; // (Pass 2)
            ast_node_add_child(assign_b_30, b_id_4);
            ast_node_add_child(assign_b_30, ast_new_num_node(30.0, 15));
            ast_node_add_child(else_block, assign_b_30);

            // Собираем if
            ast_node_add_child(if_stmt, cond_gt);
            ast_node_add_child(if_stmt, then_block);
            ast_node_add_child(if_stmt, else_block);
        }
        ast_node_add_child(main_block, if_stmt);

        // Собираем функцию main
        ast_node_add_child(main_def, ast_node_create(NODE_PARAM_LIST, 6)); // Пустой список
        ast_node_add_child(main_def, main_block); // Тело
    }

    // --- 4. Собираем программу ---
    AstNode *program = ast_node_create(NODE_PROGRAM, 0);
    ast_node_add_child(program, func_def); // Добавляем 'func'
    ast_node_add_child(program, main_def); // Добавляем 'main'

    printf("   ...AST Built (using new API).\n");
    return program;
}

/**
 * @brief Печатает данные токена, экранируя специальные символы.
 * @param data Данные токена (строка).
 */
void print_token_data(const char *data) {
    for (const char *p = data; *p; p++) {
        switch (*p) {
        case '\n': printf("\\n"); break;
        case '\t': printf("\\t"); break;
        case '\r': printf("\\r"); break;
        default:   putchar(*p); break;
        }
    }
}

/*=======================================*/
// === ТЕСТОВЫЕ ФУНКЦИИ ===
/*=======================================*/

void test_symtable() {

    Symtable table;
    symtable_init(&table);
    symtable_print(&table);

    printf("Вставка символов...\n");
    Symtable local_table;
    symtable_init(&local_table);
    Symtable local_local_table;
    symtable_init(&local_local_table);
    SymbolData data_local = { KIND_FUNC, TYPE_NUM, true, &local_local_table };
    symtable_insert(&local_table, "local_var", &data_local);
    SymbolData data1 = { KIND_VAR, TYPE_NUM, true, &local_table };
    symtable_insert(&table, "var1", &data1);
    SymbolData data2 = { KIND_FUNC, TYPE_NUM, false, NULL };
    symtable_insert(&table, "func1", &data2);



    symtable_print(&table);

    printf("Удаление символа 'var1'...\n");
    symtable_delete(&table, "var1");
    symtable_print(&table);

    printf("Проверка переполнения таблицы...\n");
    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "var%d", i);
        SymbolData data = { KIND_VAR, TYPE_NUM, true, NULL };
        if (!symtable_insert(&table, key, &data)) {
            printf("Ошибка вставки символа '%s'\n", key);
        }
    }
    symtable_print(&table);

    printf("Проверка вставки существующего символа...\n");
    SymbolData data = { KIND_VAR, TYPE_NUM, true, NULL };
    symtable_insert(&table, "func1", &data); // Перезапись существующего

    symtable_print(&table);
    symtable_free(&table);
}

int test_lexer() {
    // Use stdin for input (supports redirection like: ./test < input.wren)
    FILE *file = fopen("example.wren", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return 1;
    }

    Lexer *lexer = lexer_init();
    if (lexer == NULL) {
        fprintf(stderr, "Error initializing lexer.\n");
        fclose(file);
        return 1;
    }
    while (lexer->current_token->type != TOKEN_EOF) {
        get_next_token(lexer, file);

        printf("Token Type: %s, Data: ",
            token_type_to_string(lexer->current_token->type));

        print_token_data(lexer->current_token->data);

        printf(", Line: %d\n", lexer->current_token->line);
    }
    // Don't close stdin
    lexer_free(lexer);
    if (fclose(file) != 0) { // обработка ошибки закрытия файла
        fprintf(stderr, "Error closing file.\n");
    }
    return 0;
}

void test_tac_generator() {

    printf("--- 3AC Generator Test ---\n");

    // 1. Инициализация
    Symtable global_table;
    symtable_init(&global_table);

    DLList tac_list;
    DLL_Init(&tac_list);

    // 2. Создаем AST и заполняем Symtable
    AstNode *ast_root = create_test_ast(&global_table);

    // Раскомментируй, если у тебя есть symtable_print
    // printf("\n--- Symbol Table (Simulated Pass 2) ---\n");
    // symtable_print(&global_table);

    // 3. === ЗАПУСКАЕМ ТВОЙ ГЕНЕРАТОР ===
    printf("\n2. Calling generate_tac()...\n");
    generate_tac(ast_root, &tac_list, &global_table);
    printf("   ...generate_tac() finished.\n");

    // 4. Печатаем результат
    // (Убедись, что у тебя есть 'printer.c' и 'printer.h' с этой функцией)
    printf("\n--- Generated 3-Address Code ---\n");
    print_tac_list(&tac_list);

    // 5. Очистка
    printf("\n3. Cleaning up resources...\n");
    ast_node_free_recursive(ast_root);
    symtable_free(&global_table);
    DLL_Dispose(&tac_list); // Это вызовет free_tac_instruction

    printf("Done.\n");

}

/*=======================================*/
// === ГЛАВНАЯ ФУНКЦИЯ ===
/*=======================================*/

int main() {
    printf("=== IFJ-2025 Test Suite ===\n\n");

    test_tac_generator();

    return 0;
}