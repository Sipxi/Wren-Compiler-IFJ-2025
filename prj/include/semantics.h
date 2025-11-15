#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "symtable.h"
#include "ast.h"
#include "scopemap.h"
#include "scopestack.h"
#include <stdbool.h>
#include <string.h>

// Внешняя глобальная таблица символов
extern Symtable global_table;

/**
 * Главная функция семантического анализатора.
 * 
 * Выполняет полный семантический анализ в несколько проходов:
 * - Шаг 0: Инициализация глобальной таблицы встроенными функциями
 * - Шаг 2a: Сбор объявлений пользовательских функций
 * - Шаг 2b: Анализ тел функций и проверка типов
 * 
 * @param root Корневой узел AST (должен быть NODE_PROGRAM)
 * @return true при успехе, false при семантической ошибке
 */
bool analyze_semantics(AstNode* root);

/**
 * Регистрирует встроенную функцию в глобальной таблице символов.
 * 
 * @param name Имя функции (напр., "Ifj.write")
 * @param arity Количество параметров
 * @param return_type Тип возвращаемого значения
 * @return true при успехе, false при внутренней ошибке
 */
static bool register_builtin_function(const char* name, int arity, DataType return_type);

/**
 * Подсчитывает количество параметров у узла функции.
 * 
 * @param func_node Узел определения функции (NODE_FUNCTION_DEF, NODE_SETTER_DEF, NODE_GETTER_DEF)
 * @return Количество параметров
 */
static int count_parameters(AstNode* func_node);

/**
 * Обрабатывает объявление функции и добавляет в глобальную таблицу символов.
 * 
 * Создает искаженные имена, проверяет на переопределение и инициализирует
 * локальную таблицу символов для функции.
 * 
 * @param func_node Узел определения функции
 * @return true при успехе, false при семантической ошибке (переопределение)
 */
static bool process_function_declaration(AstNode* func_node);

/**
 * Анализирует тело одной функции.
 * 
 * Настраивает стек областей видимости, обрабатывает параметры и рекурсивно
 * анализирует операторы тела функции.
 * 
 * @param func_node Узел определения функции
 * @return true при успехе, false при семантической ошибке
 */
static bool analyze_function_body(AstNode* func_node);

/**
 * Рекурсивно анализирует узел-оператор.
 * 
 * Обрабатывает блоки, определения переменных, присваивания, управляющие
 * конструкции и вызовы функций в контексте операторов.
 * 
 * @param node Узел оператора для анализа
 * @param func_local_table Локальная таблица символов текущей функции
 * @param stack Стек областей видимости для разрешения переменных
 * @param mangling_counter Счетчик для генерации уникальных имен переменных
 * @return true при успехе, false при семантической ошибке
 */
static bool analyze_statement(AstNode* node, Symtable* func_local_table, 
                             ScopeStack* stack, int* mangling_counter);

/**
 * Рекурсивно анализирует узел-выражение.
 * 
 * Выполняет проверку типов, разрешает идентификаторы, валидирует операции
 * и определяет результирующие типы для выражений.
 * 
 * @param node Узел выражения для анализа
 * @param func_local_table Локальная таблица символов текущей функции
 * @param stack Стек областей видимости для разрешения переменных
 * @param mangling_counter Счетчик для генерации уникальных имен переменных
 * @param result_type Выходной параметр для типа результата выражения
 * @return true при успехе, false при семантической ошибке
 */
static bool analyze_expression(AstNode* node, Symtable* func_local_table,
                              ScopeStack* stack, int* mangling_counter, 
                              DataType* result_type);

#endif