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

void gen_operand(FrameType frame, Operand *op){
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
    fprintf(stdout, "%s", op->value.entry->key);

}

void gen_defvar(FrameType frame, Operand *var){
    fprintf(stdout, "DEFVAR ");
    gen_operand(frame, var);
    fprintf(stdout, "\n");
}

void gen_move(FrameType frame_dest, Operand *dest,
    FrameType frame_src, Operand *src){
    fprintf(stdout, "MOVE ");
    gen_operand(frame_dest, dest);
    fprintf(stdout, ", ");
    gen_operand(frame_src, src);
    fprintf(stdout, "\n");

}

// void gen_param(Quadruple *instr){
//     gen_create_frame();
//     while (instr->op != OP_PARAM) {
//         gen_defvar(instr->arg1);
//         gen_move(TF, instr->arg1, LF, instr->arg2);
        
//     }
// }

int generate_code(InstructionList *instructions, Symtable *table) {
    gen_init();
    DLL_First(instructions);
    while (instructions->active_element != NULL) {
        Quadruple *instr = (Quadruple *)instructions->active_element->data;
        switch (instr->op) {
        case OP_LABEL:
            gen_label(instr->result->value.label_name);
            break;
        case OP_JUMP:
            gen_jump(instr->result->value.label_name);
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
