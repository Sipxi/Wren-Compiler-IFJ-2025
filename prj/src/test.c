/**
 * test_semantics.c
 *
 * Тестовый фреймворк для семантического анализатора (Pass 2).
 * Обновлен для работы с exit() кодами вместо return false/true.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "semantics.h" // Наш модуль, который мы тестируем
#include "ast.h"        // Наш ast.h
#include "symtable.h"   // Наша таблица символов

 // --- "Красивый визуал" (ANSI цвета) ---
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_CYAN    "\x1b[36m"

int total_tests = 0;
int passed_tests = 0;

/**
 * @brief "Запускатор" тестов.
 *
 * @param test_name         Имя теста для вывода.
 * @param test_ast_root     Фейковый AST, который нужно проверить.
 * @param expected_code     Ожидаемый код выхода (0 для успеха, коды ошибок для ошибок).
 */
void run_test(const char *test_name, AstNode *test_ast_root, int expected_code) {
    total_tests++;
    printf(ANSI_COLOR_CYAN "Тест: %s" ANSI_COLOR_RESET " (ожидаем код %d)...\n", test_name, expected_code);

    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс - запускаем семантический анализ
        // Если функция завершается нормально, выходим с кодом 0
        bool result = analyze_semantics(test_ast_root);
        if (result) {
            exit(0); // Успешное завершение
        } else {
            exit(99); // Это не должно происходить, так как теперь используется exit()
        }
    } else if (pid > 0) {
        // Родительский процесс - ждем завершения и проверяем код выхода
        int status;
        waitpid(pid, &status, 0);
        
        int actual_code = 0;
        if (WIFEXITED(status)) {
            actual_code = WEXITSTATUS(status);
        } else {
            actual_code = 99; // Процесс был убит сигналом
        }

        // Проверяем результат
        if (actual_code == expected_code) {
            printf("  " ANSI_COLOR_GREEN "[PASS]" ANSI_COLOR_RESET " Получен ожидаемый код выхода: %d\n", actual_code);
            passed_tests++;
        } else {
            printf("  " ANSI_COLOR_RED "[FAIL]" ANSI_COLOR_RESET " Ожидался код %d, но получен %d\n", expected_code, actual_code);
        }
    } else {
        // Ошибка fork()
        printf("  " ANSI_COLOR_RED "[ERROR]" ANSI_COLOR_RESET " Не удалось создать дочерний процесс\n");
    }

    // Очистка AST (делается в родительском процессе)
    if (test_ast_root) {
        ast_node_free_recursive(test_ast_root);
    }

    printf("------------------------------------------------------------\n");
}

/* ================================================================== */
/* ============= ФУНКЦИИ-ГЕНЕРАТОРЫ ФЕЙКОВЫХ AST ================== */
/* ================================================================== */

// Все функции-генераторы остаются теми же...
// (копируем все функции create_test_* без изменений)

/**
 * @brief Тест: Ошибка 4 (Редефиниция) [cite: 39]
 * Код:
 * static main() {
 * var a
 * var a  // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_redefinition_error() {
    // Используем твои API-функции из ast.h
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1)); // Пустой список параметров
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // var a  <-- Ошибка 4
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 4, "a"));

    return root;
}

/**
 * @brief Тест: Ошибка 3 (Неопределенная переменная) [cite: 38]
 * Код:
 * static main() {
 * a = 10  // <-- ОШИБКА ЗДЕСЬ ('a' не определена)
 * }
 */
AstNode *create_test_undefined_variable_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // a = 10
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a")); // <-- Ошибка 3
    ast_node_add_child(assignment, ast_new_num_node(10.0, 3));

    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Несовместимость типов) [cite: 41]
 * Код:
 * static main() {
 * var a = "hello" + 10 // <-- ОШИБКА ЗДЕСЬ (String + Num)
 * }
 * (Мы симулируем `var a; a = "hello" + 10;` т.к. `var a = ...` это расширение)
 */
AstNode *create_test_type_mismatch_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // "hello" + 10
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 3,
        ast_new_string_node("hello", 3), // левый операнд (String)
        ast_new_num_node(10.0, 3)         // правый операнд (Num)
    );

    // a = ...
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, plus_op);

    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: OK (Правильное использование переменной)
 * Код:
 * static main() {
 * var a
 * a = 10
 * var b = a + 20
 * }
 * (Симулируем: var a; a = 10; var b; b = a + 20;)
 */
