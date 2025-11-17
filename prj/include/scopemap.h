/**
 * scopemap.h
 *
 * Временная "карта" (реализованная как лист), которая хранит
 * соответствие между простым именем (напр. "a") и указателем на
 * его TableEntry (а.к.а "уебищное имя с итерируемой константой" напр."a_1") в постоянной func_local_table.
 *
 * Используется только в Pass 2 (Семантика).
 */

#ifndef SCOPEMAP_H
#define SCOPEMAP_H

#include <stdbool.h>
#include "symtable.h" // доступ к 'TableEntry'


//* --- 1. Структура Узла Списка ---
/**
 * Один узел в ScopeMap.
 */
typedef struct ScopeMapNode
{
    char* key;                 // Простое имя (напр., "a"), которое мы копируем
    TableEntry* entry;         // УКАЗАТЕЛЬ на запись в func_local_table
    struct ScopeMapNode* next; // Следующий узел в списке
} ScopeMapNode;


//* --- 2. Структура Самой Карты ---
/**
 * Сама "карта", которая просто указывает на начало списка.
 */
typedef struct {
    ScopeMapNode* head; // Голова списка
} ScopeMap;


//* --- 3. Функции ---
/**
 * Создает новую, пустую ScopeMap.
 * @return Указатель на новую ScopeMap (или NULL при ошибке).
 */
ScopeMap* scopemap_create();


/**
 * Освобождает ScopeMap и все ее узлы.
 *
 * ВАЖНО: Эта функция освобождает 'map', 'ScopeMapNode' и 'key'.
 * Она НЕ ТРОГАЕТ 'TableEntry*', на которые она ссылается,
 *
 * @param map Карта для освобождения.
 */
void scopemap_free(ScopeMap* map);


/**
 * Вставляет новую пару (ключ -> указатель) в карту.
 * Вставляет в начало списка
 * Не проверяет наличие дубликатов
 *
 * @param map Карта, в которую производится вставка.
 * @param key Простое имя (будет скопировано).
 * @param entry Указатель на TableEntry (будет сохранен).
 * @return true в случае успеха, false при ошибке
 */
bool scopemap_insert(ScopeMap* map, const char* key, TableEntry* entry);


/**
 * Ищет ключ (простое имя) *только* в этой карте.
 * Линейный поиск O(n).
 *
 * @param map Карта, в которой производится поиск.
 * @param key Простое имя для поиска.
 * @return TableEntry* если найдено, иначе NULL.
 */
TableEntry* scopemap_lookup(ScopeMap* map, const char* key);



#endif //SCOPEMAP_H