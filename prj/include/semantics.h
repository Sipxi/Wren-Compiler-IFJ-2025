/**
 * semantics.h
 * Заголовочный файл семантического анализатора.
 */

#ifndef SEMANTICS_H
#define SEMANTICS_H

#include <stdbool.h>
#include <string.h>
#include "ast.h"
#include "symtable.h"


// Глобальная таблица символов.
// Мы делаем её extern, чтобы main.c (или тесты) могли её видеть/печатать,
// а генератор кода (Pass 3) мог её использовать.
extern Symtable global_table;

/**
 * Главная функция семантического анализатора.
 * Запускает все проходы:
 * 1. Шаг 0: Регистрация встроенных функций.
 * 2. Шаг 2a: Сбор пользовательских функций.
 * 3. Шаг 2b: Анализ тел функций.
 * * @param root Корень AST (NODE_PROGRAM).
 * @return true, если анализ прошел успешно, иначе false.
 * (TODO: Поменять возвращаемый тип на int для кодов ошибок 3, 4, 5, 6)
 */
bool analyze_semantics(AstNode* root);

#endif // SEMANTICS_H