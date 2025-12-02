/**
 * @file optimizer.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Hlavní hlavičkový soubor pro optimalizaci třiadresného kódu (TAC).
 *
 * @author
 *     - Serhij Čepil (253038)
 */


#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "tac.h"


/* ======================================*/
/* ===== Definice veřejných funkcí =====*/
/* ======================================*/

/**
 * @brief Optimalizuje seznam tříadresného kódu (TAC).
 * 
 * Tato funkce aplikuje různé optimalizace na seznam instrukcí TAC,
 * aby zlepšila výkon a snížila velikost kódu.
 * 
 * @param tac_list Seznam instrukcí TAC k optimalizaci.
 */
void optimize_tac(TACDLList *tac_list);



#endif // OPTIMIZER_H