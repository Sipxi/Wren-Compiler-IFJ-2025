/**
 * @file symtable.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Implementace tabulky symbolů s použitím otevřené adresace a lineárního průzkumu.
 *
 * @author
 *     - Serhij Čepil (253038)
 */
#include "symtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 512
// "Magické" číslo pro inicializaci hashe DJB2
#define HASH_NUMBER 5381
#define LOAD_FACTOR_THRESHOLD 0.5
#define RESIZE_FACTOR 2

/* ======================================*/
/* ===== Deklarace privátních funkcí =====*/
/* ======================================*/

/**
 * Mění velikost tabulky symbolů s odpovídajícím LOAD_FACTOR_THRESHOLD.
 * Používá symtable_rehash pro přerozdělení záznamů
 *
 * @param table Ukazatel na tabulku symbolů pro změnu velikosti.
 */
static bool symtable_resize(Symtable* table);

/**
 * Kontroluje, zda je překročen LOAD_FACTOR_THRESHOLD pro tabulku symbolů.
 *
 * @param table Ukazatel na tabulku symbolů pro kontrolu.
 * @return true pokud současný koeficient zatížení překračuje
 * LOAD_FACTOR_THRESHOLD, jinak false.
 */
static bool check_load_factor(Symtable* table) {
    return ((double)table->count / (double)table->capacity) >=
           LOAD_FACTOR_THRESHOLD;
}

/**
 * Kopíruje řetězec, alokuje pro něj paměť.
 *
 * Uděláno kvůli tomu, že C99 nezaručuje přítomnost strdup v string.h
 *? Je to potřeba?
 *
 * @param s Zdrojový řetězec pro kopírování.
 * @return Ukazatel na nový řetězec-kopii, nebo NULL v případě chyby
 */
static char* my_strdup(const char* s);

/* ======================================*/
/* ===== Implementace privátních funkcí =====*/
/* ======================================*/

static char* my_strdup(const char* s) {
    size_t len = strlen(s);
    char* copy = malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len + 1);  // +1 pro nulový terminátor
    return copy;
}

static void symtable_insert_rehash(Symtable* table, TableEntry* entry) {
    size_t hash_index = get_hash(entry->key, table->capacity);
    while (table->entries[hash_index].status == SLOT_OCCUPIED) {
        hash_index = (hash_index + 1) % table->capacity;
    }
    // Zkopírovat záznam do nového slotu
    table->entries[hash_index] = *entry;
    table->entries[hash_index].status = SLOT_OCCUPIED;
    table->count++;
}

static bool symtable_resize(Symtable* table) {
    size_t new_capacity = table->capacity * RESIZE_FACTOR;
    TableEntry* new_entries = calloc(new_capacity, sizeof(TableEntry));
    if (new_entries == NULL) {
        return false;  // Chyba alokace paměti, necháváme tabulku beze
                       // změn
    }
    // Ukládáme staré záznamy
    TableEntry* old_entries = table->entries;
    size_t old_capacity = table->capacity;
    table->entries = new_entries;
    table->capacity = new_capacity;
    table->count = 0;

    // Přehashujeme staré záznamy
    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].status == SLOT_OCCUPIED) {
            symtable_insert_rehash(table, &old_entries[i]);
        }
    }

    free(old_entries);
    return true;
}

/* ======================================*/
/* ===== Implementace veřejných funkcí =====*/
/* ======================================*/

bool symtable_init(Symtable* table) {
    table->entries = calloc(INITIAL_CAPACITY, sizeof(TableEntry));
    if (table->entries == NULL) {
        return false;  // Chyba při alokaci paměti
    }
    table->count = 0;
    table->capacity = INITIAL_CAPACITY;
    return true;
}

size_t get_hash(const char* key, size_t capacity) {
    // Funkce vypočítává hash pro řetězec key
    // Používá algoritmus DJB2 a vrací index v rozsahu capacity

    unsigned long hash = HASH_NUMBER;  // Inicializace hashe "magickým" číslem
                                       // 5381 (používá se v DJB2)
    int c;  // Proměnná pro současný symbol z řetězce

    while (
        (c = *key++)) {  // Cyklus po každém symbolu řetězce do symbolu konce '\0'
        // *key++ bere současný symbol a posunuje ukazatel na následující
        hash = ((hash << 5) + hash) + c;  // hash = hash * 33 + c
        // (hash << 5) + hash = hash * 32 + hash = hash * 33
        // Taková operace dává dobré rozložení hashe pro krátké řetězce
    }

    return (size_t)(hash %
                    capacity);  // Omezujeme výsledek velikostí tabulky
    // % capacity zaručuje, že index spadne do rozsahu [0, capacity-1]
}

