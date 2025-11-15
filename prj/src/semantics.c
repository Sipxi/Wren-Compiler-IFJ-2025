/** ss
 * semantics.c - Семантический анализ
 * ШАГ 0 - подготовка, заполнение глобальной таблицы символов встроенными функциями
 * ШАГ 2A - сбор функций, проход по АСТ добавить в global_table
 * ШАГ 2Б - семантический анализ тела функций
 */
// TODO Убрать всю хуйню которую я написал в перерывах
// TODO Коды ошибок

#include "symtable.h"
#include "ast.h"
#include "scopemap.h"
#include "scopestack.h"
#include <stdio.h> 
#include <stdlib.h>

Symtable global_table; // Глобальная таблица символов

/**
 * Главная функция семантического анализатора.
 */
bool analyze_semantics(AstNode* root) {
    
    // 1. Инициализируем нашу global_table
    if (!symtable_init(&global_table)) {
        // Это внутренняя ошибка компилятора (Ошибка 99)
        fprintf(stderr, "Internal Compiler Error: Failed to init global_table.\n");
        return false;
    }

    //* --- ШАГ 0: Регистрация Встроенных Функций ---

    // static Ifj.read_str() -> String | Null
    if (!register_builtin_function("Ifj.read_str", 0, TYPE_STR)) return false;
    
    // static Ifj.read_num() -> Num | Null
    if (!register_builtin_function("Ifj.read_num", 0, TYPE_NUM)) return false;

    // static Ifj.write(term) -> Null
    if (!register_builtin_function("Ifj.write", 1, TYPE_NIL)) return false;

    // static Ifj.floor(term: Num) -> Num
    if (!register_builtin_function("Ifj.floor", 1, TYPE_NUM)) return false;

    // static Ifj.substring(s: String, i: Num, j: Num) -> String | Null
    if (!register_builtin_function("Ifj.substring", 3, TYPE_STR)) return false;

    // static Ifj.strcmp(s1: String, s2: String) -> Num
    if (!register_builtin_function("Ifj.strcmp", 2, TYPE_NUM)) return false;

    // static Ifj.ord(s: String, i: Num) -> Num
    if (!register_builtin_function("Ifj.ord", 2, TYPE_NUM)) return false;

    // static Ifj.chr(i: Num) -> String
    if (!register_builtin_function("Ifj.chr", 1, TYPE_STR)) return false;

    // static Ifj.str(term) -> String
    if (!register_builtin_function("Ifj.str", 1, TYPE_STR)) return false;

    // static Ifj.length(s: String) -> Num
    if (!register_builtin_function("Ifj.length", 1, TYPE_NUM)) return false;

    printf("DEBUG: Step 0 completed. Global tagit ble populated with ALL built-in functions.\n");



    //* --- ШАГ 2a: Сбор Пользовательских Функций ---

    if (root == NULL || root->type != NODE_PROGRAM) {
        fprintf(stderr, "Internal Error: AST root is not NODE_PROGRAM.\n");
        symtable_free(&global_table);
        return false;
    }

    // 2. Идем по всем дочерним узлам NODE_PROGRAM
    for (AstNode* node = root->child; node != NULL; node = node->sibling) {
        // Нас интересуют только определения функций
        if (node->type == NODE_FUNCTION_DEF || 
            node->type == NODE_GETTER_DEF || 
            node->type == NODE_SETTER_DEF) 
        {
            if (!process_function_declaration(node)) {
                // Ошибка
                symtable_free(&global_table);
                return false;
            }
        }
    }
    
    printf("DEBUG: Step 2a completed. User functions collected.\n");



    //* --- ШАГ 2b: Анализ Тел Функций ---

    // 1. Снова проходим по всем узлам функций
    for (AstNode* node = root->child; node != NULL; node = node->sibling) {
        if (node->type == NODE_FUNCTION_DEF || 
            node->type == NODE_GETTER_DEF || 
            node->type == NODE_SETTER_DEF) 
        {
            // 2. Вызываем новую функцию для анализа ТЕЛА
            if (!analyze_function_body(node)) {
                symtable_free(&global_table);
                return false;
            }
        }
    }

    printf("DEBUG: Step 2b completed. All function bodies analyzed.\n");

    //* --- Финальная Проверка: main@0 ---
    
    // 3. Проверяем, что 'main' без параметров существует
    TableEntry* main_entry = symtable_lookup(&global_table, "main@0");

    if (main_entry == NULL) {
        fprintf(stderr, "Semantic Error: Function 'main()' is not defined.\n");
        symtable_free(&global_table);
        return false;
    }

    // убедимся, что она была определена (т.е. у нее было тело)
    if (main_entry->data->is_defined == false) {
        fprintf(stderr, "Semantic Error: Function 'main()' is declared but not defined.\n");
        symtable_free(&global_table);
        return false;
    }

    printf("DEBUG: Final check completed. 'main@0' found and defined.\n");

    // В конце чистим
    symtable_free(&global_table);
    return true;
}


/**
 * Вспомогательная функция для регистрации встроенной функции в global_table.
 *
 * @param name Имя функции (напр. "Ifj.write")
 * @param arity Количество параметров
 * @param return_type Тип возвращаемого значения (напр. TYPE_NIL)
 */
static bool register_builtin_function(const char* name, int arity, DataType return_type) {
    
    // 1. Создаем "искаженное имя"
    char mangled_name[256];
    sprintf(mangled_name, "%s@%d", name, arity);

    // 2. Создаем SymbolData для этой функции
    SymbolData data;
    data.kind = KIND_FUNC;       // Это функция
    data.data_type = return_type; // Что она возвращает
    data.is_defined = true;      // Встроенные функции всегда "определены"
    data.local_table = NULL;     // У них нет локальной таблицы IFJ25

    // 3. Вставляем в global_table
    if (!symtable_insert(&global_table, mangled_name, &data)) {
        // Ошибка 99
        fprintf(stderr, "Internal Error: Failed to insert builtin '%s'\n", mangled_name);
        return false;
    }

    return true;
}


