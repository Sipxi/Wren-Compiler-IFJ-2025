// Имплементация функций генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//
#include "codegen.h"
#include "printer.h"
#include <stdlib.h>


void gen_init(Symtable *table){
    fprintf(stdout, ".IFJcode25\n");
    fprintf(stdout, "DEFVAR GF@$tmp_2\n");
    fprintf(stdout, "DEFVAR GF@$tmp\n");
    fprintf(stdout, "DEFVAR GF@$tmp_type_1\n");
    fprintf(stdout, "DEFVAR GF@$tmp_type_2\n");
    fprintf(stdout, "DEFVAR GF@$tmp_op_1\n");
    fprintf(stdout, "DEFVAR GF@$tmp_op_2\n");
    fprintf(stdout, "DEFVAR GF@ret_ifj_fun\n");
    for (size_t i = 0; i < table->capacity; i++) {
        TableEntry* entry = &table->entries[i];
        if (entry->status == SLOT_OCCUPIED) {
            // Определяем переменную в GF
            if (entry->data->kind == KIND_VAR) {
                fprintf(stdout, "DEFVAR GF@%s\n", entry->key);
            }
        }
    }

    gen_create_frame();
    fprintf(stdout, "CALL $$main\n\n");
    fprintf(stdout, "EXIT int@0\n");
    fprintf(stdout, "LABEL $EXIT25\n");
    fprintf(stdout, "EXIT int@25\n");
    fprintf(stdout, "LABEL $EXIT26\n");
    fprintf(stdout, "EXIT int@26\n");
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
    fprintf(stdout, "LABEL $%s\n", label_name);
}
void gen_jump(char* label_name){
    fprintf(stdout, "JUMP $%s\n", label_name);
}
void gen_call(char* label_name){
    fprintf(stdout, "CALL $%s\n", label_name);
}
void gen_return(TacInstruction *instr){
    if (instr->result != NULL) {
        fprintf(stdout, "MOVE LF@ret ");
        gen_operand(instr->result);
        fprintf(stdout, "\n");
    } else 
        fprintf(stdout, "MOVE LF@ret nil@nil\n");
    gen_pop_frame();
    fprintf(stdout, "RETURN\n");
}

