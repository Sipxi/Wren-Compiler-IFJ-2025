#include "scopemap.h"
#include <stdlib.h>
#include <string.h>

/**
 * Вспомогательная функция для копирования строки (спиздил из ast.c (что технически значит что спиздил сам у себя))
 */
static char* my_strdup(const char* s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s);
    char* new_s = (char*)malloc(len + 1);
    if (new_s == NULL) return NULL;
    memcpy(new_s, s, len + 1);
    return new_s;
}

/**
 * Создает новую пустую ScopeMap.
 */
ScopeMap* scopemap_create() {
    // calloc обнуляет память, поэтому map->head будет NULL
    ScopeMap* map = (ScopeMap*)calloc(1, sizeof(ScopeMap));
    return map; // Вернет NULL, если calloc не удался
}

/**
 * Освобождает ScopeMap и все ее узлы.
 */
void scopemap_free(ScopeMap* map) {
    if (map == NULL) {
        return;
    }

    ScopeMapNode* current = map->head;
    while (current != NULL) {
        ScopeMapNode* next = current->next; // Сохраняем указатель на следующий
        // 1. Освобождаем ключ (который мы скопировали)
        free(current->key);
        // 2. НЕ ТРОГАЕМ current->entry (func_local_table)
        // 3. Освобождаем сам узел
        free(current);

        current = next; // Переходим к следующему
    }
    // 4. Освобождаем саму структуру ScopeMap
    free(map);
}

/**
 * Вставляет новую пару (ключ -> указатель) в карту.
 */
bool scopemap_insert(ScopeMap* map, const char* key, TableEntry* entry) {
    if (map == NULL || key == NULL || entry == NULL) {
        return false;
    }

    // 1. Создаем новый узел
    ScopeMapNode* new_node = (ScopeMapNode*)malloc(sizeof(ScopeMapNode));
    if (new_node == NULL) {
        return false;
    }

    // 2. Копируем ключ (простое имя)
    new_node->key = my_strdup(key);
    if (new_node->key == NULL) {
        free(new_node);
        return false;
    }

    // 3. Сохраняем УКАЗАТЕЛЬ на TableEntry
    new_node->entry = entry;

    // 4. Вставляем в начало списка
    new_node->next = map->head;
    map->head = new_node;

    return true;
}

/**
 * Ищет ключ (простое имя) *только* в этой карте.
 */
TableEntry* scopemap_lookup(ScopeMap* map, const char* key) {
    if (map == NULL || key == NULL) {
        return NULL;
    }

    // Проходим по списку от головы
    ScopeMapNode* current = map->head;
    while (current != NULL) {
        // Сравниваем ключи
        if (strcmp(current->key, key) == 0) {
            return current->entry; // найдено
        }
        current = current->next;
    }

    return NULL; // проебались, не найдено
}