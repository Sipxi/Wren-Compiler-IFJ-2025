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
#include "optimizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

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
        exit(EXIT_FAILURE);
    }

    // Free data after successful insertion, since symtable makes its own copy
    free(data);
    return symtable_lookup(table, name);
}

/**
 * @brief Строит фейковый AST для кода:
 * (Тест для ВСЕХ случаев Constant Folding Оптимизатора)
 *
 * static main() {
 * var a // (для контроля)
 *
 * var s1 = "hello" + "world" // -> "helloworld"
 * var s2 = "a" * 3          // -> "aaa"
 * var n1 = 10 + 20.5        // -> 30.5
 * var n2 = 100 / 0          // -> (NaN, не сворачивать)
 *
 * a = 1 // (чтобы 'a' была 'Num')
 * var n3 = a + 10           // (НЕ сворачивать)
 * }
 */
static AstNode *create_test_ast(Symtable *global_table) {
    printf("1. Building Fake AST and Symtable...\n");

    // --- 1. (СИМУЛЯЦИЯ Pass 2) ---
    TableEntry *func_main = define_symbol(global_table, "main", KIND_FUNC);
    TableEntry *var_a = define_symbol(global_table, "a", KIND_VAR);
    TableEntry *var_s1 = define_symbol(global_table, "s1", KIND_VAR);
    TableEntry *var_s2 = define_symbol(global_table, "s2", KIND_VAR);
    TableEntry *var_n1 = define_symbol(global_table, "n1", KIND_VAR);
    TableEntry *var_n2 = define_symbol(global_table, "n2", KIND_VAR);
    TableEntry *var_n3 = define_symbol(global_table, "n3", KIND_VAR);


    // --- 2. Строим 'main()' ---
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    main_def->table_entry = func_main; // (Pass 2) Линкуем
    {
        AstNode *main_block = ast_node_create(NODE_BLOCK, 1);
        
        // --- Определения переменных ---
        ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 2, "a"));
        ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "s1"));
        ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 4, "s2"));
        ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 5, "n1"));
        ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 6, "n2"));
        ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 7, "n3"));
        // (Симуляция Pass 2: Линкуем 'var defs')
        main_block->child->table_entry = var_a;
        main_block->child->sibling->table_entry = var_s1;
        main_block->child->sibling->sibling->table_entry = var_s2;
        // ... и т.д. (не так важно для 3AC, но важно для семантики)


        // --- 1. s1 = "hello" + "world" ---
        AstNode *assign_s1 = ast_node_create(NODE_ASSIGNMENT, 9);
        AstNode* s1_id = ast_new_id_node(NODE_ID, 9, "s1");
        s1_id->table_entry = var_s1; // (Pass 2)
        
        AstNode* str_hello = ast_new_string_node("hello", 9); // (из ast.c)
        str_hello->data_type = TYPE_STR; // (Pass 2)
        AstNode* str_world = ast_new_string_node("world", 9); // (из ast.c)
        str_world->data_type = TYPE_STR; // (Pass 2)
        
        AstNode *plus_str = ast_new_bin_op(NODE_OP_PLUS, 9, str_hello, str_world);
        plus_str->data_type = TYPE_STR; // (Pass 2) 'Str + Str' -> 'Str'

        ast_node_add_child(assign_s1, s1_id);
        ast_node_add_child(assign_s1, plus_str);
        ast_node_add_child(main_block, assign_s1);
        var_s1->data->data_type = TYPE_STR; // (Pass 2) 's1' теперь String


        // --- 2. s2 = "a" * 3 ---
        AstNode *assign_s2 = ast_node_create(NODE_ASSIGNMENT, 10);
        AstNode* s2_id = ast_new_id_node(NODE_ID, 10, "s2");
        s2_id->table_entry = var_s2; // (Pass 2)
        
        AstNode* str_a = ast_new_string_node("a", 10);
        str_a->data_type = TYPE_STR; // (Pass 2)
        AstNode* num_3 = ast_new_num_node(3.0, 10);
        num_3->data_type = TYPE_NUM; // (Pass 2) -> Симулируем 'int'
        
        AstNode *mul_str = ast_new_bin_op(NODE_OP_MUL, 10, str_a, num_3);
        mul_str->data_type = TYPE_STR; // (Pass 2) 'Str * Num' -> 'Str'

        ast_node_add_child(assign_s2, s2_id);
        ast_node_add_child(assign_s2, mul_str);
        ast_node_add_child(main_block, assign_s2);
        var_s2->data->data_type = TYPE_STR; // (Pass 2) 's2' теперь String


        // --- 3. n1 = 10 + 20.5 ---
        AstNode *assign_n1 = ast_node_create(NODE_ASSIGNMENT, 11);
        AstNode* n1_id = ast_new_id_node(NODE_ID, 11, "n1");
        n1_id->table_entry = var_n1; // (Pass 2)

        AstNode* num_10 = ast_new_num_node(10.0, 11);
        num_10->data_type = TYPE_NUM; // (Pass 2) -> Симулируем 'int'
        AstNode* num_20_5 = ast_new_num_node(20.5, 11);
        num_20_5->data_type = TYPE_FLOAT; // (Pass 2) -> Симулируем 'float'

        AstNode *plus_num = ast_new_bin_op(NODE_OP_PLUS, 11, num_10, num_20_5);
        plus_num->data_type = TYPE_FLOAT; // (Pass 2) 'Num + Float' -> 'Float'
        
        ast_node_add_child(assign_n1, n1_id);
        ast_node_add_child(assign_n1, plus_num);
        ast_node_add_child(main_block, assign_n1);
        var_n1->data->data_type = TYPE_FLOAT; // (Pass 2) 'n1' теперь Float
        

        // --- 4. n2 = 100 / 0 ---
        AstNode *assign_n2 = ast_node_create(NODE_ASSIGNMENT, 12);
        AstNode* n2_id = ast_new_id_node(NODE_ID, 12, "n2");
        n2_id->table_entry = var_n2; // (Pass 2)
        
        AstNode* num_100 = ast_new_num_node(100.0, 12);
        num_100->data_type = TYPE_NUM; // (Pass 2)
        AstNode* num_0 = ast_new_num_node(0.0, 12);
        num_0->data_type = TYPE_NUM; // (Pass 2)
        
        AstNode *div_zero = ast_new_bin_op(NODE_OP_DIV, 12, num_100, num_0);
        div_zero->data_type = TYPE_FLOAT; // (Pass 2) Деление всегда 'Float'
        
        ast_node_add_child(assign_n2, n2_id);
        ast_node_add_child(assign_n2, div_zero);
        ast_node_add_child(main_block, assign_n2);
        var_n2->data->data_type = TYPE_FLOAT; // (Pass 2)


        // --- 5. a = 1 (Подготовка для Контроля) ---
        AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 13);
        AstNode* a_id_1 = ast_new_id_node(NODE_ID, 13, "a");
        a_id_1->table_entry = var_a; // (Pass 2)
        AstNode* num_1 = ast_new_num_node(1.0, 13);
        num_1->data_type = TYPE_NUM; // (Pass 2)
        ast_node_add_child(assign_a, a_id_1);
        ast_node_add_child(assign_a, num_1);
        ast_node_add_child(main_block, assign_a);
        var_a->data->data_type = TYPE_NUM; // (Pass 2) 'a' теперь Num


        // --- 6. n3 = a + 10 (Контроль) ---
        AstNode *assign_n3 = ast_node_create(NODE_ASSIGNMENT, 14);
        AstNode* n3_id = ast_new_id_node(NODE_ID, 14, "n3");
        n3_id->table_entry = var_n3; // (Pass 2)
        
        AstNode* a_id_2 = ast_new_id_node(NODE_ID, 14, "a");
        a_id_2->table_entry = var_a; // (Pass 2)
        a_id_2->data_type = TYPE_NUM; // (Pass 2) 'a' теперь Num
        
        AstNode* num_10_2 = ast_new_num_node(10.0, 14);
        num_10_2->data_type = TYPE_NUM; // (Pass 2)
        
        AstNode *plus_mixed = ast_new_bin_op(NODE_OP_PLUS, 14, a_id_2, num_10_2);
        plus_mixed->data_type = TYPE_NUM; // (Pass 2) 'Num + Num' -> 'Num'
        
        ast_node_add_child(assign_n3, n3_id);
        ast_node_add_child(assign_n3, plus_mixed);
        ast_node_add_child(main_block, assign_n3);
        var_n3->data->data_type = TYPE_NUM; // (Pass 2)
        

        // --- Собираем 'main' ---
        ast_node_add_child(main_def, ast_node_create(NODE_PARAM_LIST, 1));
        ast_node_add_child(main_def, main_block);
    }

    // --- 3. Собираем программу ---
    AstNode *program = ast_node_create(NODE_PROGRAM, 0);
    ast_node_add_child(program, main_def);

    printf("   ...AST Built (Test for ALL optimizer cases).\n");
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
        return EXIT_FAILURE;
    }

    Lexer *lexer = lexer_init();
    if (lexer == NULL) {
        fprintf(stderr, "Error initializing lexer.\n");
        fclose(file);
        return EXIT_FAILURE;
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
    if (fclose(file) != EXIT_SUCCESS) { // обработка ошибки закрытия файла
        fprintf(stderr, "Error closing file.\n");
    }
    return EXIT_SUCCESS;
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
    optimize_tac(&tac_list);


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

    return EXIT_SUCCESS;
}