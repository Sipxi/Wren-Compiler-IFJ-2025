// Имплементация функций генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//
#include "codegen.h"
#include <stdlib.h>

// * Поговорив с Gemini я понял, что мне не нужно реализовывать tzb потому-что я не работаю с регистрами


void gen_init(){
    fprintf(stdout, ".IFJcode25\n");
    fprintf(stdout, "DEFVAR GF@tmp\n");
    fprintf(stdout, "DEFVAR GF@tmp_type_1\n");
    fprintf(stdout, "DEFVAR GF@tmp_type_2\n");
    
    fprintf(stdout, "JUMP $$main\n\n");
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

void gen_jumpifeq(TacInstruction *instr){
    // Если результат в GF@tmp
    // fprintf(stdout, "JUMPIFEQ $%s GF@tmp bool@false", label_name);
    // !Пока делаем из резулата
    fprintf(stdout, "JUMPIFEQ $%s ", instr->arg2->data.label_name);
    gen_operand(instr->arg1);
    fprintf(stdout, " bool@false\n");

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
void gen_tac(TacInstruction *instr){
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->arg1);
    fprintf(stdout, " ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
}

void gen_type_check(TacInstruction *instr) {
    fprintf(stdout, "TYPE GF@tmp_type_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@tmp_type_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
}
void gen_divide(TacInstruction *instr){
    gen_type_check(instr);
    char *label_div = create_unique_label("$DIV");
    char *label_idiv = create_unique_label("$IDIV");
    char *label_end = create_unique_label("$END_DIV");
    // Проверка типов
    fprintf(stdout, "JUMPIFNEQ $EXIT53 GF@tmp_type_1 GF@tmp_type_2\n");

    // разделение на деление float и int 
    fprintf(stdout, "JUMPIFEQ $%s GF@tmp_type_1 string@float", label_div);
    fprintf(stdout, "JUMPIFEQ $%s GF@tmp_type_1 string@int", label_idiv);
    // если не float и не int то ошибка
    gen_jump("$EXIT53");

    // деление float
    gen_label(label_div);
    //деление на ноль
    fprintf(stdout, "JUMPIFEQ $EXIT57 ");
    gen_operand(instr->arg2);
    fprintf(stdout, " float@0.0\n");
    //деление
    fprintf(stdout, "DIV ");
    // прыжок в конец
    fprintf(stdout, "JUMP %s\n", label_end);

    // деление int
    gen_label(label_idiv);
    //деление на ноль
    fprintf(stdout, "JUMPIFEQ $EXIT57 ");
    gen_operand(instr->arg2);
    fprintf(stdout, " int@0\n");
    //деление
    fprintf(stdout, "IDIV ");
    gen_tac(instr);

    gen_label(label_end);

}
void gen_arithmetic(TacInstruction *instr){
    gen_type_check(instr);
    // Проверка типов
    fprintf(stdout, "JUMPIFNEQ $EXIT53 GF@tmp_type_1 GF@tmp_type_2\n");
    fprintf(stdout, "JUMPIFEQ $EXIT53 GF@tmp_type_1 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT53 GF@tmp_type_1 string@bool\n");

    switch (instr->operation_code) {
        case OP_ADD:
            fprintf(stdout, "JUMPIFEQ $EXIT53 GF@tmp_type_1 string@string\n");
            fprintf(stdout, "ADD ");
            break;
        case OP_SUBTRACT:
            fprintf(stdout, "JUMPIFEQ $EXIT53 GF@tmp_type_1 string@string\n");
            fprintf(stdout, "SUB ");
            break;
        case OP_MULTIPLY:
            fprintf(stdout, "JUMPIFEQ $EXIT53 GF@tmp_type_1 string@string\n");
            fprintf(stdout, "MUL ");
            break;
        case OP_CONCAT:
            fprintf(stdout, "JUMPIFNEQ $EXIT53 GF@tmp_type_1 string@string\n");
            fprintf(stdout, "CONCAT ");
            break;
        // Добавьте другие арифметические операции по мере необходимости
        default:
            break;
    }
    gen_tac(instr);
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
gen_end(){
    for (int i = 50; i < 59; i++) {
        printf(stdout, "LABEL $Exit%i\n", i);
        printf(stdout, "EXIT int@%i\n", i); 
        printf(stdout, "JUMP $End\n");
    }
    printf(stdout, "LABEL $Exit60\n");
    printf(stdout, "EXIT int@60\n"); 
    printf(stdout, "JUMP $End\n");

    printf(stdout, "LABEL $Exit0\n");
    printf(stdout, "EXIT int@0\n"); 

    printf(stdout, "LABEL $End\n");
}
int generate_code(DLList *instructions, Symtable *table) {
    gen_init();
    DLL_First(instructions);
    while (instructions->active_element != NULL) {
        TacInstruction *instr = (TacInstruction *)instructions->active_element->data;
        switch (instr->operation_code) {
        case OP_JUMP:
            gen_jump(instr->arg1->data.label_name);
            break;
        case OP_JUMP_IF_FALSE:
            gen_jumpifeq(instr);
            break;
        case OP_LABEL:
            gen_label(instr->result->data.label_name);
            break;
        case OP_ADD:
        case OP_SUBTRACT:
        case OP_MULTIPLY:
        case OP_CONCAT:
            gen_arithmetic(instr);
            break;
        case OP_DIVIDE:
            gen_divide(instr);
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
    gen_end();
    return 0;
}