/**
 * Вспомогательная функция для подсчета параметров функции.
 * @param func_node Узел NODE_FUNCTION_DEF (или SETTER).
 * @return Количество параметров.
 */
static int count_parameters(AstNode* func_node) {
    if (func_node->type == NODE_SETTER_DEF) {
        return 1; // Сеттер всегда имеет 1 параметр
    }
    if (func_node->type == NODE_GETTER_DEF) {
        return 0; // Геттер всегда имеет 0 параметров
    }

    // Для NODE_FUNCTION_DEF 
    AstNode* param_list = func_node->child;
    if (param_list == NULL || param_list->type != NODE_PARAM_LIST) {
        // Это геттер или тело функции, у него нет списка параметров
        return 0; 
    }

    int arity = 0;
    // Идем по списку NODE_PARAM
    for (AstNode* param = param_list->child; param != NULL; param = param->sibling) {
        if (param->type == NODE_PARAM) {
            arity++;
        }
    }
    return arity;
}


/**
 * (Шаг 2a) Обрабатывает одно объявление функции из AST.
 * Находит имя, арность, проверяет на ре-дефиницию,
 * создает local_table и вставляет в global_table.
 *
 * @param func_node Узел (NODE_FUNCTION_DEF, GETTER, SETTER).
 * @return true в случае успеха, false при ошибке.
 */
static bool process_function_declaration(AstNode* func_node) {
    
    // 1. Получаем Имя
    const char* name = func_node->data.identifier;
    if (name == NULL) {
        fprintf(stderr, "Internal Error: Function node has no name.\n");
        return false;
    }

    // 2. Считаем Арность
    int arity = count_parameters(func_node);

    // 3. Создаем "Mangled Name" (Ключ)
    char mangled_name[256];
    if (func_node->type == NODE_SETTER_DEF) {
        //Сеттер и геттер имеет внутри "="б поэтому что бы не путать с функцией которая может
        // иметь такое же имя и арность дадим им уникальный суффикс 
        sprintf(mangled_name, "%s@setter", name); // e.g., "myVar@setter"
    } else if (func_node->type == NODE_GETTER_DEF){
        sprintf(mangled_name, "%s@getter", name); // e.g., "myVar@getter"
    } else {
        sprintf(mangled_name, "%s@%d", name, arity); // e.g., "foo@1"
    }

    // 4. Проверка (Ошибка 4 - Ре-дефиниция)
    if (symtable_lookup(&global_table, mangled_name) != NULL) {
        // Эта функция уже была определена.
        fprintf(stderr, "Semantic Error (Line %d): Redefinition of function '%s' with %d parameters.\n",
                func_node->line_number, name, arity);
        return false; 
    }

    // 5. Создаем SymbolData
    SymbolData data;
    data.kind = KIND_FUNC;
    data.data_type = TYPE_NIL; // По умолчанию, функция возвращает nil
    data.is_defined = false;   // Мы еще НЕ анализировали тело (смотреть 2б)
    
    // 6. Создаем 'local_table' для этой функции.
    // Это *постоянное* хранилище для Pass 3.
    data.local_table = (Symtable*)malloc(sizeof(Symtable));
    if (data.local_table == NULL) {
        fprintf(stderr, "Internal Error: Failed to malloc local_table for '%s'.\n", name);
        return false;
    }
    if (!symtable_init(data.local_table)) {
        fprintf(stderr, "Internal Error: Failed to init local_table for '%s'.\n", name);
        free(data.local_table);
        return false;
    }

    // 7. Вставляем в global_table
    if (!symtable_insert(&global_table, mangled_name, &data)) {
        fprintf(stderr, "Internal Error: Failed to insert function '%s' into global_table.\n", name);
        symtable_free(data.local_table); // Очищаем то, что создали
        free(data.local_table);
        return false;
    }
    
    // 8. Связываем узел AST с этой TableEntry
    // 
    // Мы только что вставили, так что можем сразу найти
    func_node->table_entry = symtable_lookup(&global_table, mangled_name);

    return true;
}



/**
 * (Шаг 2b) Анализирует тело ОДНОЙ функции.
 *
 * Эта функция "подготавливает площадку" для рекурсивного семантического
 * анализа. Ее задачи:
 *
 * 1.  Получить Контекст: Взять `func_entry` (из `global_table`) и
 * `func_local_table` (напоминаю - постоянное хранилище для Pass 3),
 * которые были созданы в Шаге 2a.
 *
 * 2.  Инициализировать Инструменты: Создать временный `ScopeStack`
 * (для отслеживания областей видимости в Pass 2) и `mangling_counter`
 * (для создания хуйни-имен типа `a_1`, `a_2`).
 *
 * 3.  Войти в Область Функции: Создать `ScopeMap` для верхнего уровня
 * функции (параметров) и поместить (`Push`) его в `ScopeStack`.
 *
 * 4.  Найти Узлы: Найти два ключевых дочерних узла, основываясь
 * на `ast.h`:
 * - `param_iter`: Указатель на первый `NODE_PARAM` (или NULL).
 * - `body_node`: Указатель на `NODE_BLOCK` тела.
 * Логика поиска разная для `NODE_FUNCTION_DEF`, `NODE_SETTER_DEF`
 * и `NODE_GETTER_DEF`.
 *
 * 5.  Обработать Параметры (Цикл `for`):
 * - Проверка (Ошибка 4): Искать `param_name` в `scopemap_lookup(top_map, ...)`
 * (проверка на дубликат *только* в текущей области).
 * - Хранение (Pass 3): Вставить `param_name` (как `KIND_VAR`)
 * в *постоянную* `func_local_table`.
 * - Поиск (Pass 2): Вставить указатель на эту `TableEntry`
 * в *временный* `top_map` (`scopemap_insert`).
 * - Связывание AST Заполнить `param->table_entry`, чтобы узел AST
 * указывал на запись в `func_local_table`.
 *
 * 6.  Запустить Анализ Тела: Вызвать `analyze_statement(body_node, ...)`
 * для рекурсивного анализа — дальше бога нет.
 *
 * 7.  Пометить как "Определенную": Люто заебались но анализ прошел успешно,
 * установить `func_entry->data->is_defined = true`.
 *
 * 8.  очистка ну это и так понятно: Освободить временный `ScopeStack` (`ScopeStack_Free`).
 *
 * @param func_node Узел (NODE_FUNCTION_DEF, GETTER, SETTER).
 * @return true в случае успеха, false при семантической ошибке.
 */