TableEntry* symtable_lookup(Symtable* table, const char* key) {
	size_t hash_index =
        get_hash(key, table->capacity);  // Vypočítáváme hash-index pro klíč
    size_t original_index = hash_index;  // Ukládáme originální index pro
                                         // detekci úplného průchodu
    TableEntry* entry = &table->entries[hash_index];  // Získáváme ukazatel na
                                                      // záznam podle hash-indexu

    while (entry->status != SLOT_EMPTY) {
        if (entry->status == SLOT_OCCUPIED && strcmp(entry->key, key) == 0) {
            return entry;  // Pokud je záznam obsazený a klíče se shodují, vracíme
                           // záznam
        }
        // Lineární průzkum: přecházíme k následujícímu slotu
        // modul je potřeba pro průchod v případě dosažení konce pole
        hash_index = (hash_index + 1) % table->capacity;
        entry = &table->entries[hash_index];
        if (hash_index == original_index) {
            // Prošli jsme celou tabulku a nenašli záznam
            break;
        }
    }
    return NULL;  // Pokud záznam nebyl nalezen, vracíme NULL
}

bool symtable_insert(Symtable* table, const char* key, SymbolData* data) {
    // Kontrolujeme nutnost změny velikosti tabulky
    if (check_load_factor(table)) {
        if (!symtable_resize(table)) {
            return false;  // Chyba změny velikosti, vložení nebylo provedeno
        }
    }

    size_t hash_index = get_hash(key, table->capacity);
    size_t original_index = hash_index;
    TableEntry* entry = &table->entries[hash_index];

    while (entry->status == SLOT_OCCUPIED) {
        if (strcmp(entry->key, key) == 0) {
            // Klíč už existuje, aktualizujeme data
            free(entry->data);
            entry->data = malloc(sizeof(SymbolData));
            
            if (entry->data == NULL) {
                free(entry->key);  // Uvolňujeme klíč při chybě
                return false;      // Chyba alokace paměti pro data
            }
            memcpy(entry->data, data, sizeof(SymbolData));  // Kopírujeme data
            return true;
        }
        hash_index = (hash_index + 1) % table->capacity;
        entry = &table->entries[hash_index];
        if (hash_index == original_index) {
            // Tabulka je plná (i když se to nemělo stát kvůli kontrole výše)
            return false;
        }
    }

    // Vkládáme nový záznam
    entry->key = my_strdup(key);  // Kopírujeme klíč
    if (entry->key == NULL) {
        return false;  // Chyba alokace paměti pro klíč
    }
    entry->data = malloc(sizeof(SymbolData));
    if (entry->data == NULL) {
        free(entry->key);  // Uvolňujeme klíč při chybě
        return false;      // Chyba alokace paměti pro data
    }
    memcpy(entry->data, data, sizeof(SymbolData));  // Kopírujeme data
    entry->status = SLOT_OCCUPIED;  // Aktualizujeme stav slotu
    table->count++;                 // Zvyšujeme počet záznamů

    return true;
}

void symtable_delete(Symtable* table, const char* key) {
    TableEntry* entry = symtable_lookup(table, key);
    if (entry != NULL) {
        entry->status = SLOT_DELETED;  // Označujeme slot jako smazaný
        table->count--;                // Snižujeme počet záznamů
        // Uvolnění paměti pro klíč a data
        free(entry->key);
        free(entry->data);
        entry->key = NULL;
        entry->data = NULL;
    }
}

void symtable_free(Symtable* table) {
    for (size_t i = 0; i < table->capacity; i++) {
        TableEntry* entry = &table->entries[i];
        if (entry->status == SLOT_OCCUPIED) {
            // Rekurzivně uvolňujeme vnořené tabulky symbolů
            if (entry->local_table != NULL) {
                symtable_free(entry->local_table);
                free(entry->local_table);
                entry->local_table = NULL;
            }
            free(entry->key);   // Uvolňujeme paměť pro klíč
            free(entry->data);  // Uvolňujeme paměť pro data symbolu
        }
    }
    free(table->entries);  // Uvolňujeme pole záznamů
    table->entries = NULL;
    table->count = 0;
    table->capacity = 0;
}

/* ===================================================*/
/* ===== Pomocné funkce pro tisk (NOVÉ) =====*/
/* ===================================================*/

/**
 * Převádí enum SlotStatus na čitelný řetězec.
 */
static const char* status_to_string(SlotStatus status) {
    switch (status) {
        case SLOT_EMPTY:    return "EMPTY";
        case SLOT_OCCUPIED: return "OCCUPIED";
        case SLOT_DELETED:  return "DELETED";
    }
    return "UNKNOWN_STATUS";
}

/**
 * Převádí enum SymbolKind na čitelný řetězec.
 */
static const char* kind_to_string(SymbolKind kind) {
    switch (kind) {
        case KIND_VAR:    return "Variable";
        case KIND_FUNC:   return "Function";
        case KIND_BLOCK:  return "Block";
    }
    return "UNKNOWN_KIND";
}

/**
 * Převádí enum DataType na čitelný řetězec.
 */
static const char* type_to_string(DataType type) {
    switch (type) {
        case TYPE_NUM:    return "Number";
        case TYPE_STR:    return "String";
        case TYPE_NIL:    return "Nil";
        case TYPE_BOOL:   return "Boolean";
        case TYPE_FLOAT:  return "Float";
        default: return "UNKNOWN_TYPE";
    }
}