AstNode *create_test_valid_program() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = 10
    AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign_a, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign_a, ast_new_num_node(10.0, 4));
    ast_node_add_child(main_block, assign_a);

    // var b
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 5, "b"));

    // a + 20
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 6,
        ast_new_id_node(NODE_ID, 6, "a"), // левый операнд (переменная)
        ast_new_num_node(20.0, 6)         // правый операнд (литерал)
    );

    // b = ...
    AstNode *assign_b = ast_node_create(NODE_ASSIGNMENT, 6);
    ast_node_add_child(assign_b, ast_new_id_node(NODE_ID, 6, "b"));
    ast_node_add_child(assign_b, plus_op);
    ast_node_add_child(main_block, assign_b);

    return root;
}

// Добавляем еще несколько тестов для демонстрации...

/**
 * @brief Тест: Ошибка 3 (Нет функции main)
 * Код:
 * static foo() { } // (main отсутствует)
 */
AstNode *create_test_no_main_function_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    AstNode *func_foo = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    ast_node_add_child(func_foo, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(func_foo, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_foo);

    return root;
}

/**
 * @brief Тест: Ошибка 5 (Встроенная функция - неверный тип параметра)
 * Код:
 * static main() {
 * var a = Ifj.length(123) // <-- ОШИБКА 5 (ожидает String)
 * }
 */
AstNode *create_test_builtin_type_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    AstNode *call_expr = ast_node_create(NODE_CALL_STATEMENT, 6);
    ast_node_add_child(call_expr, ast_new_id_node(NODE_ID, 6, "Ifj.length"));
    AstNode *args = ast_node_create(NODE_ARGUMENT_LIST, 6);
    ast_node_add_child(args, ast_new_num_node(123.0, 6)); // <-- Ошибка 5
    ast_node_add_child(call_expr, args);

    AstNode *assign = ast_node_create(NODE_ASSIGNMENT, 6);
    ast_node_add_child(assign, ast_new_id_node(NODE_ID, 6, "a"));
    ast_node_add_child(assign, call_expr);
    ast_node_add_child(main_block, assign);

    return root;
}

/* ================================================================== */
/* ==================== ГЛАВНАЯ ФУНКЦИЯ ТЕСТА ======================= */
/* ================================================================== */

int main() {
    printf("============================================================\n");
    printf(" Запуск тестов для Семантического Анализатора (Pass 2)...\n");
    printf(" (Используются коды выхода вместо булевых значений)\n");
    printf("============================================================\n");

    // --- Тесты на Ошибки (ожидаем соответствующие коды ошибок) ---
    printf(ANSI_COLOR_CYAN "--- Тесты на Ошибки ---\n" ANSI_COLOR_RESET);
    
    // Ошибки определения (код 3)
    run_test("Ошибка 3: Использование неопределенной переменной", create_test_undefined_variable_error(), 3);
    run_test("Ошибка 3: Нет функции main", create_test_no_main_function_error(), 3);
    
    // Ошибки переопределения (код 4)
    run_test("Ошибка 4: Переопределение переменной в том же скоупе", create_test_redefinition_error(), 4);
    
    // Ошибки типов встроенных функций (код 5)
    run_test("Ошибка 5: Неверный тип параметра встроенной функции", create_test_builtin_type_error(), 5);
    
    // Ошибки совместимости типов (код 6)
    run_test("Ошибка 6: Несовместимые типы в выражении", create_test_type_mismatch_error(), 6);

    // --- Тесты на Успех (ожидаем код 0) ---
    printf(ANSI_COLOR_CYAN "\n--- Тесты на Успех (ожидаем код 0) ---\n" ANSI_COLOR_RESET);
    run_test("Успех: Валидная программа", create_test_valid_program(), 0);

    // --- Итоги ---
    printf("============================================================\n");
    if (passed_tests == total_tests) {
        printf(ANSI_COLOR_GREEN "Все %d тестов пройдены успешно!\n" ANSI_COLOR_RESET, total_tests);
    } else {
        printf(ANSI_COLOR_RED "Провалено %d из %d тестов.\n" ANSI_COLOR_RESET, total_tests - passed_tests, total_tests);
    }
    printf("============================================================\n");

    return (passed_tests == total_tests) ? 0 : 1;
}