static bool analyze_function_body(AstNode* func_node)
{
    printf("DEBUG: Analyzing body for '%s'...\n", func_node->data.identifier);

    // 1. Получаем 'func_entry' (из Шага 2a)
    TableEntry* func_entry = func_node->table_entry;
    if (func_entry == NULL) {
        // Этого не должно случиться, если Шаг 2a прошел успешно
        fprintf(stderr, "Internal Error: func_node->table_entry is NULL for '%s'.\n", func_node->data.identifier);
        return false;
    }

    // 2. Получаем 'func_local_table' (из Шага 2a)
    Symtable* func_local_table = func_entry->data->local_table;
    if (func_local_table == NULL) {
        fprintf(stderr, "Internal Error: func_local_table is NULL for '%s'.\n", func_node->data.identifier);
        return false;
    }

    // 3. Создаем временный ScopeStack
    ScopeStack stack;
    if (!ScopeStack_Init(&stack)) {
        fprintf(stderr, "Internal Error: Failed to init ScopeStack for '%s'.\n", func_node->data.identifier);
        return false;
    }

    // Счетчик для "искаженных имен" (напр., a_1, a_2),
    // сбрасывается для каждой функции.
    int mangling_counter = 0;

    // 4. Создаем ScopeMap для этой (верхней) области видимости
    ScopeMap* top_map = scopemap_create();
    if (top_map == NULL) {
        ScopeStack_Free(&stack);
        return false;
    }

    // 5. Помещаем его в 'stack'
    if (!ScopeStack_Push(&stack, top_map)) {
        scopemap_free(top_map);
        ScopeStack_Free(&stack);
        return false;
    }

    // 6. Определяем, где начало списка параметров и где тело
    AstNode* param_iter = NULL; // По умолчанию NULL (как для геттера)
    AstNode* body_node = NULL;  // По умолчанию NULL

    // --- СЛУЧАЙ 1: 'static foo(a,b) { ... }' ---
    if (func_node->type == NODE_FUNCTION_DEF) {
        AstNode* param_list = func_node->child; // Это NODE_PARAM_LIST
        
        if (param_list != NULL && param_list->type == NODE_PARAM_LIST) {
            param_iter = param_list->child;   // Указатель на первый NODE_PARAM 
            body_node = param_list->sibling;  // Указатель на NODE_BLOCK
        } else {
            fprintf(stderr, "Internal Error: NODE_FUNCTION_DEF has malformed children.\n");
            ScopeStack_Free(&stack);
            return false;
        }
    }
    // --- СЛУЧАЙ 2: 'static foo = (a) { ... }' ---
    else if (func_node->type == NODE_SETTER_DEF) {
        param_iter = func_node->child;   // Указатель на NODE_PARAM 
        body_node = func_node->child->sibling; // Указатель на NODE_BLOCK 
    } 
    // --- СЛУЧАЙ 3: 'static foo { ... }' ---
    else if (func_node->type == NODE_GETTER_DEF) {
        param_iter = NULL; // Параметров нет
        body_node = func_node->child; // Указатель на NODE_BLOCK
    }

    // 6. (Часть 2) Обрабатываем параметры.
    // Этот цикл корректно пропустится для геттеров (param_iter == NULL).
    for (AstNode* param = param_iter; param != NULL; param = param->sibling) {
        
        if (param->type != NODE_PARAM) continue; // На всякий случай

        const char* param_name = param->data.identifier;

        // 6a. Проверка (Ошибка 4 - дубликат параметра)
        // Ищем *только* в верхнем Map (top_map)
        if (scopemap_lookup(top_map, param_name) != NULL) {
            fprintf(stderr, "Semantic Error (Line %d): Duplicate parameter name '%s'.\n",
                    param->line_number, param_name);
            ScopeStack_Free(&stack); // Очистит top_map
            return false;
        }

        // 6b. Создаем данные для Symtable
        SymbolData data;
        data.kind = KIND_VAR;
        data.data_type = TYPE_NIL; // Изначально тип неизвестен
        data.is_defined = true;    // Параметры всегда "определены"
        data.local_table = NULL;

        // 6c. Вставляем в *постоянную* func_local_table (для Pass 3)
        if (!symtable_insert(func_local_table, param_name, &data)) {
            fprintf(stderr, "Internal Error: Failed to insert param '%s' into func_local_table.\n", param_name);
            ScopeStack_Free(&stack);
            return false;
        }

        // 6d. Связываем временный Map с постоянной TableEntry (для Pass 2)
        TableEntry* entry = symtable_lookup(func_local_table, param_name);
        if (!scopemap_insert(top_map, param_name, entry)) {
            fprintf(stderr, "Internal Error: Failed to insert param '%s' into top_map.\n", param_name);
            ScopeStack_Free(&stack);
            return false;
        }

        // 6e. Связываем узел AST
        param->table_entry = entry;
    }


    // 7. Вызываем analyze_statement для ТЕЛА функции
    if (body_node == NULL || body_node->type != NODE_BLOCK) {
         // Синтаксическая ошибка 2 (или 99), парсер не должен был этого допустить
        fprintf(stderr, "Internal Error: Function '%s' has no NODE_BLOCK body.\n", func_node->data.identifier);
        ScopeStack_Free(&stack);
        return false;
    }
    
    // Запускаем рекурсивный анализ
    if (!analyze_statement(body_node, func_local_table, &stack, &mangling_counter)) {
        ScopeStack_Free(&stack);
        return false;
    }

    // 8. Если мы дошли сюда без ошибок:
    func_entry->data->is_defined = true;
    
    // 9. Очищаем временный стек
    ScopeStack_Free(&stack);
    
    return true;
}