/**
 * Tiskne zadané množství mezer pro odsazení (pro vnořenost).
 */
static void print_indent(int level) {
    for (int i = 0; i < level; i++) {
        printf("    "); // 4 mezery na jednu úroveň odsazení
    }
}

// Předběžná deklarace vnitřní rekurzivní funkce,
// aby symtable_print_entry na ni mohla odkazovat při tisku vnořených tabulek.
static void symtable_print_internal(Symtable *table, int level);

/**
 * Tiskne JEDEN záznam (jeden slot) tabulky symbolů.
 * Tato funkce je volána v cyklu z 'symtable_print_internal'.
 *
 * @param entry Ukazatel na záznam (slot) pro tisk.
 * @param index Index tohoto slotu v poli.
 * @param level Současná úroveň odsazení.
 */
static void symtable_print_entry(TableEntry *entry, size_t index, int level) {
    // 1. Tiskeme základní informace o slotu
    print_indent(level);
    printf("Slot %3zu: [%-8s] ", index, status_to_string(entry->status));

    // 2. Pokud je slot obsazený, tiskeme podrobná data
    if (entry->status == SLOT_OCCUPIED) {
        // Kontrola na poškozená data (pokud by key nebo data byly náhodou NULL)
        if (entry->key == NULL || entry->data == NULL) {
            printf("-> !!! POŠKOZENÝ ZÁZNAM !!!\n");
            return; // Víc tisknout není co
        }

        // Tiskeme klíč
        printf("-> Klíč: \"%s\"\n", entry->key);

        // 3. Tiskeme detaily z SymbolData (s dodatečným odsazením)
        SymbolData *data = entry->data;
        print_indent(level + 1); // +1 úroveň odsazení
        printf("Druh:     %s\n", kind_to_string(data->kind));
        print_indent(level + 1);
        printf("Typ:      %s\n", type_to_string(data->data_type));
        print_indent(level + 1);
        printf("Opred:    %s\n", data->is_defined ? "true" : "false");
        print_indent(level + 1);
        printf("Unique:   %s\n", data->unique_name ? data->unique_name : "-");
       

        // 4. Rekurzivní volání (OPRAVENO)
        // Díváme se na 'entry', ne na 'data'!
        if (entry->local_table != NULL) { 
            print_indent(level + 1);
            printf("Lok. tabulka:\n");
            // Voláme rekurzi pro vnořenou tabulku
            symtable_print_internal(entry->local_table, level + 1);
        } else {
            print_indent(level + 1);
            printf("Lok. tabulka: (NULL)\n");
        }
    } else {
        // Pro EMPTY a DELETED jen přejdeme na nový řádek
        printf("\n");
    }
}


/**
 * Vnitřní rekurzivní funkce pro tisk celé tabulky.
 * (Tato funkce je potřebná pro zpracování úrovní vnořenosti).
 *
 * @param table Ukazatel na tabulku pro tisk.
 * @param level Současná úroveň odsazení.
 */
static void symtable_print_internal(Symtable *table, int level) {
    // Ochrana před NULL (například pokud local_table u funkce není inicializovaná)
    if (table == NULL) {
        print_indent(level);
        printf("(NULL Lokální tabulka)\n");
        return;
    }

    // --- 1. Záhlaví tabulky ---
    print_indent(level);
    printf("--- Tabulka Symbolů (Úroveň %d) ---\n", level);
    print_indent(level);
    printf("  Kapacita:   %zu\n", table->capacity);
    print_indent(level);
    printf("  Množství:   %zu\n", table->count);
    print_indent(level);
    
    // Počítáme a tiskeme faktor zatížení
    double load_factor = (table->capacity == 0) ? 0.0 : (double)table->count / (double)table->capacity;
    printf("  Zatížení:   %.2f%% (Práh: %.0f%%)\n", 
           load_factor * 100.0, 
           LOAD_FACTOR_THRESHOLD * 100.0); // V procentech
    print_indent(level);
    printf("------------------------------------\n");

    // --- 2. Tisk všech záznamů slotů ---
    for (size_t i = 0; i < table->capacity; i++) {
        // Voláme funkci tisku pro každý jednotlivý záznam
        symtable_print_entry(&table->entries[i], i, level);
    }
}



/**
 * Veřejná funkce obálka pro tisk tabulky.
 * Vytváří pěkné záhlaví a volá vnitřní 
 * rekurzivní funkci, začínajíc úrovní 0.
 */
void symtable_print(Symtable *table) {
    // Obecné záhlaví
    printf("\n");
    printf("======================================================\n");
    printf("                LADÍCÍ VÝSTUP TABULKY\n");
    printf("======================================================\n");
    
    // Kontrola, zda nám nebyl předán NULL
    if (table == NULL) {
        printf("(NULL Tabulka)\n");
    } else {
        // Spouštíme rekurzivní tisk.
        // Úroveň 0 - to je nejhornější (globální) úroveň.
        symtable_print_internal(table, 0);
    }
    
    // Obecné zápatí
    printf("======================================================\n\n");
}