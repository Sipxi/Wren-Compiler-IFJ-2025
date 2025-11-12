/** 
 * @file symtable.c
 * 
 * @brief Файл реализации для таблицы символов
 *
 * Author:
 *      - Serhij Čepil (253038)
 */
#include "symtable.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 8  //! ИЗМЕНИТЬ ПРИ РАБОТЕ НА БОЛЬШЕ(например 64)
// "Магическое" число для инициализации хеша DJB2
#define HASH_NUMBER 5381
#define LOAD_FACTOR_THRESHOLD 0.5
#define RESIZE_FACTOR 2

/* ======================================*/
/* ===== Декларация приватных функций =====*/
/* ======================================*/

/**
 * Изменяет размер таблицы символов c соответствующим LOAD_FACTOR_THRESHOLD.
 * Использует symtable_rehash для перераспределения записей
 *
 * @param table Указатель на таблицу символов для изменения размера.
 */
static bool symtable_resize(Symtable* table);

/**
 * Проверяет, превышен ли LOAD_FACTOR_THRESHOLD для таблицы символов.
 *
 * @param table Указатель на таблицу символов для проверки.
 * @return true если текущий коэффициент загрузки превышает
 * LOAD_FACTOR_THRESHOLD, иначе false.
 */
static bool check_load_factor(Symtable* table) {
    return ((double)table->count / (double)table->capacity) >=
           LOAD_FACTOR_THRESHOLD;
}


/* ======================================*/
/* ===== Реализация приватных функций =====*/
/* ======================================*/

static void symtable_insert_rehash(Symtable* table, TableEntry* entry) {
    size_t hash_index = get_hash(entry->key, table->capacity);
    while (table->entries[hash_index].status == SLOT_OCCUPIED) {
        hash_index = (hash_index + 1) % table->capacity;
    }
    // Скопировать запись в новый слот
    table->entries[hash_index] = *entry;
    table->entries[hash_index].status = SLOT_OCCUPIED;
    table->count++;
}

static bool symtable_resize(Symtable* table) {
    size_t new_capacity = table->capacity * RESIZE_FACTOR;
    TableEntry* new_entries = calloc(new_capacity, sizeof(TableEntry));
    if (new_entries == NULL) {
        return false;  // Ошибка выделения памяти, оставляем таблицу без
                       // изменений
    }
    // Сохраняем старые записи
    TableEntry* old_entries = table->entries;
    size_t old_capacity = table->capacity;
    table->entries = new_entries;
    table->capacity = new_capacity;
    table->count = 0;

    // Перехешируем старые записи
    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].status == SLOT_OCCUPIED) {
            symtable_insert_rehash(table, &old_entries[i]);
        }
    }

    free(old_entries);
    return true;
}

/* ======================================*/
/* ===== Реализация публичных функций =====*/
/* ======================================*/

bool symtable_init(Symtable* table) {
    table->entries = calloc(INITIAL_CAPACITY, sizeof(TableEntry));
    if (table->entries == NULL) {
        return false;  // Ошибка выделения памяти
    }
    table->count = 0;
    table->capacity = INITIAL_CAPACITY;
    return true;
}

size_t get_hash(const char* key, size_t capacity) {
    // Функция вычисляет хеш для строки key
    // Использует алгоритм DJB2 и возвращает индекс в пределах capacity

    unsigned long hash = HASH_NUMBER;  // Инициализация хеша "магическим" числом
                                       // 5381 (используется в DJB2)
    int c;  // Переменная для текущего символа из строки

    while (
        (c = *key++)) {  // Цикл по каждому символу строки до символа конца '\0'
        // *key++ берёт текущий символ и сдвигает указатель на следующий
        hash = ((hash << 5) + hash) + c;  // hash = hash * 33 + c
        // (hash << 5) + hash = hash * 32 + hash = hash * 33
        // Такая операция даёт хорошее распределение хеша для коротких строк
    }

    return (size_t)(hash %
                    capacity);  // Ограничиваем результат размером таблицы
    // % capacity гарантирует, что индекс попадёт в диапазон [0, capacity-1]
}