/*
        Young dope dealer, sellin' dope, is you like that? (If you like that, yeah, yeah)
        Kickin' doors, kickin' in doors, is you like that? (How?, yeah)
        Young throwed nigga, sellin' lows, is you like that? (Holy water, Holy water, yeah)
        All 24, you on go, is you like that? (If you like that)
        Niggas from the bottom really like that (if you like that) (he was once a thug, he was, he-)
        Steppin' in Balencis if you like that (if you like that) (he was once a thug, he was, he-)
        Pop another bottle if you like that (if you like that) (he was once a thug, he was, he-)

    (c) "Like That" - Future, Kendrick Lamar, and Metro Boomin (2024)
*/


/**
 * (Шаг 2b) Рекурсивно анализирует узел-оператор (statement).
 *
 * Как любит говорить джеминай "это СЕРДЦЕ (имя вашей хуйни)", в нашем случае Pass 2b. Она рекурсивно обходит
 * блоки, определяет переменные, обрабатывает присваивания, условия и вызывает analyze_expression для выражений.
 * Все через кейсы.
 *
 * @param node Узел AST для анализа (напр., NODE_BLOCK, NODE_VAR_DEF).
 * @param func_local_table *Постоянная* таблица символов функции (для Pass 3).
 * @param stack *Временный* стек областей видимости (для Pass 2).
 * @param mangling_counter Указатель на счетчик для уникальных имен (напр. a_1).
 * @return true в случае успеха, false при семантической ошибке.
 */
