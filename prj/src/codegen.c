// Имплементация функций генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//
#include "codegen.h"
#include <stdlib.h>

// * Поговорив с Gemini я понял, что мне не нужно реализовывать tzb потому-что я не работаю с регистрами

void gen_init(){
    fprintf(stdout, ".IFJcode25\nJUMP $$main\n\n");
}
void gen_push_frame(){
    fprintf(stdout, "PUSHFRAME\n");
}
void gen_create_frame(){
    fprintf(stdout, "CREATEFRAME\n");
}
void gen_pop_frame(){
    fprintf(stdout, "POPFRAME\n");
}
void gen_label(char* label_name) { 
    fprintf(stdout, "LABEL $%s:\n", label_name);
}
void gen_function(char* label_name){
    if (strcmp(label_name, "main") == 0) {
        fprintf(stdout, "$$main:\n");
        gen_create_frame();
        fprintf(stdout, "DEFVAR GF@tmp\n");
        
    } else
        gen_label(label_name);
    gen_push_frame();
}
void gen_jump(char* label_name){
    fprintf(stdout, "JUMP $%s\n", label_name);
}
void gen_call(char* label_name){
    fprintf(stdout, "CALL $%s\n", label_name);
}
void gen_return(){
    fprintf(stdout, "RETURN\n");
    gen_pop_frame();
}

void gen_jumpifeq(char* label_name){
    fprintf(stdout, "JUMPIFEQ $%s GF@tmp bool@false", label_name);
    fprintf(stdout, "\n");
}
void gen_operand(Operand *op){
    FrameType frame;
    if (op->type == OPERAND_TYPE_SYMBOL) {
        if (op->data.symbol_entry->data->local_table->nesting_level == 0)
            fprintf(stdout, "GF@");
        else
            fprintf(stdout, "TF@");
        fprintf(stdout, "%s", op->data.symbol_entry->key);
    } else if (op->type == OPERAND_TYPE_CONSTANT) {
        switch (op->data.constant.type)
        {
        case TYPE_NUM:
            fprintf(stdout, "int@%d", op->data.constant.value.int_value);
            break;
        case TYPE_FLOAT:
            fprintf(stdout, "float@%f", op->data.constant.value.float_value);
            break;
        case TYPE_STR:
            fprintf(stdout, "string@%s", op->data.constant.value.str_value);
            break;
        case TYPE_NIL:
            fprintf(stdout, "nil@nil");
            break;
        default:
            break;
        }
        // Обработка других типов операндов по мере необходимости
        return;
    } else if (op->type == OPERAND_TYPE_TEMP) {
        // Для временных переменных можно использовать LF
        fprintf(stdout, "LF@tmp$%i", op->data.temp_id);
    }

}
void gen_arithmetic(TacInstruction *instr){
    switch (instr->operation_code) {
        case OP_ADD:
            fprintf(stdout, "ADD ");
            break;
        case OP_SUBTRACT:
            fprintf(stdout, "SUB ");
            break;
        case OP_MULTIPLY:
            fprintf(stdout, "MUL ");
            break;
        case OP_DIVIDE:
            if ((instr->arg2->type == OPERAND_TYPE_CONSTANT &&
                instr->arg2->data.constant.type == TYPE_NUM &&
                instr->arg2->data.constant.value.int_value == 0) ||
                (instr->arg2->type == OPERAND_TYPE_CONSTANT &&
                instr->arg2->data.constant.type == TYPE_FLOAT &&
                instr->arg2->data.constant.value.float_value == 0.0) ||
                (instr->arg2->type == OPERAND_TYPE_SYMBOL && 
                instr->arg2->data.symbol_entry->data->)) {
                // Генерируем код для обработки деления на ноль
                fprintf(stdout, "EXIT int@9\n"); // Завершаем программу с кодом ошибки 9
            }
            fprintf(stdout, "DIV ");
            break;
        // Добавьте другие арифметические операции по мере необходимости
        default:
            break;
    }
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->arg1);
    fprintf(stdout, " ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
}

void gen_defvar(Operand *var){
    fprintf(stdout, "DEFVAR ");
    gen_operand(var);
    fprintf(stdout, "\n");
}

void gen_move(Operand *dest, Operand *src){
    fprintf(stdout, "MOVE ");
    gen_operand(dest);
    fprintf(stdout, " ");
    gen_operand(src);
    fprintf(stdout, "\n");

}

void gen_param(TacInstruction *instr){
    gen_create_frame();
    while (instr->operation_code != OP_PARAM) {
        gen_defvar(instr->arg1);
        gen_move(instr->arg1, instr->arg2);
        DLL_Next(instr);
    }
}

int generate_code(DLList *instructions, Symtable *table) {
    gen_init();
    DLL_First(instructions);
    while (instructions->active_element != NULL) {
        TacInstruction *instr = (TacInstruction *)instructions->active_element->data;
        switch (instr->operation_code) {
        case OP_JUMP:
            gen_jump(instr->result->data.label_name);
            break;
        case OP_JUMP_IF_FALSE:
            gen_jumpifeq(instr->arg1->data.label_name);
            break;
        case OP_LABEL:
            gen_label(instr->result->data.label_name);
            break;
        case OP_ADD:
        case OP_SUBTRACT:
        case OP_MULTIPLY:
        case OP_DIVIDE:
            gen_arithmetic(instr);
            break;
        case OP_FUNCTION_BEGIN:
            // Обработка начала функции
            gen_function(instr->result->data.label_name);
            break;
        case OP_RETURN:
            gen_return();
            break;
        case OP_PARAM:
            gen_param(instr);
            break;
        default:
            break;
        }
        DLL_Next(instructions);
    }
    return 0;
}
