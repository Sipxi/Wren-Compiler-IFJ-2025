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

void gen_label(char* label_name){
    char *prefix = "fun$";
    if (strcmp(label_name, "main") == 0) {
        fprintf(stdout, "$$main:\n");
        gen_create_frame();
        gen_push_frame();
    } else if (strncmp(label_name, prefix, strlen(prefix)) == 0) {
        fprintf(stdout, "$%s:\n", label_name);
        gen_push_frame();
    } else 
        fprintf(stdout, "$%s:\n", label_name);
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

void gen_operand(Operand *op){
    FrameType frame;
    if (op->type == OPERAND_TYPE_SYMBOL) {
        if (op->data.symbol_entry->data->is_defined)
            if (op->data.symbol_entry->data->local_table->nesting_level == 0)
            frame = GF;
        else
            frame = LF;
    } else {
        // Обработка других типов операндов по мере необходимости
        return;
    }
    switch (frame){
    case TF:
        fprintf(stdout, "TF@");
        break;
    case GF:
        fprintf(stdout, "GF@");
        break;
    case LF:
        fprintf(stdout, "LF@");
        break;
    case INT:
        fprintf(stdout, "int@");
        break;
    case FLOAT:
        fprintf(stdout, "float@");
        break;
    case STRING:
        fprintf(stdout, "string@");
        break;
    case BOOL:
        fprintf(stdout, "bool@");
        break;
    case NIL:
        fprintf(stdout, "nil@");
        break;
    default:
        break;
    }
    fprintf(stdout, "%s", op->data.symbol_entry->key);

}

void gen_defvar(FrameType frame, Operand *var){
    fprintf(stdout, "DEFVAR ");
    gen_operand(frame, var);
    fprintf(stdout, "\n");
}

void gen_move(Operand *dest, Operand *src){
    fprintf(stdout, "MOVE ");
    gen_operand(dest->frame, dest);
    fprintf(stdout, ", ");
    gen_operand(src->frame, src);
    fprintf(stdout, "\n");

}

void gen_param(TacInstruction *instr){
    gen_create_frame();
    while (instr->operation_code != OP_PARAM) {
        gen_defvar(TF, instr->arg1);
        gen_move(instr->arg1, instr->arg2);
        DLL_Next(instr);
    }
    }
}

int generate_code(DLList *instructions, Symtable *table) {
    gen_init();
    DLL_First(instructions);
    while (instructions->active_element != NULL) {
        TacInstruction *instr = (TacInstruction *)instructions->active_element->data;
        switch (instr->operation_code) {
        case OP_LABEL:
            gen_label(instr->result->data.label_name);
            break;
        case OP_JUMP:
            gen_jump(instr->result->data.label_name);
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
