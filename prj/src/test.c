/**
 * test_semantics.c
 *
 * Тестовый фреймворк для семантического анализатора (Pass 2).
 * Обновлен для работы с exit() кодами вместо return false/true.
 */


#include "lexer.h"
#include "parser.h"
#include "optimizer.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "semantics.h" // Наш модуль, который мы тестируем
#include "ast.h"        // Наш ast.h
#include "tac.h"        // Наш 3ac.h
#include "optimizer.h"        // Наш optimizer.h
#include "codegen.h"    // Наш codegen.h
#include "symtable.h"   // Наша таблица символов
#include "parser.h"     // Наш parser.h
#include "printer.h"   // Для отладки

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

/**
 * @brief Тест: OK (Правильное вложенное затенение - shadowing с выражением)
 * Код:
 * static main() {
 * var a
 * if (5 == 10) {
 * var a // <-- OK, это новая 'a' в новой области видимости
 * }
 * }
 */
AstNode *create_test_valid_shadowing() {
    // Корень программы
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1)); // Пустой список параметров ()
    ast_node_add_child(root, func_main);

    // { ... } (Блок main)
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a (на 2-й строке)
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 2, "a"));

    // if (5 == 10) (на 3-й строке)
    AstNode *if_stmt = ast_node_create(NODE_IF, 3);
    ast_node_add_child(main_block, if_stmt);

    // ---
    // Создание узла условия (5 == 10)
    // ---
    // 1. Создаем узел для операции '=='
    AstNode *condition_op = ast_node_create(NODE_OP_EQ, 3); // Предполагаемое имя узла

    // 2. Создаем и добавляем левый операнд (5)
    ast_node_add_child(condition_op, ast_new_num_node(5.0, 3));

    // 3. Создаем и добавляем правый операнд (10)
    ast_node_add_child(condition_op, ast_new_num_node(10.0, 3));

    // 4. Добавляем узел '==' (со всем его поддеревом) как условие в if
    ast_node_add_child(if_stmt, condition_op);
    // --- Конец изменений ---

    // { ... } (Блок if, на 4-й строке)
    AstNode *if_block = ast_node_create(NODE_BLOCK, 4);
    ast_node_add_child(if_stmt, if_block);

    // var a (внутри if, на 4-й строке)
    ast_node_add_child(if_block, ast_new_id_node(NODE_VAR_DEF, 4, "a")); // <-- OK

    // else { } (Пустой else, строка 5)
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 5));

    return root;
}

/* ================================================================== */
/* ============= ДОПОЛНИТЕЛЬНЫЕ ФУНКЦИИ-ГЕНЕРАТОРЫ AST ============= */
/* ================================================================== */

/**
 * [cite_start]@brief Тест: Ошибка 3 (Неопределенная функция) [cite: 38]
 * Код:
 * static main() {
 * неопределеннаяФункция()
 * }
 */
AstNode *create_test_undefined_function_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // неопределеннаяФункция()
    AstNode *call = ast_node_create(NODE_CALL_STATEMENT, 3);
    ast_node_add_child(call, ast_new_id_node(NODE_ID, 3, "неопределеннаяФункция"));
    ast_node_add_child(call, ast_node_create(NODE_ARGUMENT_LIST, 3)); // Пустой список аргументов

    ast_node_add_child(main_block, call);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 4 (Редефиниция функции) [cite: 39]
 * Код:
 * static foo() { }
 * static foo() { } // <-- ОШИБКА ЗДЕСЬ
 * static main() { }
 */
