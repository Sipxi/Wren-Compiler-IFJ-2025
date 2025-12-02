/**
 * @file codegen.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Declarace veřejných funkcí generátoru cílového kódu IFJcode25
 *
 * @author
 *     - Dmytro Kravchenko (273125)
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "symtable.h"
#include "tac.h"
#include "utils.h"

/**
 * @brief Hlavní funkce pro generování cílového kódu z TAC instrukcí.
 * 
 * @param instructions Seznam TAC instrukcí pro generování kódu.
 * @param table Symbolová tabulka pro generování kódu.
 * @return int Návratový kód (0 při úspěchu).
 */
int generate_code(TACDLList *instructions, Symtable *table);

/**
 * @brief Převádí řetězec na formátovaný řetězec pro IFJcode25.
 * 
 * @param original Původní řetězec.
 * @return char* Nový řetězec ve formátu IFJcode25 (nutné uvolnit paměť).
 */
char* string_to_ifjcode(const char *original);

#endif // CODEGEN_H