static bool analyze_statement(AstNode* node, Symtable* func_local_table, ScopeStack* stack, int* mangling_counter)
{
    // Узел не должен быть NULL, но если он есть это не семантическая ошибка, а просто конец списка.
    if (node == NULL) {
        return true;
    }

    switch (node->type) {

        //* --- СЛУЧАЙ 1: Блок { ... } ---
        // Его функция по сути ТУПА создать новую область видимости и рекурсивно вызвать другие кейсы
        case NODE_BLOCK: {
            printf("DEBUG: Entering NODE_BLOCK.\n");
            
            // 1. Создаем новую область видимости
            ScopeMap* new_scope = scopemap_create();
            if (new_scope == NULL) {
                return false;
            }
            
            // 2. Помещаем ее в стек
            if (!ScopeStack_Push(stack, new_scope)) {
                scopemap_free(new_scope);
                return false;
            }

            // 3. Рекурсивно анализируем *все* операторы внутри блока
            bool result = true;
            for (AstNode* stmt = node->child; stmt != NULL; stmt = stmt->sibling) {
                if (!analyze_statement(stmt, func_local_table, stack, mangling_counter)) {
                    result = false;
                    break; // Прерываем при первойошибке
                }
            }

            // 4. Выходим из области видимости:
            // Pop'аем нашу ScopeMap и уничтожаем ее.
            ScopeMap* popped_map = ScopeStack_Pop(stack);
            scopemap_free(popped_map);

            printf("DEBUG: Exiting NODE_BLOCK.\n");
            return result; // Возвращаем true/false в зависимости от успеха
        }

        //* --- СЛУЧАЙ 2: Определение 'var id' ---
        case NODE_VAR_DEF: {
            printf("DEBUG: Entering NODE_VAR_DEF (%s).\n", node->data.identifier);
            
            // Берем имя (напр. 'b')
            const char* name = node->data.identifier;
            ScopeMap* current_scope = ScopeStack_Peek(stack);
            
            if (current_scope == NULL) {
                // Этого не должно случиться, стек не должен быть пустым
                return false;
            }

            // 1. Проверка (Ошибка 4 - Ре-дефиниция в *этом* блоке)
            if (scopemap_lookup(current_scope, name) != NULL) {
                fprintf(stderr, "Semantic Error (Line %d): Redefinition of variable '%s' in the same scope.\n",
                        node->line_number, name);
                return false;
            }

            // 2. Создаем "Искаженное Имя"
            char mangled_name[256];
            // Проверяем глубину стека. > 1 означает, что мы внутри { }
            if (stack->topIndex > 0) { 
                // Мы в блоке, создаем имя "name_N"
                sprintf(mangled_name, "%s_%d", name, (*mangling_counter));
                (*mangling_counter)++; // Увеличиваем счетчик
            } else {
                // Мы на верхнем уровне функции (topIndex == 0)
                strcpy(mangled_name, name);
            }

            // 3. Хранение (Pass 3): Вставляем в *постоянную* таблицу
            SymbolData data;
            data.kind = KIND_VAR;
            data.data_type = TYPE_NIL; // По умолчанию 'var a' имеет значение 'null'
            data.is_defined = true;    // Она определена
            data.local_table = NULL;

            if (!symtable_insert(func_local_table, mangled_name, &data)) {
                return false;
            }
            
            // 4. Поиск (Pass 2): Вставляем во *временную* карту
            TableEntry* entry = symtable_lookup(func_local_table, mangled_name);
            if (entry == NULL || !scopemap_insert(current_scope, name, entry)) {
                return false;
            }
            
            // 5. Связывание AST (для Pass 3)
            node->table_entry = entry;

            return true;
        }

        //* --- СЛУЧАЙ 3: Присваивание 'id = ...' ---
        case NODE_ASSIGNMENT: {
            printf("DEBUG: Entering NODE_ASSIGNMENT.\n");

            AstNode* id_node = node->child;
            AstNode* expr_node = node->child->sibling;

            // 1. Анализируем правую часть (выражение)
            DataType expr_type;
            if (!analyze_expression(expr_node, func_local_table, stack, mangling_counter, &expr_type)) {
                return false;
            }

           if (expr_type == TYPE_BOOL) {
                fprintf(stderr, "Semantic Error (Line %d): Cannot assign a truth value to a variable.\n",
                        expr_node->line_number);
                // ВОЗВРАЩАЕМ КОД ОШИБКИ 6
                return false;
            }
            
            // 2. Ищем левую часть (КУДА присваивать).
            const char* name = id_node->data.identifier; 
            TableEntry* entry = NULL;

            // 2a. Ищем в локальных областях (локальная 'a')
            entry = ScopeStack_Find_Var(stack, name);

            // 2b. Если не нашли, ищем СЕТТЕР ('a@setter')
            if (entry == NULL) {
                char mangled_name[256];
                sprintf(mangled_name, "%s@setter", name);
                entry = symtable_lookup(&global_table, mangled_name);
            }

            // 2c. Если не нашли, ищем Глобальную переменную ('__a')
            if (entry == NULL) {
                if (strncmp(name, "__", 2) == 0) {
                    entry = symtable_lookup(&global_table, name);
                }
            }

            // 3. Проверка (Ошибка 3)
            if (entry == NULL) {
                fprintf(stderr, "Semantic Error (Line %d): Cannot assign to undefined variable or setter '%s'.\n",
                        id_node->line_number, name);
                // ВОЗВРАЩАЕМ КОД ОШИБКИ
                return false;
            }
            
            // 4. Проверка (Присваивание ФУНКЦИИ или ГЕТТЕРУ?)
            if (entry->data->kind == KIND_FUNC) {
                char setter_name[256];
                sprintf(setter_name, "%s@setter", name);
                // Если 'kind' - функция, но ключ НЕ 'name@setter',
                // значит, мы пытаемся присвоить Геттеру или Функции.
                if (strcmp(entry->key, setter_name) != 0) {
                    fprintf(stderr, "Semantic Error (Line %d): Cannot assign to read-only function or getter '%s'.\n", id_node->line_number, name);
                    return false;
                }
            }
            
            // 5. Связываем AST
            id_node->table_entry = entry;

            // 6. Обновляем Тип (Только для ПЕРЕМЕННЫХ)
            // Мы НЕ обновляем тип для Сеттера
            if (entry->data->kind == KIND_VAR) {
                entry->data->data_type = expr_type;
            }

            return true;
        }

        // НЕ УДАЛЯТЬ ИНАЧЕ КОД НЕ РАБОТАЕТ
        /*
        Го́споди Иису́есе Христе́, Сы́не Бо́жий, молитв ра́ди Пречи́стыя Твоея́ Ма́тере и всех святы́х, поми́луй нас. Ами́нь.
        Сла́ва Тебе́, Бо́же наш, сла́ва Тебе́.
        Царю́ Небе́сный, Уте́шителю, Ду́ше и́стины, И́же везде́ сый и вся исполня́яй,
        Сокро́вище благи́х и жи́зни Пода́телю, прииди́ и всели́ся в ны, и очи́сти ны
        от вся́кия скве́рны, и спаси́, Бла́же, ду́ши на́ша.
        */

        //* --- СЛУЧАЙ 4: Условие 'if (cond) { ... } else { ... }' ---
        case NODE_IF: {
            printf("DEBUG: Entering NODE_IF.\n");
            
            AstNode* cond_node = node->child;
            AstNode* if_body = node->child->sibling;
            AstNode* else_body = node->child->sibling->sibling;
            
            // 1. Анализируем Условие
            DataType cond_type; // Нам не важен тип, но мы должны проверить на ошибки
            if (!analyze_expression(cond_node, func_local_table, stack, mangling_counter, &cond_type)) {
                return false; // Ошибка
            }

            // 2. Анализируем 'if' тело
            if (!analyze_statement(if_body, func_local_table, stack, mangling_counter)) {
                return false;
            }

            // 3. Анализируем 'else' тело
            // В базовом задании 'else' всегда есть
            if (!analyze_statement(else_body, func_local_table, stack, mangling_counter)) {
                return false;
            }
            
            return true;
        }

        //* --- СЛУЧАЙ 5: Цикл 'while (cond) { ... }' ---
        case NODE_WHILE: {
            printf("DEBUG: Entering NODE_WHILE.\n");

            AstNode* cond_node = node->child;
            AstNode* while_body = node->child->sibling; // 

            // 1. Анализируем Условие
            DataType cond_type;
            if (!analyze_expression(cond_node, func_local_table, stack, mangling_counter, &cond_type)) {
                return false; // Ошибка 
            }

            // 2. Анализируем Тело
            if (!analyze_statement(while_body, func_local_table, stack, mangling_counter)) {
                return false;
            }

            return true;
        }

        // --- СЛУЧАЙ 6: Возврат 'return ...' ---
        case NODE_RETURN: {
            printf("DEBUG: Entering NODE_RETURN.\n");

            AstNode* expr_node = node->child;
            // 1. Анализируем возвращаемое выражение
            DataType return_type;
            if (!analyze_expression(expr_node, func_local_table, stack, mangling_counter, &return_type)) {
                // Ошибка
                return false;
            }
            return true;
        }

        //* --- СЛУЧАЙ 7: Самостоятельный вызов 'id(...)' ---
        case NODE_CALL_STATEMENT: {
            printf("DEBUG: Entering standalone NODE_CALL_STATEMENT (%s).\n", node->data.identifier);

            DataType return_type; // Мы не используем, но функция должна ее вернуть

            // Просто анализируем выражение, чтобы проверить его на ошибки
            // (Error 3, 5, 6)
            if (!analyze_expression(node, func_local_table, stack, mangling_counter, &return_type)) {
                return false;
            }
            return true;
    }
        
        
        default:
            return true;
    }
}