AstNode *create_test_function_redefinition_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static foo() { }
    AstNode *func_foo1 = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    ast_node_add_child(func_foo1, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(func_foo1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_foo1);

    // static foo() { }  <-- Ошибка 4 (такое же имя, такая же арность)
    AstNode *func_foo2 = ast_new_id_node(NODE_FUNCTION_DEF, 2, "foo");
    ast_node_add_child(func_foo2, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(func_foo2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, func_foo2);

    // static main() { } (Нужен для запуска)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}


/**
 * [cite_start]@brief Тест: Ошибка 5 (Слишком мало аргументов) [cite: 40]
 * Код:
 * static foo(a, b) { }
 * static main() {
 * foo(1) // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_too_few_args_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static foo(a, b)
    AstNode *func_foo = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    AstNode *foo_params = ast_node_create(NODE_PARAM_LIST, 1);
    ast_node_add_child(foo_params, ast_new_id_node(NODE_PARAM, 1, "a"));
    ast_node_add_child(foo_params, ast_new_id_node(NODE_PARAM, 1, "b")); // 2 параметра
    ast_node_add_child(func_foo, foo_params);
    ast_node_add_child(func_foo, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_foo);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 2, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 3);
    ast_node_add_child(func_main, main_block);

    // foo(1)
    AstNode *call = ast_node_create(NODE_CALL_STATEMENT, 4);
    ast_node_add_child(call, ast_new_id_node(NODE_ID, 4, "foo"));
    AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, 4);
    ast_node_add_child(arg_list, ast_new_num_node(1.0, 4)); // 1 аргумент
    ast_node_add_child(call, arg_list);
    ast_node_add_child(main_block, call);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 6 (Арифметика со строками) [cite: 41, 288]
 * Код:
 * static main() {
 * var a = "a" * "b" // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_string_arithmetic_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // "a" * "b"
    AstNode *mul_op = ast_new_bin_op(NODE_OP_MUL, 3,
        ast_new_string_node("a", 3),
        ast_new_string_node("b", 3)
    );

    // a = ...
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, mul_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 6 (Неверное реляционное сравнение) [cite: 41, 296, 297]
 * Код:
 * static main() {
 * if ("a" < "b") { } // <-- ОШИБКА ЗДЕСЬ (только Num < Num)
 * }
 */
AstNode *create_test_invalid_relational_op_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // "a" < "b"
    AstNode *lt_op = ast_new_bin_op(NODE_OP_LT, 3,
        ast_new_string_node("a", 3),
        ast_new_string_node("b", 3)
    );

    // if (...) { } else { }
    AstNode *if_stmt = ast_node_create(NODE_IF, 3);
    ast_node_add_child(if_stmt, lt_op); // Условие
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3)); // if-блок
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3)); // else-блок
    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 4 (Редефиниция геттера) [cite: 39, 219]
 * Код:
 * static myVar { }
 * static myVar { } // <-- ОШИБКА ЗДЕСЬ
 */
AstNode *create_test_getter_redefinition_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myVar { }
    AstNode *getter1 = ast_new_id_node(NODE_GETTER_DEF, 1, "myVar");
    ast_node_add_child(getter1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, getter1);

    // static myVar { } <-- Ошибка 4
    AstNode *getter2 = ast_new_id_node(NODE_GETTER_DEF, 2, "myVar");
    ast_node_add_child(getter2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, getter2);

    // static main() { } (Нужен для запуска)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 4 (Редефиниция сеттера) [cite: 39, 219]
 * Код:
 * static myVar = (a) { }
 * static myVar = (b) { } // <-- ОШИБКА ЗДЕСЬ
 */
AstNode *create_test_setter_redefinition_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myVar = (a) { }
    AstNode *setter1 = ast_new_id_node(NODE_SETTER_DEF, 1, "myVar");
    ast_node_add_child(setter1, ast_new_id_node(NODE_PARAM, 1, "a"));
    ast_node_add_child(setter1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, setter1);

    // static myVar = (b) { } <-- Ошибка 4
    AstNode *setter2 = ast_new_id_node(NODE_SETTER_DEF, 2, "myVar");
    ast_node_add_child(setter2, ast_new_id_node(NODE_PARAM, 2, "b"));
    ast_node_add_child(setter2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, setter2);

    // static main() { } (Нужен для запуска)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}

/**
 * [cite_start]@brief Тест: OK (Валидная перегрузка) [cite: 191]
 * Код:
 * static foo() { }
 * static foo(a) { }
 * static main() {
 * foo()
 * foo(1)
 * }
 */
