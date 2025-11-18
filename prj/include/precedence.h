
#ifndef PRECEDENCE_H
#define PRECEDENCE_H

// Подключаем твой токенайзер, чтобы иметь определение Token
// (Если он называется по-другому, измени имя)
#include "lexer.h" 

/**
 * @brief Индексы для прецедентной таблицы (Терминалы)
 * Используются как для стека (S), так и для входа (I).
 */
typedef enum {
    // Операторы по порядку из PDF
    T_PLUS,       // 0: +
    T_MINUS,      // 1: -
    T_MUL,        // 2: *
    T_DIV,        // 3: /
    T_LT,         // 4: <
    T_GT,         // 5: >
    T_LTE,        // 6: <=
    T_GTE,        // 7: >=
    T_IS,         // 8: is
    T_EQ,         // 9: ==
    T_NEQ,        // 10: !=
    T_OPEN_PAREN, // 11: (
    T_CLOSE_PAREN,// 12: )
    
    // Операнды и типы
    T_TERM,       // 13: i (id, int, string, null, ...)
    T_TYPE,       // 14: k (Num, String, Null)
    
    // Специальные символы
    T_DOLLAR,     // 15: $ (конец выражения)
    
    T_ERROR       // 16: Ошибочный токен
} TermIndex; // Индекс терминала

/**
 * @brief Символы для грамматических правил (Нетерминалы + Терминалы)
 * GS = Grammar Symbol
 */
typedef enum {
    GS_UNDEF = 0, // Неопределенный/пустой символ

    // --- Терминалы (должны соответствовать TermIndex) ---
    GS_PLUS,        // 1: +
    GS_MINUS,       // 2: -
    GS_MUL,         // 3: *
    GS_DIV,         // 4: /
    GS_LT,          // 5: <
    GS_GT,          // 6: >
    GS_LTE,         // 7: <=
    GS_GTE,         // 8: >=
    GS_IS,          // 9: is
    GS_EQ,          // 10: ==
    GS_NEQ,         // 11: !=
    GS_OPEN_PAREN,  // 12: (
    GS_CLOSE_PAREN, // 13: )
    GS_TERM,        // 14: i (операнд)
    GS_TYPE,        // 15: k (тип)
    GS_DOLLAR,      // 16: $
    
    // --- Нетерминалы ---
    GS_E,           // 17: E (Expression)
    GS_HANDLE_START // 18: < (Специальный маркер начала рукоятки)

} GrammarSymbol;

// --- Константы ---
#define PRECEDENCE_TABLE_SIZE 16 // 16x16 (от T_PLUS до T_DOLLAR)
#define NUM_GRAMMAR_RULES 13     // У нас 13 правил (0-12)
#define MAX_RULE_LENGTH 4        // Макс. длина правила: 3 (E -> E op E) + 1 (результат)

/**
 * @brief Глобальная прецедентная таблица
 */
extern const char precedence_table[PRECEDENCE_TABLE_SIZE][PRECEDENCE_TABLE_SIZE];

/**
 * @brief Глобальные правила грамматики
 */
extern const int grammar_rules[NUM_GRAMMAR_RULES][MAX_RULE_LENGTH];

/**
 * @brief Конвертирует токен из сканера в индекс для таблицы.
 * @param token Токен от лексического анализатора.
 * @return TermIndex Индекс для precedence_table.
 */
TermIndex token_to_index(Token token);

/**
 * @brief Получает правило ('>', '<', '=', 'E') из таблицы.
 * @param stack_top Индекс символа на вершине стека.
 * @param input Индекс символа на входе.
 * @return char Правило.
 */
char get_precedence_rule(TermIndex stack_top, TermIndex input);

/**
 * @brief Конвертирует токен из сканера в символ грамматики.
 * Используется при *сдвиге* (shift).
 * @param token Токен от лексического анализатора.
 * @return GrammarSymbol Символ для стека парсера.
 */
GrammarSymbol token_to_grammar_symbol(Token token);


#endif // PRECEDENCE_H
