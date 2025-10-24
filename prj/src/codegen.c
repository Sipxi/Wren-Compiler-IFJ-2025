// Имплементация функций генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//
#include "codegen.h"

void convert_Instructions_To_TZB(DLList* instruction_list) {
    DLL_First(instruction_list);
    DLLElementPtr active_element;
    // Проходим по всем элементам списка и преобразуем InstructionData в TZB_Data
    while (!DLL_IsActive(instruction_list)) {
        active_element = instruction_list->active_element;
        TZB_Data *newData = (TZB_Data*) malloc (sizeof(TZB_Data));
        if (newData == NULL){
            DLL_Error();
            return NULL;
        }
        newData->data = *((InstructionData *) active_element->data);
        free(active_element->data);
        newData->info_arg1.is_live = false;
        newData->info_arg1.next_use_line = -1;
        newData->info_arg2.is_live = false;
        newData->info_arg2.next_use_line = -1;
        newData->info_result.is_live = false;
        newData->info_result.next_use_line = -1;
        active_element->data = newData;
    } 
}