void gen_jumpifeq(TacInstruction *instr){
    // Если результат в GF@$tmp
    // fprintf(stdout, "JUMPIFEQ $%s GF@$tmp bool@false", label_name);
    // !Пока делаем из резулата
    fprintf(stdout, "JUMPIFEQ $%s ", instr->arg2->data.label_name);
    gen_operand(instr->arg1);
    fprintf(stdout, " bool@false\n");

}
void gen_operand(Operand *op){
    if (op->type == OPERAND_TYPE_SYMBOL) {
        fprintf(stdout, "LF@%s", op->data.symbol_entry->data->unique_name);
    } else if (op->type == OPERAND_TYPE_CONSTANT) {
        switch (op->data.constant.type)
        {
        case TYPE_NUM:
            fprintf(stdout, "int@%i", op->data.constant.value.int_value);
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
        fprintf(stdout, "LF@$t$%i", op->data.temp_id);
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
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
}
void gen_divide(TacInstruction *instr){
    char *label_div = create_unique_label("DIV");
    char *label_idiv = create_unique_label("IDIV");
    char *label_end = create_unique_label("END_DIV");
    // Проверка типов
    gen_type_check(instr);
    gen_same_operand_check(instr);
    fprintf(stdout, "TYPE GF@$tmp_type_1 GF@$tmp_op_1\n");
    // разделение на деление float и int 
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@float", label_div);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@int", label_idiv);
    // если не float и не int то ошибка
    gen_jump("$EXIT26");

    // деление float
    gen_label(label_div);
    //деление
    fprintf(stdout, "DIV ");
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");
    // прыжок в конец
    fprintf(stdout, "JUMP %s\n", label_end);

    // деление int
    gen_label(label_idiv);
    //деление
    fprintf(stdout, "IDIV ");
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");
    gen_convert_result(instr);
    gen_label(label_end);
}
void gen_same_operand_check(TacInstruction *instr){
    
    // Проверка на операнды nil и bool
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@bool\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@bool\n");

    // Дополнительная проверка для конкатенации
    if (instr->operation_code == OP_CONCAT) {
        fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@int\n");
        fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@int\n");
        fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@float\n");
        fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@float\n");
        fprintf(stdout, "MOVE GF@$tmp_op_1 ");
        gen_operand(instr->arg1);
        fprintf(stdout, "\n");
        fprintf(stdout, "MOVE GF@$tmp_op_2 ");
        gen_operand(instr->arg2);
        fprintf(stdout, "\n");
        return;
    }

    char *label_start_operation = create_unique_label("START_OPERATION");
    char *label_arg_2_float= create_unique_label("ARG2_TO_FLOAT");

    // Общая проверка для остальных операций
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@string\n");
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_1 GF@$tmp_type_2\n", label_start_operation);

    // конвертация в float
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_2 string@int\n", label_arg_2_float);

    fprintf(stdout, "INT2FLOAT GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");

    gen_label(label_arg_2_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");

    gen_label(label_start_operation);
}
void gen_convert_result(TacInstruction *instr){
    char *label_end_convert = create_unique_label("END_CONVERT");
    // Конвертация результата обратно в int, если нужно
    fprintf(stdout, "ISINT GF@$tmp ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp bool@false\n", label_end_convert);
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_1 string@int\n", label_end_convert);
    fprintf(stdout, "FLOAT2INT ");
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");

    gen_label(label_end_convert);

}
void gen_arithmetic(TacInstruction *instr){
    gen_type_check(instr);
    gen_same_operand_check(instr);
    // Проверка типов

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
        case OP_CONCAT:
            fprintf(stdout, "CONCAT ");
            break;
        // Добавьте другие арифметические операции по мере необходимости
        default:
            break;
    }
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");
    if (instr->operation_code != OP_CONCAT) {
        gen_convert_result(instr);
    }
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
    if (src->type == OPERAND_TYPE_LABEL)
        fprintf(stdout, "TF@ret");
    else
        gen_operand(src);
    fprintf(stdout, "\n");

}

void gen_param(TACDLList *instructions){
    gen_create_frame();

    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    char *label_name = instr->arg1->data.label_name;

    TACDLList instrs_fun = *instructions;
    TacInstruction *instr_param;
    TACDLL_First(&instrs_fun);
    while (TACDLL_IsActive(&instrs_fun)){
        TACDLL_GetValue(&instrs_fun, &instr_param);
        TACDLL_Next(&instrs_fun);    
        if (instr_param->operation_code == OP_LABEL &&
            strcmp(instr_param->arg1->data.label_name, label_name) == 0)
            break;
    }

    TACDLL_Next(&instrs_fun);    
    TACDLL_Next(instructions);
    while (TACDLL_IsActive(instructions)) {
        TACDLL_GetValue(instructions, &instr);
        TACDLL_GetValue(&instrs_fun, &instr_param);

        if (instr->operation_code != OP_PARAM){
            TACDLL_Previous(instructions);
            break;
        }

        char *param_name = instr_param->result->data.symbol_entry->data->unique_name;
        fprintf(stdout, "DEFVAR TF@%s\n", param_name);
        fprintf(stdout, "MOVE TF@%s ", param_name);
        gen_operand(instr->arg1);
        fprintf(stdout, "\n");

        TACDLL_Next(instructions);
        TACDLL_Next(&instrs_fun);
    }
}

void gen_mul_str(TacInstruction *instr){
    gen_type_check(instr);
    // Проверка типов
    fprintf(stdout, "JUMPIFNEQ $EXIT26 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@bool\n");
    // Проверка на целое число
    fprintf(stdout, "ISINT GF@$tmp ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp bool@false\n");
    
    char *label_loop = create_unique_label("MUL_STR_LOOP");
    char *label_loop_end = create_unique_label("MUL_STR_LOOP_END");
    // Перевод на инт
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_2 string@int\n", label_loop);
    fprintf(stdout, "FLOAT2INT GF@$tmp ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    // Проверка на неотрицательность
    fprintf(stdout, "LT GF@$tmp_2 GF@$tmp int@0\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_2 bool@true\n");
    // Инициализация счетчика и результата
    fprintf(stdout, "MOVE ");
    gen_operand(instr->result);
    fprintf(stdout, " string@\n");
    // Начало цикла
    gen_label(label_loop);
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp int@0\n", label_loop_end);
    // Конкатенация строки
    fprintf(stdout, "CONCAT ");
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    // Декремент счетчика
    fprintf(stdout, "SUB GF@$tmp GF@$tmp int@1\n");
    // Переход к началу цикла
    fprintf(stdout, "JUMP %s\n", label_loop);
    // Конец цикла
    gen_label(label_loop_end);
}

void gen_comprasion(TacInstruction *instr){
    gen_type_check(instr);
    // Проверка типов
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@nil\n");
    char *label_start_operation = create_unique_label("START_OPERATION");
    char *label_arg_2_float= create_unique_label("ARG2_TO_FLOAT");
    fprintf(stdout, "MOVE GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "MOVE GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 GF@$tmp_type_2\n", label_start_operation);
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_1 string@bool\n");
    fprintf(stdout, "JUMPIFEQ $EXIT26 GF@$tmp_type_2 string@bool\n");

     // конвертация в float
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_2 string@int\n", label_arg_2_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");

    gen_label(label_arg_2_float);
    fprintf(stdout, "INT2FLOAT GF@$tmp_op_2 ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");

    
    gen_label(label_start_operation);
    fprintf(stdout, "JUMPIFNEQ $EXIT26 GF@$tmp_type_1 GF@$tmp_type_2\n");
    switch (instr->operation_code) {
        case OP_LESS:
        case OP_LESS_EQUAL:
            fprintf(stdout, "LT ");
            break;
        case OP_GREATER:
        case OP_GREATER_EQUAL:
            fprintf(stdout, "GT ");
            break;
        case OP_EQUAL:
        case OP_NOT_EQUAL:
            fprintf(stdout, "EQ ");
            break;
        // Добавьте другие операции сравнения по мере необходимости
        default:
            break;
    }
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_op_1 GF@$tmp_op_2\n");
}
void gen_eq_comprasion(TacInstruction *instr){
    // Проверка на REATER и LESS
    gen_comprasion(instr);
    // Проверка на EQUAL
    fprintf(stdout, "EQ GF@$tmp ");
    gen_operand(instr->arg1);
    fprintf(stdout, " ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    // OR результата с предыдущим
    fprintf(stdout, "OR ");
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");
}
void gen_not_equal(TacInstruction *instr){
    // Проверка на EQUAL
    gen_comprasion(instr);
    // NOT результата
    fprintf(stdout, "NOT ");
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->result);
    fprintf(stdout, "\n");
}
void gen_declare(Operand *result){
    fprintf(stdout, "MOVE ");
    gen_operand(result);
    fprintf(stdout, " nil@nil\n");
}

void gen_function_begin(TACDLList  *instructions){
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    if (strcmp(instr->arg1->data.label_name, "main@0") == 0) {
        fprintf(stdout, "LABEL $$main\n");        
    } else
        gen_label(instr->arg1->data.label_name);
    gen_push_frame();
    
    fprintf(stdout, "DEFVAR LF@ret\n");
    fprintf(stdout, "MOVE LF@ret nil@nil\n");
    
    TACDLL_Next(instructions);
    while (TACDLL_IsActive(instructions)){
        TACDLL_GetValue(instructions, &instr);
        TacOperationCode op_code = instr->operation_code;
        if (op_code == OP_FUNCTION_END) {
            break;
        } else if (op_code == OP_DECLARE) {
            // Разкоментить при тестировке с готовой таблицей символов
            // if (instr->result->data.symbol_entry->data->nesting_level != 0)
                gen_defvar(instr->result);
        }
        TACDLL_Next(instructions);
    }
}

void gen_label_from_instr(TACDLList  *instructions){
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    TACDLList instructions_copy = *instructions;
    TACDLL_Next(&instructions_copy);
    if (TACDLL_IsActive(&instructions_copy)) {
        TACDLL_GetValue(&instructions_copy, &instr);
    }
    if (instr->operation_code == OP_FUNCTION_BEGIN || instr->operation_code == OP_PARAM) {
        TACDLL_Previous(&instructions_copy);
        gen_function_begin(&instructions_copy);
    } else {
        TACDLL_GetValue(instructions, &instr);
        gen_label(instr->arg1->data.label_name);
    }
}

void gen_is(TacInstruction *instr){
    char *type = instr->arg2->data.constant.value.str_value;
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "EQ ");
    gen_operand(instr->result);
    fprintf(stdout, " GF@$tmp_type_1 ");
    if (strcmp(type, "String") == 0)
        fprintf(stdout, "string@string\n");
    else if (strcmp(type, "NULL") == 0)
        fprintf(stdout, "string@nil\n");
    else if (strcmp(type, "NUM") == 0){
        fprintf(stdout, "string@int\n");
        fprintf(stdout, "EQ GF@$tmp_type_1 GF@$tmp_type_1 string@float\n");
        fprintf(stdout, "OR ");
        gen_operand(instr->result);
        fprintf(stdout, " GF@$tmp_type_1 ");
        gen_operand(instr->result);
        fprintf(stdout, "\n");
    }

}
void gen_read_str(){
    fprintf(stdout, "READ GF@ret_ifj_fun string\n");
}
void gen_read_num(TACDLList *instructions){
    // char *label_end = create_unique_label("READ_NUM_END");
    // fprintf(stdout, "READ GF@ret_ifj_fun float\n");
    // fprintf(stdout, "ISINT GF@$tmp ");
    // gen_operand(instr->arg1);
    // fprintf(stdout, "\n");
    // fprintf(stdout, "JUMPIFEQ %s GF@$tmp bool@false\n", label_end);
    // fprintf(stdout, "FLOAT2INT ");
    // gen_operand(instr->arg1);
    // fprintf(stdout, " ");
    // gen_operand(instr->arg1);
    // fprintf(stdout, "\n");
    // gen_label(label_end);
    void *a = (void*) instructions;
    if (a  == NULL) {
        return;
    }
    return;
}
void gen_write(TACDLList *instructions){
    TACDLL_Next(instructions);
    TacInstruction *instr;
    char *label_write = create_unique_label("WRITE");
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "MOVE GF@$tmp ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE GF@$tmp_type_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_1 string@nil\n", label_write);
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_1 string@string\n", label_write);
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_1 string@bool\n", label_write);

    fprintf(stdout, "ISINT GF@$tmp_type_1 ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_1 bool@false\n", label_write);
    fprintf(stdout, "INT2FLOAT GF@$tmp ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
    gen_label(label_write);
    fprintf(stdout, "WRITE GF@$tmp\n");
}
void gen_floor(TACDLList *instructions){
    TacInstruction *instr;
    TACDLL_Next(instructions);
    TACDLL_GetValue(instructions, &instr);
    fprintf(stdout, "FLOOR ");
    gen_operand(instr->result);
    fprintf(stdout, " ");
    gen_operand(instr->arg1);
    fprintf(stdout, "\n");
}
void gen_ifj_fun(TACDLList *instructions){
    TacInstruction *instr;
    TACDLL_GetValue(instructions, &instr);
    if (strcmp(instr->arg1->data.label_name, "Ifj.read_str") == 0) {
        gen_read_str();
    } else if (strcmp(instr->arg1->data.label_name, "Ifj.read_num") == 0) {
        gen_read_num(instructions);
    } else if (strcmp(instr->arg1->data.label_name, "Ifj.write") == 0) {
        gen_write(instructions);
    } else if (strcmp(instr->arg1->data.label_name, "Ifj.floor") == 0) {
        gen_floor(instructions);
    } else if (strcmp(instr->arg1->data.label_name, "Ifj.str") == 0) {
        return;
    } else if (strcmp(instr->arg1->data.label_name, "Ifj.length") == 0) {
        return;
    } else if (strcmp(instr->arg1->data.label_name, "Ifj.substring") == 0) {
        return;
    } else if (strcmp(instr->arg1->data.label_name, "Ifj.strcmp") == 0) {
        return;
    } else if (strcmp(instr->arg1->data.label_name, "Ifj.ord") == 0) {
        return;
    } else if (strcmp(instr->arg1->data.label_name, "Ifj.chr") == 0) {
        return;
    }
}

int generate_code(TACDLList *instructions, Symtable *table) {
    gen_init(table);
    TACDLL_First(instructions);
    while (TACDLL_IsActive(instructions)) {
        TacInstruction *instr;
        TACDLL_GetValue(instructions, &instr);

        fprintf(stdout, "\n# ======================= NEW INSTRUCTION =======================\n#");
        print_single_tac_instruction(instr);

        switch (instr->operation_code) {
        case OP_JUMP:
            gen_jump(instr->arg2->data.label_name);
            break;
        case OP_JUMP_IF_FALSE:
            gen_jumpifeq(instr);
            break;
        case OP_LABEL:
            gen_label_from_instr(instructions);
            break;
        case OP_ADD:
        case OP_SUBTRACT:
        case OP_MULTIPLY:
        case OP_CONCAT:
            gen_arithmetic(instr);
            break;
        case OP_LESS:
        case OP_GREATER:
        case OP_EQUAL:
            gen_comprasion(instr);
            break;
        case OP_LESS_EQUAL:
        case OP_GREATER_EQUAL:
            gen_eq_comprasion(instr);
            break;
        case OP_NOT_EQUAL:
            gen_not_equal(instr);
            break;
        case OP_DIVIDE:
            gen_divide(instr);
            break;
        case OP_MULTIPLY_STRING:
            gen_mul_str(instr);
            break;
        case OP_RETURN:
        case OP_FUNCTION_END:
            gen_return(instr);
            break;
        case OP_CALL:
            if (strstr(instr->arg1->data.label_name, "Ifj.")) {
                gen_ifj_fun(instructions);
            } else {
                gen_param(instructions);
                gen_call(instr->arg1->data.label_name);
            }
            break;
        case OP_ASSIGN:
            gen_move(instr->result, instr->arg1);
            break;
        case OP_DECLARE:
            gen_declare(instr->result);
            break;
        case OP_IS:
            gen_is(instr);
            break;
        case OP_PARAM:
        case OP_FUNCTION_BEGIN:
        default:
            break;
        }
        TACDLL_Next(instructions);
    }
    return 0;
}