TableEntry* symtable_lookup(Symtable* table, const char* key) {
	size_t hash_index =
        get_hash(key, table->capacity);  // Вычисляем хеш-индекс для ключа
    size_t original_index = hash_index;  // Сохраняем оригинальный индекс для
                                         // обнаружения полного обхода
    TableEntry* entry = &table->entries[hash_index];  // Получаем указатель на
                                                      // запись по хеш-индексу

    while (entry->status != SLOT_EMPTY) {
        if (entry->status == SLOT_OCCUPIED && strcmp(entry->key, key) == 0) {
            return entry;  // Если запись занята и ключи совпадают, возвращаем
                           // запись
        }
        // Линейное пробирование: переходим к следующему слоту
        // модуль нужно для обхода в случае достижения конца массива
        hash_index = (hash_index + 1) % table->capacity;
        entry = &table->entries[hash_index];
        if (hash_index == original_index) {
            // Мы обошли всю таблицу и не нашли запись
            break;
        }
    }
    return NULL;  // Если запись не найдена, возвращаем NULL
}

bool symtable_insert(Symtable* table, const char* key, SymbolData* data) {
    // Проверяем необходимость изменения размера таблицы
    if (check_load_factor(table)) {
        if (!symtable_resize(table)) {
            return false;  // Ошибка изменения размера, вставка не выполнена
        }
    }

    size_t hash_index = get_hash(key, table->capacity);
    size_t original_index = hash_index;
    TableEntry* entry = &table->entries[hash_index];

    while (entry->status == SLOT_OCCUPIED) {
        if (strcmp(entry->key, key) == 0) {
            // Ключ уже существует, обновляем данные
            free(entry->data);
            entry->data = malloc(sizeof(SymbolData));
            
            if (entry->data == NULL) {
                free(entry->key);  // Освобождаем ключ при ошибке
                return false;      // Ошибка выделения памяти для данных
            }
            memcpy(entry->data, data, sizeof(SymbolData));  // Копируем данные
            return true;
        }
        hash_index = (hash_index + 1) % table->capacity;
        entry = &table->entries[hash_index];
        if (hash_index == original_index) {
            // Таблица полна (хотя это не должно случиться из-за проверки выше)
            return false;
        }
    }

    // Вставляем новую запись
    entry->key = strdup_c99(key);  // Копируем ключ
    if (entry->key == NULL) {
        return false;  // Ошибка выделения памяти для ключа
    }
    entry->data = malloc(sizeof(SymbolData));
    if (entry->data == NULL) {
        free(entry->key);  // Освобождаем ключ при ошибке
        return false;      // Ошибка выделения памяти для данных
    }
    memcpy(entry->data, data, sizeof(SymbolData));  // Копируем данные
    entry->status = SLOT_OCCUPIED;  // Обновляем статус слота
    table->count++;                 // Увеличиваем количество записей

    return true;
}

void symtable_delete(Symtable* table, const char* key) {
    TableEntry* entry = symtable_lookup(table, key);
    if (entry != NULL) {
        entry->status = SLOT_DELETED;  // Помечаем слот как удалённый
        table->count--;                // Уменьшаем количество записей
        // Освобождение памяти для ключа и данных
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
            free(entry->key);   // Освобождаем память для ключа
            free(entry->data);  // Освобождаем память для данных символа
        }
    }
    free(table->entries);  // Освобождаем массив записей
    table->entries = NULL;
    table->count = 0;
    table->capacity = 0;
}

/* ===================================================*/
/* ===== Вспомогательные функции для печати (НОВЫЕ) =====*/
/* ===================================================*/

/**
 * Преобразует enum SlotStatus в читаемую строку.
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
 * Преобразует enum SymbolKind в читаемую строку.
 */
static const char* kind_to_string(SymbolKind kind) {
    switch (kind) {
        case KIND_VAR:    return "Variable";
        case KIND_FUNC:   return "Function";
    }
    return "UNKNOWN_KIND";
}

/**
 * Преобразует enum DataType в читаемую строку.
 */
static const char* type_to_string(DataType type) {
    switch (type) {
        case TYPE_NUM:    return "Number";
        case TYPE_STR:    return "String";
        case TYPE_NIL:    return "Nil";
        case TYPE_NULL:   return "Null";
        case TYPE_FLOAT:  return "Float";
        // TODO: Добавьте другие типы, когда они у вас появятся
    }
    return "UNKNOWN_TYPE";
}

/**
 * Печатает заданное количество пробелов для отступа (для вложенности).
 */
static void print_indent(int level) {
    for (int i = 0; i < level; i++) {
        printf("    "); // 4 пробела на один уровень отступа
    }
}

