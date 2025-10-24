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

void DLL_Init( DLL_Instruction_Set *list ) {
	list->first_element = NULL;
	list->active_element = NULL;
	list->last_element = NULL;
	list->current_length = 0;
}

void DLL_Dispose( DLL_Instruction_Set *list ) {
	InstructionNodePtr tmp = list->first_element; // Pomocná proměnná pro průchod seznamem
	InstructionNodePtr next; // Pomocná proměnná pro uchování dalšího prvku
	// Procházíme seznam a uvolňujeme jednotlivé prvky pokud nedojdeme na konec
	while (tmp != NULL) {
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

void DLL_InsertFirst( DLL_Instruction_Set *list, InstructionData data ) {
	InstructionNodePtr newElement = (InstructionNodePtr) malloc(sizeof(struct InstructionNode));
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

void DLL_InsertLast( DLL_Instruction_Set *list, InstructionData data ) {
	InstructionNodePtr newElement = (InstructionNodePtr) malloc(sizeof(struct InstructionNode));
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

void DLL_First( DLL_Instruction_Set *list ) {
	list->active_element = list->first_element;
}

void DLL_Last( DLL_Instruction_Set *list ) {
	list->active_element = list->last_element;
}

void DLL_GetFirst( DLL_Instruction_Set *list, InstructionData *dataPtr ) {
	if (list->first_element != NULL)
		*dataPtr = list->first_element->data;
	else 
		DLL_Error();
}

void DLL_GetLast( DLL_Instruction_Set *list, InstructionData *dataPtr ) {
	if (list->last_element != NULL)
		*dataPtr = list->last_element->data;
	else 
		DLL_Error();
}

void DLL_DeleteFirst( DLL_Instruction_Set *list ) {
	if (list->first_element == NULL) return; // Seznam je prázdný, nic neděláme
	// Pokud byl první prvek aktivní, ztrácí se aktivita
	if (list->active_element == list->first_element)
		list->active_element = NULL;
	
	InstructionNodePtr tmp = list->first_element; // Pomocná proměnná pro uvolnění prvku
	list->first_element = tmp->next_element; // Posuneme ukazatel na první prvek na další prvek
	// Pokud nový první prvek existuje, nastavíme jeho předchůdce na NULL
	if (list->first_element != NULL) 
		list->first_element->prev_element = NULL;
	// Pokud seznam byl jediný prvek, nastavíme také ukazatel na poslední prvek na NULL
	else
		list->last_element = NULL;
	free(tmp); // Uvolníme paměť původního prvního prvku
	list->current_length--;
}

void DLL_DeleteLast( DLL_Instruction_Set *list ) {
	if (list->last_element == NULL) return; // Seznam je prázdný, nic neděláme
	// Pokud byl poslední prvek aktivní, ztrácí se aktivita
	if (list->active_element == list->last_element)
		list->active_element = NULL;
	
	InstructionNodePtr tmp = list->last_element; // Pomocná proměnná pro uvolnění prvku
	list->last_element = tmp->prev_element;
	// Pokud nový poslední prvek existuje, nastavíme jeho následující prvek na NULL
	if (list->last_element != NULL)
		list->last_element->next_element = NULL;
	// Pokud seznam byl jediný prvek, nastavíme také ukazatel na první prvek na NULL
	else 
		list->first_element = NULL;
	free(tmp);
	list->current_length--;
}

void DLL_DeleteAfter( DLL_Instruction_Set *list ) {
	InstructionNodePtr active_element= list->active_element; // Pomocná proměnná pro zkrácení zápisu

	// Pokud není seznam aktivní nebo je aktivní prvek poslední, nic neděláme
	if (active_element == NULL || active_element->next_element == NULL)
		return;
	
	InstructionNodePtr afterActiveElememt = active_element->next_element; // Prvek za aktivním prvkem
	active_element->next_element = afterActiveElememt->next_element; // Přeskočíme prvek za aktivním
	// Pokud prvek za aktivním není poslední, nastavíme jeho předchůdce na aktivní prvek
	if (active_element->next_element != NULL)
		active_element->next_element->prev_element = active_element;
	// Pokud byl prvek za aktivním poslední, aktualizujeme ukazatel na poslední prvek
	else
		list->last_element = active_element;
	free(afterActiveElememt);
	list->current_length--;
}

void DLL_DeleteBefore( DLL_Instruction_Set *list ) {
	InstructionNodePtr active_element= list->active_element; // Pomocná proměnná pro zkrácení zápisu

	if (active_element == NULL || active_element->prev_element == NULL)
		return;
	
	InstructionNodePtr beforeActiveElememt = active_element->prev_element; // Prvek před aktivním prvkem
	active_element->prev_element = beforeActiveElememt->prev_element; // Přeskočíme prvek před aktivním
	// Pokud prvek před aktivním není první, nastavíme jeho následující prvek na aktivní prvek
	if (active_element->prev_element != NULL)
		active_element->prev_element->next_element = active_element;
	// Pokud byl prvek před aktivním prvním, aktualizujeme ukazatel na první prvek
	else
		list->first_element = active_element;
	free(beforeActiveElememt);
	list->current_length--;
}

void DLL_InsertAfter( DLL_Instruction_Set *list, InstructionData data ) {
	InstructionNodePtr active_element= list->active_element; // Pomocná proměnná pro zkrácení zápisu

	if (active_element == NULL) return;

	InstructionNodePtr newElement = (InstructionNodePtr) malloc(sizeof(struct InstructionNode)); // Ukazatel na nový prvek
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

void DLL_InsertBefore( DLL_Instruction_Set *list, InstructionData data ) {
	InstructionNodePtr active_element= list->active_element;
	
	if (active_element == NULL) return;

	InstructionNodePtr newElement = (InstructionNodePtr) malloc(sizeof(struct InstructionNode));
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

void DLL_GetValue( DLL_Instruction_Set *list, InstructionData *dataPtr ) {
	if (list->active_element == NULL){
		DLL_Error();
		return;
	}
	*dataPtr = list->active_element->data;
}

void DLL_SetValue( DLL_Instruction_Set *list, InstructionData data ) {
	if (list->active_element == NULL) return;
	list->active_element->data = data;
}

void DLL_Next( DLL_Instruction_Set *list ) {
	if (list->active_element == NULL) return;
	list->active_element = list->active_element->next_element;
}

void DLL_Previous( DLL_Instruction_Set *list ) {
	if (list->active_element == NULL) return;
	list->active_element = list->active_element->prev_element;
}

bool DLL_IsActive( DLL_Instruction_Set *list ) {
	return (list->active_element == NULL) ? false : true; 
}