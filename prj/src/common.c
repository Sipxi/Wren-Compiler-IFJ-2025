// Реализация структур и функций, которые нужные нескольким модулям
//
// Авторы: 
// Serhij Čepil (253038)
// Dmytro Kravchenko (273125)
//! Допишите ваши имена и номера

#include "common.h"
#include <stdbool.h>

//! Переделать потом
void DLL_Error(void) {
	printf("*ERROR* The program has performed an illegal operation.\n");
}

void *Gen_InstructionData(InstructionData data){
    InstructionData *newData = (InstructionData *) malloc(sizeof(InstructionData));
    if (newData == NULL){
        DLL_Error();
        return NULL;
    }
    newData->op = data.op;
    newData->arg1 = data.arg1;
    newData->arg2 = data.arg2;
    newData->result = data.result;
    return newData;
}


/* ======================================*/
/* ===== Реализация публичных функций для работы с DLL =====*/
/* ======================================*/

// ! ПРИМЕР РАБОТЫ С DLL ЧЕРЕЗ VOID POINTERS
/*
	DLList list;
    int* input_1 = (int *)malloc(sizeof(int));
    int* input_2 = (int *)malloc(sizeof(int));
    *input_1 = 1;
    *input_2 = 2; 
    int* print_val;

    DLL_Init(&list);
    DLL_InsertFirst(&list, (void *)input_1);
    DLL_First(&list);
    DLL_InsertAfter(&list, (void *) input_2);
    
    DLL_DeleteLast(&list);
    DLL_GetLast(&list, (void **)&print_val);

    printf("Last value: %d\n", *print_val);

    DLL_Dispose(&list); // Не нужно после освобождать input_1 и input_2
*/

void DLL_Init( DLList *list ) {
	list->first_element = NULL;
	list->active_element = NULL;
	list->last_element = NULL;
	list->current_length = 0;
}

void DLL_Dispose( DLList *list ) {
	DLLElementPtr tmp = list->first_element; // Pomocná proměnná pro průchod seznamem
	DLLElementPtr next; // Pomocná proměnná pro uchování dalšího prvku
	// Procházíme seznam a uvolňujeme jednotlivé prvky pokud nedojdeme na konec
	while (tmp != NULL) {
        free(tmp->data);
		next = tmp->next_element;
		free(tmp);
		tmp = next;
	}
	// Inicializujeme seznam do prazdního stavu
	list->first_element = NULL;
	list->active_element = NULL;
	list->last_element = NULL;
	list->current_length = 0;
}

void DLL_InsertFirst( DLList *list, void *data ) {
	DLLElementPtr newElement = (DLLElementPtr) malloc(sizeof(struct DLLElement));
	if (newElement == NULL) {
		DLL_Error();
		return;
	}

	newElement->data = data; // Nastavení hodnoty nového prvku
	newElement->prev_element = NULL; // Nový prvek bude první, nemá předchůdce
	newElement->next_element = list->first_element; // Nový prvek bude ukazovat na bývalý první prvek
	// Pokud seznam nebyl prázdný, nastavíme předchůdce bývalého prvního prvku na nový prvek
	if (list->first_element != NULL) 
		list->first_element->prev_element = newElement;
	// Pokud byl seznam prázdný, nastavíme také ukazatel na poslední prvek
	else
		list->last_element = newElement;
	list->first_element = newElement;
	list->current_length++;
}

void DLL_InsertLast( DLList *list, void *data ) {
	DLLElementPtr newElement = (DLLElementPtr) malloc(sizeof(struct DLLElement));
	if (newElement == NULL) {
		DLL_Error();
		return;
	}
	newElement->data = data;
	newElement->next_element = NULL;
	newElement->prev_element = list->last_element;
	// Pokud seznam nebyl prázdný, nastavíme následující prvek bývalého posledního prvku na nový prvek
	if (list->last_element != NULL)
		list->last_element->next_element = newElement;
	// Pokud byl seznam prázdný, nastavíme také ukazatel na první prvek
	else
		list->first_element = newElement;
	list->last_element = newElement;
	list->current_length++;
}

void DLL_First( DLList *list ) {
	list->active_element = list->first_element;
}

void DLL_Last( DLList *list ) {
	list->active_element = list->last_element;
}

void DLL_GetFirst( DLList *list, void **dataPtr ) {
	if (list->first_element != NULL)
		*dataPtr = list->first_element->data;
	else 
		DLL_Error();
}

void DLL_GetLast( DLList *list, void **dataPtr ) {
	if (list->last_element != NULL)
		*dataPtr = list->last_element->data;
	else 
		DLL_Error();
}

void DLL_DeleteFirst( DLList *list ) {
	if (list->first_element == NULL) return; // Seznam je prázdný, nic neděláme
	// Pokud byl první prvek aktivní, ztrácí se aktivita
	if (list->active_element == list->first_element)
		list->active_element = NULL;
	
	DLLElementPtr tmp = list->first_element; // Pomocná proměnná pro uvolnění prvku
	list->first_element = tmp->next_element; // Posuneme ukazatel na první prvek na další prvek
	// Pokud nový první prvek existuje, nastavíme jeho předchůdce na NULL
	if (list->first_element != NULL) 
		list->first_element->prev_element = NULL;
	// Pokud seznam byl jediný prvek, nastavíme také ukazatel na poslední prvek na NULL
	else
		list->last_element = NULL;
    free(tmp->data);
	free(tmp); // Uvolníme paměť původního prvního prvku
	list->current_length--;
}

