/**
 * @file utils.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Hlavní hlavičkový soubor pro pomocné funkce používané v různých částech projektu.
 *
 * @author
 *     - Serhij Čepil (253038)
 */

#ifndef UTILS_H
#define UTILS_H


/**
 * @brief Kopíruje řetězec a alokuje paměť pro jeho novou kopii.
 * 
 * Tato funkce se chová obdobně jako strdup z C99.
 * Alokovanou paměť je třeba uvolnit voláním free().
 * 
 * @param s Původní řetězec ke kopírování.
 * @return Ukazatel na novou kopii řetězce nebo NULL při chybě alokace paměti.
 */
char *strdup_c99(const char *s);

#endif // UTILS_H