/**
 * (Шаг 2b) Рекурсивно анализирует узел-выражение.
 *
 * Определяет тип выражения, проверяет на ошибки
 * и связывает узлы ID с их записями в TableEntry.
 *
 * @param node Узел AST (напр., NODE_OP_PLUS, NODE_ID, NODE_LITERAL_NUM).
 * @param func_local_table *Постоянная* таблица символов функции (для Pass 3).
 * @param stack *Временный* стек областей видимости (для Pass 2).
 * @param mangling_counter Указатель на счетчик для уникальных имен (напр., a_1).
 * @param result_type [out] Указатель, куда будет записан тип результата.
 * @return true в случае успеха, false при семантической ошибке.
 */
static bool analyze_expression(AstNode* node, Symtable* func_local_table, ScopeStack* stack, int* mangling_counter, DataType* result_type)
{
    // Если узел пустой (напр., 'return' без выражения)
    //? это расширение, но в базе 'return' всегда с выражением
    if (node == NULL) {
        *result_type = TYPE_NIL;
        return true; 
    }

    switch (node->type)
    {
        //* --- БАЗОВЫЕ СЛУЧАИ (Литералы) ---
        case NODE_LITERAL_NUM:
            node->data_type = TYPE_NUM;
            *result_type = TYPE_NUM;
            return true;

        case NODE_LITERAL_STRING:
            node->data_type = TYPE_STR;
            *result_type = TYPE_STR;
            return true;

        case NODE_LITERAL_NULL:
            node->data_type = TYPE_NIL;
            *result_type = TYPE_NIL;
            return true;

        //* --- БАЗОВЫЙ СЛУЧАЙ (Идентификатор) ---
        case NODE_ID: {
            printf("DEBUG: Analyzing NODE_ID (%s).\n", node->data.identifier);
            const char* name = node->data.identifier;

            // 1. Ищем в локальных областях (локальная 'a')
            TableEntry* entry = ScopeStack_Find_Var(stack, name);

            // 2. Если не нашли, ищем ГЕТТЕР ('a@getter')
            if (entry == NULL) {
                char mangled_name[256];
                sprintf(mangled_name, "%s@getter", name);
                entry = symtable_lookup(&global_table, mangled_name);
            }
            
            // 3. Если не нашли, ищем Глобальную переменную ('__a')
            if (entry == NULL) {
                if (strncmp(name, "__", 2) == 0) {
                    entry = symtable_lookup(&global_table, name);
                }
            }

            // 4. Проверка (Ошибка 3)
            if (entry == NULL) {
                fprintf(stderr, "Semantic Error (Line %d): Use of undefined variable or getter '%s'.\n",
                        node->line_number, name);
                // ВОЗВРАЩАЕМ КОД ОШИБКИ
                return false;
            }
            
            // 5. Проверка (Использование ФУНКЦИИ или СЕТТЕРА?)
            if (entry->data->kind == KIND_FUNC) {
                char getter_name[256];
                sprintf(getter_name, "%s@getter", name);
                // Если 'kind' - функция, но ключ НЕ 'name@getter',
                // значит, мы пытаемся прочитать Сеттер или Функцию.
                if (strcmp(entry->key, getter_name) != 0) {
                     fprintf(stderr, "Semantic Error (Line %d): Cannot use function or setter '%s' as a variable.\n", node->line_number, name);
                    return false;
                }
            }

            // 6. Связываем AST
            node->table_entry = entry;
            
            // 7. Устанавливаем и возвращаем тип
            node->data_type = entry->data->data_type;
            *result_type = entry->data->data_type;
            
            return true;
        }

        // ... (case'ы для Литералов и NODE_ID) ...
        //* --- РЕКУРСИВНЫЕ СЛУЧАИ (Операторы) ---
        
        case NODE_OP_PLUS:
        case NODE_OP_MINUS:
        case NODE_OP_MUL:
        case NODE_OP_DIV:
        {
            printf("DEBUG: Analyzing Binary Op (%d).\n", node->type);
            
            // 1. Анализируем левую часть
            DataType left_type;
            if (!analyze_expression(node->child, func_local_table, stack, mangling_counter, &left_type)) {
                return false; // Ошибка в левом под-выражении
            }

            // 2. Анализируем правую часть
            DataType right_type;
            if (!analyze_expression(node->child->sibling, func_local_table, stack, mangling_counter, &right_type)) {
                return false; // Ошибка в правом под-выражении
            }
        
            // 3. Проверка Типов
            // 3a. Проверка на NIL (нельзя использовать в арифметике)   
            if (left_type == TYPE_NIL || right_type == TYPE_NIL) {
                 fprintf(stderr, "Semantic Error (Line %d): Cannot use 'null' in arithmetic or string operations.\n",
                        node->line_number);
                // ВОЗВРАЩАЕМ КОД ОШИБКИ
                return false;
            }
        
            // 3b. Проверка для конкретного оператора
            switch (node->type)
            {
                // --- Сложение / Конкатенация ---
                case NODE_OP_PLUS:
                    if (left_type == TYPE_NUM && right_type == TYPE_NUM) {
                        *result_type = TYPE_NUM; // Число + Число
                    } 
                    else if (left_type == TYPE_STR && right_type == TYPE_STR) {
                        *result_type = TYPE_STR; // Строка + Строка 
                    }
                    else {
                        // Все остальное (NUM + STR, STR + NUM) - ошибка
                        fprintf(stderr, "Semantic Error (Line %d): Invalid operands for '+'. Must be Num+Num or String+String.\n", node->line_number);
                        // ВОЗВРАЩАЕМ КОД ОШИБКИ
                        return false;
                    }
                    break;

                // --- Вычитание ---
                case NODE_OP_MINUS:
                    if (left_type == TYPE_NUM && right_type == TYPE_NUM) {
                        *result_type = TYPE_NUM; // Только Число - Число
                    } else {
                        fprintf(stderr, "Semantic Error (Line %d): Invalid operands for '-'. Must be Num-Num.\n", node->line_number);
                        // ВОЗВРАЩАЕМ КОД ОШИБКИ
                        return false;
                    }
                    break;
                
                // --- Умножение / Итерация ---
                case NODE_OP_MUL:
                    if (left_type == TYPE_NUM && right_type == TYPE_NUM) {
                        *result_type = TYPE_NUM; // Число * Число
                    }
                    else if (left_type == TYPE_STR && right_type == TYPE_NUM) {
                        *result_type = TYPE_STR; // Строка * Число (Итерация)
                        // Здесь нужна проверка, что правый 'Num' - целое.
                        // Это часть расширения STATICAN вроде
                    }
                    else {
                        fprintf(stderr, "Semantic Error (Line %d): Invalid operands for '*'. Must be Num*Num or String*Num.\n", node->line_number);
                        // ВОЗВРАЩАЕМ КОД ОШИБКИ
                        return false;
                    }
                    break;

                /* Шел 3 день написания семантического анализа. Алкоголь закончился где-то 4 часа назад. Ахуеть я на бутылку Джека Дениелса
                   вьебал 500 крон. И это по скидке нахуй. Ну ладно, там не скидка, просто теско ублюдки завышают цену а потом 
                   типа с клабкартой дешевле получается. Вообще пиздатая штука, только я еще ни одного бода не потратил с нее. Но, возращаясь
                   к теме, я не знаю, смогу ли простить свою команду, за то что свалили эту хуйню на меня. Хотя кого я обманываю?
                   Я сам виноват во всем, что даже нормально по клавиутуре не попадаю. Блядь, она была единственная, кого я по настоящему любил... пометка для себя на будующее: записаться к психотерапевту
                */

                // --- Деление ---
                case NODE_OP_DIV:
                    if (left_type == TYPE_NUM && right_type == TYPE_NUM) {
                        *result_type = TYPE_NUM; // Только Число / Число
                    } else {
                        fprintf(stderr, "Semantic Error (Line %d): Invalid operands for '/'. Must be Num/Num.\n", node->line_number);
                        // ВОЗВРАЩАЕМ КОД ОШИБКИ
                        return false;
                    }
                    break;
                
                default: break; // Невозможно
            }

            // 4. Устанавливаем тип узла
            node->data_type = *result_type;
            return true;
        }


        // ... (case'ы для Литералов, NODE_ID, Арифметики) ...
        // --- СЛУЧАЙ 3: Реляционные Операторы (<, >, <=, >=) ---
        case NODE_OP_LT:
        case NODE_OP_GT:
        case NODE_OP_LTE:
        case NODE_OP_GTE:
        {
            printf("DEBUG: Analyzing Relational Op (%d).\n", node->type);

            // 1. Анализируем левую часть
            DataType left_type;
            if (!analyze_expression(node->child, func_local_table, stack, mangling_counter, &left_type)) {
                return false; 
            }

            // 2. Анализируем правую часть
            DataType right_type;
            if (!analyze_expression(node->child->sibling, func_local_table, stack, mangling_counter, &right_type)) {
                return false; 
            }

            // 3. Проверка Типов
            // Оба обязаны быть Num
            if (left_type != TYPE_NUM || right_type != TYPE_NUM) {
                 fprintf(stderr, "Semantic Error (Line %d): Invalid operands for relational operator. Must be Num <> Num.\n", node->line_number);
                // ВОЗВРАЩАЕМ КОД ОШИБКИ
                return false;
            }
        
            // 4. Устанавливаем тип результата
            node->data_type = TYPE_BOOL;
            *result_type = TYPE_BOOL;
            return true;
        }

        // --- СЛУЧАЙ 4: Операторы Равенства (==, !=) ---
        case NODE_OP_EQ:
        case NODE_OP_NEQ:
        {
            printf("DEBUG: Analyzing Equality Op (%d).\n", node->type);

            // 1. Анализируем левую часть
            DataType left_type;
            if (!analyze_expression(node->child, func_local_table, stack, mangling_counter, &left_type)) {
                return false; 
            }

            // 2. Анализируем правую часть
            DataType right_type;
            if (!analyze_expression(node->child->sibling, func_local_table, stack, mangling_counter, &right_type)) {
                return false; 
            }

            // 3. Проверки типов
            // ЛЮБЫЕ 2 ТИПА МОЖНО СРАВНИВАТЬ

            // 4. Устанавливаем тип результата
            node->data_type = TYPE_BOOL;
            *result_type = TYPE_BOOL;
            return true;
        }

        // --- СЛУЧАЙ 5: Проверка Типа 'is' ---

        case NODE_OP_IS:
        {
            printf("DEBUG: Analyzing Equality Op (%d).\n", node->type);

            // 1. Анализируем левую часть
            DataType left_type;
            if (!analyze_expression(node->child, func_local_table, stack, mangling_counter, &left_type)) {
                return false; 
            }

            // 2. Анализируем правую часть
            AstNode* type_name_node = node->child->sibling;
            if (type_name_node->type != NODE_TYPE_NAME) {
                 fprintf(stderr, "Semantic Error (Line %d): Right side of 'is' must be a type name (Num, String, Null).\n", node->line_number);
                // ВОЗВРАЩАЕМ КОД ОШИБКИ
                return false;
            }

            // 3. Проверяем, что это корректное имя типа
            const char* type_name = type_name_node->data.identifier;
            if (strcmp(type_name, "Num") != 0 &&
                strcmp(type_name, "String") != 0 &&
                strcmp(type_name, "Null") != 0)
            {
                 fprintf(stderr, "Semantic Error (Line %d): Unknown type name '%s' in 'is' expression.\n", node->line_number, type_name);
                // ВОЗВРАЩАЕМ КОД ОШИБКИ 6
                return false;
            }

            // 4. Устанавливаем тип результата
            node->data_type = TYPE_BOOL;
            *result_type = TYPE_BOOL;
            return true;    
        }

        // --- СЛУЧАЙ 6: Вызов Функции 'id(...)' ---

        case NODE_CALL_STATEMENT:
        {
            printf("DEBUG: Analyzing NODE_CALL_STATEMENT (%s).\n", node->data.identifier);

            const char* name = node->data.identifier;
            AstNode* arg_list = node->child->sibling;

            // 1. Подсчитываем Аргументы
            int arity = 0;
            if (arg_list != NULL && arg_list->type == NODE_ARGUMENT_LIST) {
                for (AstNode* arg = arg_list->child; arg != NULL; arg = arg->sibling) {
                    arity++;
                }
            }

            // 2. Создаем "Mangled Name"
            char mangled_name[256];
            sprintf(mangled_name, "%s@%d", name, arity);

            // 3. Ищем Функцию 
            TableEntry* func_entry = symtable_lookup(&global_table, mangled_name);

            if (func_entry == NULL) {
                // Это может быть Ошибка 3 (функция 'foo' не найдена)
                // или Ошибка 5 (функция 'foo' найдена, но с другим числом
                // аргументов, напр. 'foo@1' вместо 'foo@2')
                fprintf(stderr, "Semantic Error (Line %d): Call to undefined function '%s' with %d arguments.\n", node->line_number, name, arity);
                // ВОЗВРАЩАЕМ КОД ОШИБКИ 3 или 5
                return false;
            }

            // 4. Связываем AST
            node->table_entry = func_entry;

            // 5. Рекурсивно Анализируем Аргументы

            // Временный массив для хранения типов
            // (10 - произвольное безопасное число, макс. нужно 3)

            DataType arg_types[10];
            int arg_index = 0;

            if (arg_list != NULL) {
                for (AstNode* arg = arg_list->child; arg != NULL; arg = arg->sibling) {
                    
                    if (!analyze_expression(arg, func_local_table, stack, mangling_counter, &arg_types[arg_index])) {
                        // Ошибка внутри аргумента
                        return false; 
                    }
                    arg_index++;
                }
            }

            // проверяем нравится ли аргументы встроенным функциям
            if (strncmp(name, "Ifj.", 4) == 0) {
                bool types_ok = true;
                
                if (strcmp(name, "Ifj.floor") == 0) {
                    if (arg_types[0] != TYPE_NUM) types_ok = false;
                }
                else if (strcmp(name, "Ifj.length") == 0) {
                    if (arg_types[0] != TYPE_STR) types_ok = false;
                }
                else if (strcmp(name, "Ifj.substring") == 0) {
                    if (arg_types[0] != TYPE_STR || arg_types[1] != TYPE_NUM || arg_types[2] != TYPE_NUM) types_ok = false;
                }
                else if (strcmp(name, "Ifj.strcmp") == 0) {
                    if (arg_types[0] != TYPE_STR || arg_types[1] != TYPE_STR) types_ok = false;
                }
                else if (strcmp(name, "Ifj.ord") == 0) {
                    if (arg_types[0] != TYPE_STR || arg_types[1] != TYPE_NUM) types_ok = false;
                }
                else if (strcmp(name, "Ifj.chr") == 0) {
                    if (arg_types[0] != TYPE_NUM) types_ok = false;
                }
                // 'Ifj.write' и 'Ifj.str' принимают любые 
                // 'Ifj.read_...' не имеют арг-ов, прроверять не нужно

                if (!types_ok) {
                    fprintf(stderr, "Semantic Error (Line %d): Invalid parameter type(s) for built-in function '%s'.\n",
                            node->line_number, name);
                    // ВОЗВРАЩАЕМ КОД ОШИБКИ
                    return false;
                }
            }

            // 6. Устанавливаем и возвращаем тип
            // Тип выражения - это тип возвращаемого значения функции
            node->data_type = func_entry->data->data_type;
            *result_type = func_entry->data->data_type;
            
            return true;
        }


        default:
            // Мы получили узел, который не является выражением
            // (напр., NODE_BLOCK или NODE_VAR_DEF)
            fprintf(stderr, "Internal Error (Line %d): Unexpected node type (%d) in expression.\n", node->line_number, node->type);
            return false;
    }
}