AstNode *create_test_valid_overloading() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static foo() { }
    AstNode *func_foo1 = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    ast_node_add_child(func_foo1, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(func_foo1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_foo1);

    // static foo(a) { }
    AstNode *func_foo2 = ast_new_id_node(NODE_FUNCTION_DEF, 2, "foo");
    AstNode *foo_params2 = ast_node_create(NODE_PARAM_LIST, 2);
    ast_node_add_child(foo_params2, ast_new_id_node(NODE_PARAM, 2, "a"));
    ast_node_add_child(func_foo2, foo_params2);
    ast_node_add_child(func_foo2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, func_foo2);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 4);
    ast_node_add_child(func_main, main_block);

    // foo()
    AstNode *call1 = ast_node_create(NODE_CALL_STATEMENT, 5);
    ast_node_add_child(call1, ast_new_id_node(NODE_ID, 5, "foo"));
    ast_node_add_child(call1, ast_node_create(NODE_ARGUMENT_LIST, 5));
    ast_node_add_child(main_block, call1);

    // foo(1)
    AstNode *call2 = ast_node_create(NODE_CALL_STATEMENT, 6);
    ast_node_add_child(call2, ast_new_id_node(NODE_ID, 6, "foo"));
    AstNode *arg_list2 = ast_node_create(NODE_ARGUMENT_LIST, 6);
    ast_node_add_child(arg_list2, ast_new_num_node(1.0, 6));
    ast_node_add_child(call2, arg_list2);
    ast_node_add_child(main_block, call2);

    return root;
}

/**
 * [cite_start]@brief Тест: OK (Валидное сравнение с null) [cite: 295]
 * Код:
 * static main() {
 * var a
 * if (a == null) { }
 * }
 */
AstNode *create_test_valid_null_comparison() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a == null
    AstNode *eq_op = ast_new_bin_op(NODE_OP_EQ, 4,
        ast_new_id_node(NODE_ID, 4, "a"),
        ast_new_null_node(4)
    );

    // if (...) { } else { }
    AstNode *if_stmt = ast_node_create(NODE_IF, 4);
    ast_node_add_child(if_stmt, eq_op);
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 4));
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 4));
    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * [cite_start]@brief Тест: OK (Валидное сравнение разных типов) [cite: 294]
 * Код:
 * static main() {
 * var a = 10
 * if (a == "hello") { } // (Всегда false, но валидно)
 * }
 */
AstNode *create_test_valid_mixed_type_equality() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = 10
    AstNode *assign = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign, ast_new_num_node(10.0, 4));
    ast_node_add_child(main_block, assign);

    // a == "hello"
    AstNode *eq_op = ast_new_bin_op(NODE_OP_EQ, 5,
        ast_new_id_node(NODE_ID, 5, "a"),
        ast_new_string_node("hello", 5)
    );

    // if (...) { } else { }
    AstNode *if_stmt = ast_node_create(NODE_IF, 5);
    ast_node_add_child(if_stmt, eq_op);
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 5));
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 5));
    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * [cite_start]@brief Тест: OK (Валидное объявление геттера, сеттера и функции) [cite: 220]
 * Код:
 * static val { }
 * static val = (newVal) { }
 * static val(a, b) { }
 * static main() { }
 */
AstNode *create_test_valid_getter_setter_func() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static val { }
    AstNode *getter = ast_new_id_node(NODE_GETTER_DEF, 1, "val");
    ast_node_add_child(getter, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, getter);

    // static val = (newVal) { }
    AstNode *setter = ast_new_id_node(NODE_SETTER_DEF, 2, "val");
    ast_node_add_child(setter, ast_new_id_node(NODE_PARAM, 2, "newVal"));
    ast_node_add_child(setter, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, setter);

    // static val(a, b) { }
    AstNode *func = ast_new_id_node(NODE_FUNCTION_DEF, 3, "val");
    AstNode *func_params = ast_node_create(NODE_PARAM_LIST, 3);
    ast_node_add_child(func_params, ast_new_id_node(NODE_PARAM, 3, "a"));
    ast_node_add_child(func_params, ast_new_id_node(NODE_PARAM, 3, "b"));
    ast_node_add_child(func, func_params);
    ast_node_add_child(func, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func);

    // static main() { } (Нужен для запуска)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 4, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 4));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 4));
    ast_node_add_child(root, func_main);

    return root;
}


