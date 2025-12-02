/**
 * @file symtable.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Hlavičkový soubor pro tabulku symbolů.
 *
 * Tento soubor obsahuje definice struktur a funkcí pro práci s tabulkou symbolů,
 * která se používá pro ukládání informací o proměnných, funkcích a dalších symbolech
 * v průběhu kompilace.
 *
 * @author
 *     - Serhij Čepil (253038)
 *     
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stddef.h>
#include <stdbool.h>

/* ======================================*/
/* ===== Enums =====*/
/* ======================================*/

// Typy dat
typedef enum{
TYPE_NUM,
TYPE_STR,
TYPE_NIL,
TYPE_BOOL, // Přidáno pro podporu porovnání
TYPE_UNKNOWN, // Pro neinicializované proměnné a chyby
TYPE_FLOAT, // Přidáno pro podporu float


} DataType;

// Druh symbolu
typedef enum{
KIND_VAR,
KIND_FUNC,
KIND_BLOCK

} SymbolKind;

// Stav slotu v tabulce symbolů
typedef enum{
// Počáteční hodnota pro prázdný slot
// Začínáme s 0 pro calloc
SLOT_EMPTY = 0,
SLOT_OCCUPIED,
SLOT_DELETED,
} SlotStatus;

/* ======================================*/
/* ===== Struktury =====*/
/* ======================================*/


// Předběžná deklarace struktury Symtable
// pro použití v ostatních částech kódu
struct Symtable;

// Data symbolu
typedef struct{
SymbolKind kind;    // Druh symbolu (proměnná, funkce atd.)
DataType data_type; // Datový typ symbolu
bool is_defined; // Příznak označující, zda je symbol definován (pro funkce)
char* unique_name; // Unikátní jméno pro pojmenování

} SymbolData;


// Záznam v tabulce symbolů
typedef struct{
char *key;              // Klíč symbolu (jméno)
SymbolData *data;      // Ukazatel na data symbolu
SlotStatus status;     // Stav slotu (obsazený, prázdný, smazaný)
struct Symtable *local_table; // Ukazatel na lokální tabulku symbolů (pro funkce)
} TableEntry;

// Tabulka symbolů
typedef struct Symtable{
TableEntry* entries;   // Pole záznamů tabulky symbolů
size_t count;           // Současný počet záznamů v tabulce
size_t capacity;       // Kapacita tabulky, tj. maximální počet záznamů

int nesting_level;     // Úroveň vnořenosti (0 pro GF, 1 pro LF, 2+ pro vnořené bloky)
// struct Symtable* parent_scope; // Ukazatel na rodičovskou tabulku (pro vyhledávání)
} Symtable;

// ======================================*/
// ===== Veřejné funkce pro práci s tabulkou symbolů =====*/
// ======================================*/

/**
 * Inicializuje tabulku symbolů.
 * 
 * @param table Ukazatel na tabulku symbolů pro inicializaci.
 * 
 * @return true pokud je inicializace úspěšná, false v případě chyby alokace paměti.
 */
bool symtable_init(Symtable *table);

/**
 * Vypočítává hash hodnotu pro daný klíč.
 * 
 * Tato funkce implementuje hashovací algoritmus DJB2.
 * 
 * @see https://theartincode.stanis.me/008-djb2/
 * @param key Klíč, pro který je potřeba vypočítat hash.
 * @param capacity Kapacita tabulky symbolů (používá se pro omezení rozsahu hash hodnoty).
 * @return Vypočítaná hash hodnota.
 */
size_t get_hash(const char *key, size_t capacity);

/**
 * Hledá záznam v tabulce symbolů podle daného klíče.
 * 
 * @param table Ukazatel na tabulku symbolů pro vyhledávání.
 * @param key Klíč symbolu pro vyhledávání.
 * @return Ukazatel na nalezený záznam TableEntry nebo NULL, pokud záznam nebyl nalezen.
 */
TableEntry* symtable_lookup(Symtable *table, const char *key);

/**
 * Vkládá nový záznam do tabulky symbolů.
 * 
 * Pokud bude tabulka zaplněna do určitého prahu,
 * bude automaticky rozšířena pro zajištění efektivity.
 * 
 * @param table Ukazatel na tabulku symbolů pro vložení.
 * @param key Klíč symbolu pro vložení.
 * @param data Ukazatel na data symbolu pro vložení. 
 * Tabulka symbolů přebírá odpovědnost za uvolnění této paměti
 * v případě přepsání.
 * 
 * @return true pokud je vložení úspěšné, false v případě chyby (například chyba alokace paměti).
 */
bool symtable_insert(Symtable *table, const char *key, SymbolData *data);


/**
 * Odstraňuje záznam z tabulky symbolů podle daného klíče.
 * 
 * @param table Ukazatel na tabulku symbolů pro odstranění.
 * @param key Klíč symbolu pro odstranění.
 */
void symtable_delete(Symtable *table, const char *key);


/**
 * Uvolňuje zdroje zabírané tabulkou symbolů.
 * 
 * @param table Ukazatel na tabulku symbolů pro uvolnění.
 */
void symtable_free(Symtable *table);


/* ======================================*/
/* ===== Pro vývoj  =====*/
/* ======================================*/

void symtable_print(Symtable *table);

#endif // SYMTABLE_H