void DLL_DeleteLast( DLList *list ) {
	if (list->last_element == NULL) return; // Seznam je prázdný, nic neděláme
	// Pokud byl poslední prvek aktivní, ztrácí se aktivita
	if (list->active_element == list->last_element)
		list->active_element = NULL;
	
	DLLElementPtr tmp = list->last_element; // Pomocná proměnná pro uvolnění prvku
	list->last_element = tmp->prev_element;
	// Pokud nový poslední prvek existuje, nastavíme jeho následující prvek na NULL
	if (list->last_element != NULL)
		list->last_element->next_element = NULL;
	// Pokud seznam byl jediný prvek, nastavíme také ukazatel na první prvek na NULL
	else 
		list->first_element = NULL;
    free(tmp->data);
	free(tmp);
	list->current_length--;
}

void DLL_DeleteAfter( DLList *list ) {
	DLLElementPtr active_element= list->active_element; // Pomocná proměnná pro zkrácení zápisu

	// Pokud není seznam aktivní nebo je aktivní prvek poslední, nic neděláme
	if (active_element == NULL || active_element->next_element == NULL)
		return;
	
	DLLElementPtr afterActiveElememt = active_element->next_element; // Prvek za aktivním prvkem
	active_element->next_element = afterActiveElememt->next_element; // Přeskočíme prvek za aktivním
	// Pokud prvek za aktivním není poslední, nastavíme jeho předchůdce na aktivní prvek
	if (active_element->next_element != NULL)
		active_element->next_element->prev_element = active_element;
	// Pokud byl prvek za aktivním poslední, aktualizujeme ukazatel na poslední prvek
	else
		list->last_element = active_element;
	free(afterActiveElememt->data);
	free(afterActiveElememt);
	list->current_length--;
}

void DLL_DeleteBefore( DLList *list ) {
	DLLElementPtr active_element= list->active_element; // Pomocná proměnná pro zkrácení zápisu

	if (active_element == NULL || active_element->prev_element == NULL)
		return;
	
	DLLElementPtr beforeActiveElememt = active_element->prev_element; // Prvek před aktivním prvkem
	active_element->prev_element = beforeActiveElememt->prev_element; // Přeskočíme prvek před aktivním
	// Pokud prvek před aktivním není první, nastavíme jeho následující prvek na aktivní prvek
	if (active_element->prev_element != NULL)
		active_element->prev_element->next_element = active_element;
	// Pokud byl prvek před aktivním prvním, aktualizujeme ukazatel na první prvek
	else
		list->first_element = active_element;
	free(beforeActiveElememt->data);
	free(beforeActiveElememt);
	list->current_length--;
}

void DLL_InsertAfter( DLList *list, void *data ) {
	DLLElementPtr active_element= list->active_element; // Pomocná proměnná pro zkrácení zápisu

	if (active_element == NULL) return;

	DLLElementPtr newElement = (DLLElementPtr) malloc(sizeof(struct DLLElement)); // Ukazatel na nový prvek
	if (newElement == NULL){
		DLL_Error();
		return;
	}

	newElement->data = data; // Hodnota nového prvku
	newElement->prev_element = active_element; // Nový prvek bude ukazovat na aktivní prvek jako na předchůdce
	newElement->next_element = active_element->next_element; // Nový prvek bude ukazovat na následující prvek aktivního prvku
	// Pokud je aktivní prvek poslední, aktualizujeme ukazatel na posled
	if (active_element->next_element == NULL)
		list->last_element = newElement;
	// Pokud není aktivní prvek poslední, nastavíme předchůdce následujícího prvku na nový prvek
	else 
		active_element->next_element->prev_element = newElement;
	active_element->next_element = newElement; // Aktivní prvek bude ukazovat na nový prvek jako na následující
	list->current_length++;
}

void DLL_InsertBefore( DLList *list, void *data ) {
	DLLElementPtr active_element= list->active_element;
	
	if (active_element == NULL) return;

	DLLElementPtr newElement = (DLLElementPtr) malloc(sizeof(struct DLLElement));
	if (newElement == NULL){
		DLL_Error();
		return;
	}

	newElement->data = data;
	newElement->next_element = active_element;
	newElement->prev_element = active_element->prev_element;
	if (active_element->prev_element == NULL)
		list->first_element = newElement;
	else 
		active_element->prev_element->next_element = newElement;
	active_element->prev_element = newElement;
	list->current_length++;
}



void DLL_GetValue( DLList *list, void **dataPtr ) {
	if (list->active_element == NULL){
		DLL_Error();
		return;
	}
	*dataPtr = list->active_element->data;
}

void DLL_SetValue( DLList *list, void *data ) {
	if (list->active_element == NULL) return;
	list->active_element->data = data;
}

void DLL_Next( DLList *list ) {
	if (list->active_element == NULL) return;
	list->active_element = list->active_element->next_element;
}

void DLL_Previous( DLList *list ) {
	if (list->active_element == NULL) return;
	list->active_element = list->active_element->prev_element;
}

bool DLL_IsActive( DLList *list ) {
	return (list->active_element == NULL) ? false : true; 
}