/**
 * @brief Тест: Ошибка 6 (Вычитание строк)
 * Код:
 * static main() {
 * var a = "a" - "b" // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_string_subtraction_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    AstNode *sub_op = ast_new_bin_op(NODE_OP_MINUS, 3,
        ast_new_string_node("a", 3),
        ast_new_string_node("b", 3)
    );

    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, sub_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Деление с null)
 * Код:
 * static main() {
 * var a = 10 / null // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_division_by_null_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    AstNode *div_op = ast_new_bin_op(NODE_OP_DIV, 3,
        ast_new_num_node(10.0, 3),
        ast_new_null_node(3)
    );

    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, div_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Реляционное сравнение с null)
 * Код:
 * static main() {
 * if (null < 10) {} // <-- ОШИБКА ЗДЕСЬ (только == or !=)
 * }
 */
AstNode *create_test_null_relational_op_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    AstNode *lt_op = ast_new_bin_op(NODE_OP_LT, 3,
        ast_new_null_node(3),
        ast_new_num_node(10.0, 3)
    );

    AstNode *if_stmt = ast_node_create(NODE_IF, 3);
    ast_node_add_child(if_stmt, lt_op);
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(main_block, if_stmt);

    return root;
}

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

int old_semantics_test() {
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

void test_gen_code(){
    AstNode *root = create_test_valid_overloading();
    analyze_semantics(root);
    TACDLList tac_list;
    TACDLL_Init(&tac_list);
    generate_tac(root, &tac_list, &global_table);
    optimize_tac(&tac_list);
    print_tac_list(&tac_list);
    generate_code(&tac_list, &global_table);
    TACDLL_Dispose(&tac_list);
    symtable_free(&global_table);
}
#include "symtable.h"
#include "lexer.h"
#include <stdio.h>
/*
Игровая площадка для тестирования чего угодно
Пожалуйста, не удаляйте этот файл, он нам еще пригодится
Если хотите запустить этот файл:

make test-pg
*/

// Print token data safely, visualizing special characters
void print_token_data(const char *data) {
    printf("\"");
    for (const char *p = data; *p; p++) {
        switch (*p) {
            case '\n': printf("\\n"); break;
            case '\t': printf("\\t"); break;
            case '\r': printf("\\r"); break;
            default:   putchar(*p); break;
        }
    }
    printf("\"");
}



int test_lexer(){
    // Use stdin for input (supports redirection like: ./test < input.IFJ25)
    FILE *file = fopen("example.IFJ25", "r");
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
        get_token(lexer, file);

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
void test_all() {
    FILE *file = fopen("example.IFJ25", "r");
    //FILE *file = stdin;
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return;
    }

	// Запускаем парсер, передавая ему стандартный ввод
    AstNode *program = parser_run(file);
    if (program == NULL) {
        fprintf(stderr, "Parsing failed.\n");
        return;
    }
    ast_print_debug(program);
    analyze_semantics(program);
//    // symtable_print(&global_table);
//      TACDLList tac_list;
//      TACDLL_Init(&tac_list);
//      generate_tac(program, &tac_list, &global_table);
//     // // optimize_tac(&tac_list);
//     // print_tac_list(&tac_list);
//      generate_code(&tac_list, &global_table);
//      TACDLL_Dispose(&tac_list);
//      symtable_free(&global_table);
//      ast_node_free_recursive(program);
//     fclose(file);
}



int main() {

    test_all();
  //test_lexer();
    
/*
    FILE *file = fopen("example.IFJ25", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return 0;
    }
        */
    // Lexer *lexer = lexer_init();
    // Token token;

    // do {
    //     token = peek_token(lexer, file);
    //     printf("Token Type: %s, Data: %s, Line: %d\n",
    //             token_type_to_string(token.type),
    //            token.data != NULL ? token.data : "NULL",
    //            token.line);
    //     get_token(lexer, file); // consume the token

    // } while (token.type != TOKEN_EOF);
    // lexer_free(lexer);
    // if (fclose(file) != 0) { // обработка ошибки закрытия файла
    //     fprintf(stderr, "Error closing file.\n");
    // }
	// Запускаем парсер, передавая ему стандартный ввод
   /* AstNode *program = parser_run(file);
    if (program == NULL) {
        fprintf(stderr, "Parsing failed.\n");
        return 0;
    }
    ast_print_debug(program);
    ast_node_free_recursive(program);

*/
    return 0;
}