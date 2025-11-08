/* common.c
 * (Ранее dll_list.c)
 * Реализация двусвязного списка.
 */
#include "common.h"
#include <stdlib.h>
#include <string.h>


char* my_strdup(const char* s) {
    if (s == NULL) {
        return NULL;
    }

    // Получаем длину строки
    size_t len = strlen(s);

    // Выделяем память: (длина + 1) (для символа '\0')
    char* new_s = (char*)malloc(len + 1);
    if (new_s == NULL) {
        // Ошибка: не удалось выделить память
        return NULL;
    }

    // Копируем данные из старой строки в новую
    memcpy(new_s, s, len + 1);  // memcpy быстрее, чем strcpy,
                                // т.к. мы уже знаем длину

    return new_s;
}


// Функция ошибки (как требуется в common.h)
void DLL_Error() {
    fprintf(stderr, "DLL Error: Memory allocation failed or list is empty.\n");
    exit(1);
}

void DLL_Init(DLList* list) {
    list->first_element = NULL;
    list->last_element = NULL;
    list->active_element = NULL;
    list->current_length = 0;
}

void DLL_Dispose(DLList* list) {
    DLLElementPtr elem = list->first_element;
    while (elem != NULL) {
        DLLElementPtr next = elem->next_element;
        // Освобождаем данные, которые хранит узел
        // (free_tac_instruction определена в tac_playground.c)
        free_tac_instruction(elem->data); 
        free(elem);
        elem = next;
    }
    DLL_Init(list);
}

void DLL_InsertFirst(DLList* list, void *data) {
    // Используем 'sizeof(DLLElement)' - тип самой структуры
    DLLElementPtr newElem = (DLLElementPtr)malloc(sizeof(DLLElement));
    if (newElem == NULL) {
        DLL_Error();
    }
    newElem->data = data;
    newElem->prev_element = NULL;
    newElem->next_element = list->first_element;

    if (list->first_element != NULL) {
        list->first_element->prev_element = newElem;
    } else {
        // Список был пуст
        list->last_element = newElem;
    }
    list->first_element = newElem;
    list->current_length++;
}

void DLL_InsertLast(DLList* list, void *data) {
    DLLElementPtr newElem = (DLLElementPtr)malloc(sizeof(DLLElement));
    if (newElem == NULL) {
        DLL_Error();
    }
    newElem->data = data;
    newElem->next_element = NULL;
    newElem->prev_element = list->last_element;

    if (list->last_element != NULL) {
        list->last_element->next_element = newElem;
    } else {
        // Список был пуст
        list->first_element = newElem;
    }
    list->last_element = newElem;
    list->current_length++;
}

void DLL_First(DLList* list) {
    list->active_element = list->first_element;
}

void DLL_Last(DLList* list) {
    list->active_element = list->last_element;
}

void DLL_GetFirst(DLList* list, void **data) {
    if (list->first_element == NULL) {
        DLL_Error();
    }
    *data = list->first_element->data;
}

void DLL_GetLast(DLList* list, void **data) {
    if (list->last_element == NULL) {
        DLL_Error();
    }
    *data = list->last_element->data;
}

void DLL_DeleteFirst(DLList* list) {
    if (list->first_element != NULL) {
        if (list->active_element == list->first_element) {
            list->active_element = NULL;
        }
        DLLElementPtr elem = list->first_element;
        list->first_element = elem->next_element;
        if (list->first_element != NULL) {
            list->first_element->prev_element = NULL;
        } else {
            // Был последний элемент
            list->last_element = NULL;
        }
        free_tac_instruction(elem->data);
        free(elem);
        list->current_length--;
    }
}

void DLL_DeleteLast(DLList* list) {
    if (list->last_element != NULL) {
        if (list->active_element == list->last_element) {
            list->active_element = NULL;
        }
        DLLElementPtr elem = list->last_element;
        list->last_element = elem->prev_element;
        if (list->last_element != NULL) {
            list->last_element->next_element = NULL;
        } else {
            // Был последний элемент
            list->first_element = NULL;
        }
        free_tac_instruction(elem->data);
        free(elem);
        list->current_length--;
    }
}

void DLL_DeleteAfter(DLList* list) {
    if (list->active_element != NULL && list->active_element->next_element != NULL) {
        DLLElementPtr elem = list->active_element->next_element;
        list->active_element->next_element = elem->next_element;
        if (elem->next_element != NULL) {
            elem->next_element->prev_element = list->active_element;
        } else {
            // Удаляли последний
            list->last_element = list->active_element;
        }
        free_tac_instruction(elem->data);
        free(elem);
        list->current_length--;
    }
}

void DLL_DeleteBefore(DLList* list) {
    if (list->active_element != NULL && list->active_element->prev_element != NULL) {
        DLLElementPtr elem = list->active_element->prev_element;
        list->active_element->prev_element = elem->prev_element;
        if (elem->prev_element != NULL) {
            elem->prev_element->next_element = list->active_element;
        } else {
            // Удаляли первый
            list->first_element = list->active_element;
        }
        free_tac_instruction(elem->data);
        free(elem);
        list->current_length--;
    }
}

void DLL_InsertAfter(DLList* list, void *data) {
    if (list->active_element != NULL) {
        DLLElementPtr newElem = (DLLElementPtr)malloc(sizeof(DLLElement));
        if (newElem == NULL) {
            DLL_Error();
        }
        newElem->data = data;
        newElem->next_element = list->active_element->next_element;
        newElem->prev_element = list->active_element;
        list->active_element->next_element = newElem;

        if (newElem->next_element != NULL) {
            newElem->next_element->prev_element = newElem;
        } else {
            // Вставляли после последнего
            list->last_element = newElem;
        }
        list->current_length++;
    }
}

void DLL_InsertBefore(DLList* list, void *data) {
    if (list->active_element != NULL) {
        DLLElementPtr newElem = (DLLElementPtr)malloc(sizeof(DLLElement));
        if (newElem == NULL) {
            DLL_Error();
        }
        newElem->data = data;
        newElem->prev_element = list->active_element->prev_element;
        newElem->next_element = list->active_element;
        list->active_element->prev_element = newElem;

        if (newElem->prev_element != NULL) {
            newElem->prev_element->next_element = newElem;
        } else {
            // Вставляли перед первым
            list->first_element = newElem;
        }
        list->current_length++;
    }
}

void DLL_GetValue(DLList* list, void **data) {
    if (list->active_element == NULL) {
        DLL_Error();
    }
    *data = list->active_element->data;
}

void DLL_SetValue(DLList* list, void *data) {
    if (list->active_element != NULL) {
        // Важно: если мы меняем данные, старые надо освободить
        free_tac_instruction(list->active_element->data);
        list->active_element->data = data;
    }
}

void DLL_Next(DLList* list) {
    if (list->active_element != NULL) {
        list->active_element = list->active_element->next_element;
    }
}

void DLL_Previous(DLList* list) {
    if (list->active_element != NULL) {
        list->active_element = list->active_element->prev_element;
    }
}

bool DLL_IsActive(DLList* list) {
    return list->active_element != NULL;
}