// Предварительное объявление внутренней рекурсивной функции,
// чтобы symtable_print_entry могла на нее ссылаться при печати вложенных таблиц.
static void symtable_print_internal(Symtable *table, int level);

/**
 * Печатает ОДНУ запись (один слот) таблицы символов.
 * Эта функция вызывается в цикле из 'symtable_print_internal'.
 *
 * @param entry Указатель на запись (слот) для печати.
 * @param index Индекс этого слота в массиве.
 * @param level Текущий уровень отступа.
 */
static void symtable_print_entry(TableEntry *entry, size_t index, int level) {
    // 1. Печатаем базовую информацию о слоте
    print_indent(level);
    printf("Слот %3zu: [%-8s] ", index, status_to_string(entry->status));

    // 2. Если слот занят, печатаем подробные данные
    if (entry->status == SLOT_OCCUPIED) {
        // Проверка на поврежденные данные (если key или data вдруг NULL)
        if (entry->key == NULL || entry->data == NULL) {
            printf("-> !!! ПОВРЕЖДЕННАЯ ЗАПИСЬ !!!\n");
            return; // Больше печатать нечего
        }

        // Печатаем ключ
        printf("-> Ключ: \"%s\"\n", entry->key);

        // 3. Печатаем детали из SymbolData (с дополнительным отступом)
        SymbolData *data = entry->data;
        print_indent(level + 1); // +1 уровень отступа
        printf("Вид:     %s\n", kind_to_string(data->kind));
        print_indent(level + 1);
        printf("Тип:     %s\n", type_to_string(data->data_type));
        print_indent(level + 1);
        printf("Опред:   %s\n", data->is_defined ? "true" : "false");

        // 4. Рекурсивный вызов для локальной таблицы (если это функция)
        if (data->local_table != NULL) {
            print_indent(level + 1);
            printf("Лок. таблица:\n");
            // Вызываем главную рекурсивную функцию с большим отступом
            symtable_print_internal(data->local_table, level + 2);
        } else {
            print_indent(level + 1);
            printf("Лок. таблица: (NULL)\n");
        }
    } else {
        // Для EMPTY и DELETED просто переводим строку
        printf("\n");
    }
}


/**
 * Внутренняя рекурсивная функция для печати всей таблицы.
 * (Эта функция нужна для обработки уровней вложенности).
 *
 * @param table Указатель на таблицу для печати.
 * @param level Текущий уровень отступа.
 */
static void symtable_print_internal(Symtable *table, int level) {
    // Защита от NULL (например, если local_table у функции не инициализирована)
    if (table == NULL) {
        print_indent(level);
        printf("(NULL Локальная таблица)\n");
        return;
    }

    // --- 1. Заголовок таблицы ---
    print_indent(level);
    printf("--- Таблица Символов (Уровень %d) ---\n", level);
    print_indent(level);
    printf("  Вместимость: %zu\n", table->capacity);
    print_indent(level);
    printf("  Количество:  %zu\n", table->count);
    print_indent(level);
    
    // Рассчитываем и печатаем фактор загрузки
    double load_factor = (table->capacity == 0) ? 0.0 : (double)table->count / (double)table->capacity;
    printf("  Загрузка:    %.2f%% (Порог: %.0f%%)\n", 
           load_factor * 100.0, 
           LOAD_FACTOR_THRESHOLD * 100.0); // В процентах
    print_indent(level);
    printf("------------------------------------\n");

    // --- 2. Печать всех записей слотов ---
    for (size_t i = 0; i < table->capacity; i++) {
        // Вызываем функцию печати для каждой отдельной записи
        symtable_print_entry(&table->entries[i], i, level);
    }
}



/**
 * Публичная функция обертка для печати таблицы.
 * Она создает красивый заголовок и вызывает внутреннюю 
 * рекурсивную функцию, начиная с уровня 0.
 */
void symtable_print(Symtable *table) {
    // Общий заголовок
    printf("\n");
    printf("======================================================\n");
    printf("                ОТЛАДОЧНЫЙ ВЫВОД ТАБЛИЦЫ\n");
    printf("======================================================\n");
    
    // Проверка, не передали ли нам NULL
    if (table == NULL) {
        printf("(NULL Таблица)\n");
    } else {
        // Запускаем рекурсивную печать.
        // Уровень 0 - это самый верхний (глобальный) уровень.
        symtable_print_internal(table, 0);
    }
    
    // Общий футер
    printf("======================================================\n\n");
}
