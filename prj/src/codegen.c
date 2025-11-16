// Имплементация функций генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//
#include "codegen.h"
#include <stdlib.h>

// * Поговорив с Gemini я понял, что мне не нужно реализовывать tzb потому-что я не работаю с регистрами
void DLL_GetInstr(DLList *list, TacInstruction **instr){
    void *data_ptr;
    DLL_GetValue(list, &data_ptr);
    *instr = (TacInstruction*)data_ptr;
}

void gen_init(Symtable *table){
    fprintf(stdout, ".IFJcode25\n");
    fprintf(stdout, "DEFVAR GF@$tmp_2\n");
    fprintf(stdout, "DEFVAR GF@$tmp\n");
    fprintf(stdout, "DEFVAR GF@$tmp_type_1\n");
    fprintf(stdout, "DEFVAR GF@$tmp_type_2\n");
    fprintf(stdout, "DEFVAR GF@$tmp_op_1\n");
    fprintf(stdout, "DEFVAR GF@$tmp_op_2\n");
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
void gen_jump(char* label_name){
    fprintf(stdout, "JUMP $%s\n", label_name);
}
void gen_call(char* label_name){
    fprintf(stdout, "CALL $%s\n", label_name);
}
void gen_return(TacInstruction *instr){
    if (instr->operation_code == OP_FUNCTION_END &&
    strcmp(instr->arg1->data.label_name, "main") == 0) {
        fprintf(stdout, "JUMP $END\n");
        return;
    }
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
        int nesting_level = op->data.symbol_entry->data->nesting_level;
        if (nesting_level == 0)
            fprintf(stdout, "GF@");
        else
            fprintf(stdout, "LF@");
        fprintf(stdout, "%s$%d", op->data.symbol_entry->key, nesting_level);
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
    gen_type_check(instr);
    char *label_div = create_unique_label("$DIV");
    char *label_idiv = create_unique_label("$IDIV");
    char *label_end = create_unique_label("$END_DIV");
    // Проверка типов
    fprintf(stdout, "JUMPIFNEQ $EXIT53 GF@$tmp_type_1 GF@$tmp_type_2\n");

    // разделение на деление float и int 
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@float", label_div);
    fprintf(stdout, "JUMPIFEQ $%s GF@$tmp_type_1 string@int", label_idiv);
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
void gen_same_operand_check(TacInstruction *instr){
    
    // Проверка на операнды nil и bool
    fprintf(stdout, "JUMPIFEQ $EXIT54 GF@$tmp_type_1 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT54 GF@$tmp_type_2 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT54 GF@$tmp_type_1 string@bool\n");
    fprintf(stdout, "JUMPIFEQ $EXIT54 GF@$tmp_type_2 string@bool\n");

    // Дополнительная проверка для конкатенации
    if (instr->operation_code == OP_CONCAT) {
        fprintf(stdout, "JUMPIFEQ $EXIT54 GF@$tmp_type_1 string@int\n");
        fprintf(stdout, "JUMPIFEQ $EXIT54 GF@$tmp_type_2 string@int\n");
        fprintf(stdout, "JUMPIFEQ $EXIT54 GF@$tmp_type_1 string@float\n");
        fprintf(stdout, "JUMPIFEQ $EXIT54 GF@$tmp_type_2 string@float\n");
        fprintf(stdout, "MOVE GF@$tmp_op_1 ");
        gen_operand(instr->arg1);
        fprintf(stdout, "\n");
        fprintf(stdout, "MOVE GF@$tmp_op_2 ");
        gen_operand(instr->arg2);
        fprintf(stdout, "\n");
        return;
    }

    char *label_start_operation = create_unique_label("$START_OPERATION");
    char *label_arg_2_float= create_unique_label("$ARG2_TO_FLOAT");

    // Общая проверка для остальных операций
    fprintf(stdout, "JUMPIFNEQ $EXIT54 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFNEQ $EXIT54 GF@$tmp_type_2 string@string\n");
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
    char *label_end_convert = create_unique_label("$END_CONVERT");
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
    // ? Заготовка для расширения EXTSTAT
    // if (src == NULL) {
    // } else
    gen_operand(src);
    fprintf(stdout, "\n");

}

void gen_param(DLList *instructions){
    gen_create_frame();

    TacInstruction *instr;
    DLL_GetInstr(instructions, &instr);
    char *label_name = instr->arg1->data.label_name;

    DLList instrs_fun = *instructions;
    TacInstruction *instr_param;
    DLL_First(&instrs_fun);
    while (DLL_IsActive(&instrs_fun)){
        DLL_GetInstr(&instrs_fun, &instr_param);
        DLL_Next(&instrs_fun);    
        if (instr_param->operation_code == OP_LABEL &&
            strcmp(instr_param->arg1->data.label_name, label_name) == 0)
            break;
    }

    DLL_Next(&instrs_fun);    
    DLL_Next(instructions);
    while (DLL_IsActive(instructions)) {
        DLL_GetInstr(instructions, &instr);
        DLL_GetInstr(&instrs_fun, &instr_param);

        if (instr->operation_code != OP_PARAM){
            DLL_Previous(instructions);
            break;
        }

        char *param_name = instr_param->result->data.symbol_entry->key;
        int param_nesting = instr_param->result->data.symbol_entry->data->nesting_level;
        fprintf(stdout, "DEFVAR TF@%s%i\n", param_name, param_nesting);
        fprintf(stdout, "MOVE TF@%s%i ", param_name, param_nesting);
        gen_operand(instr->arg1);
        fprintf(stdout, "\n");

        DLL_Next(instructions);
        DLL_Next(&instrs_fun);
    }
}
void gen_end(){
    for (int i = 50; i < 59; i++) {
        fprintf(stdout, "LABEL $Exit%i\n", i);
        fprintf(stdout, "EXIT int@%i\n", i); 
        fprintf(stdout, "JUMP $End\n");
    }
    fprintf(stdout, "LABEL $Exit60\n");
    fprintf(stdout, "EXIT int@60\n"); 
    fprintf(stdout, "JUMP $End\n");

    fprintf(stdout, "LABEL $Exit0\n");
    fprintf(stdout, "EXIT int@0\n"); 

    fprintf(stdout, "LABEL $End\n");
}

void gen_mul_str(TacInstruction *instr){
    gen_type_check(instr);
    // Проверка типов
    fprintf(stdout, "JUMPIFNEQ $EXIT53 GF@$tmp_type_1 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT53 GF@$tmp_type_2 string@string\n");
    fprintf(stdout, "JUMPIFEQ $EXIT53 GF@$tmp_type_2 string@nil\n");
    fprintf(stdout, "JUMPIFEQ $EXIT53 GF@$tmp_type_2 string@bool\n");
    // Проверка на целое число
    fprintf(stdout, "ISINT GF@$tmp ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $EXIT57 GF@$tmp bool@false\n");
    
    char *label_loop = create_unique_label("$MUL_STR_LOOP");
    char *label_loop_end = create_unique_label("$MUL_STR_LOOP_END");
    // Перевод на инт
    fprintf(stdout, "JUMPIFEQ %s GF@$tmp_type_2 string@int\n", label_loop);
    fprintf(stdout, "FLOAT2INT GF@$tmp ");
    gen_operand(instr->arg2);
    fprintf(stdout, "\n");
    // Проверка на неотрицательность
    fprintf(stdout, "LT GF@$tmp_2 GF@$tmp int@0\n");
    fprintf(stdout, "JUMPIFEQ $EXIT57 GF@$tmp_2 bool@true\n");
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
    fprintf(stdout, "JUMPIFNEQ $EXIT53 GF@$tmp_type_1 GF@$tmp_type_2\n");
    fprintf(stdout, "JUMPIFEQ $EXIT53 GF@$tmp_type_1 string@nil\n");

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
    gen_tac(instr);
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

void gen_function_begin(DLList  *instructions){
    TacInstruction *instr;
    DLL_GetInstr(instructions, &instr);
    if (strcmp(instr->arg1->data.label_name, "main") == 0) {
        fprintf(stdout, "$$main:\n");        
    } else
        gen_label(instr->arg1->data.label_name);
    gen_push_frame();
    
    fprintf(stdout, "DEFVAR LF@ret\n");
    fprintf(stdout, "MOVE LF@ret nil@nil\n");
    
    DLL_Next(instructions);
    while (DLL_IsActive(instructions)){
        DLL_GetInstr(instructions, &instr);
        TacOperationCode op_code = instr->operation_code;
        if (op_code == OP_FUNCTION_END) {
            break;
        } else if (op_code == OP_DECLARE) {
            // Разкоментить при тестировке с готовой таблицей символов
            // if (instr->result->data.symbol_entry->data->nesting_level != 0)
                gen_defvar(instr->result);
        }
        DLL_Next(instructions);
    }
}

void gen_label_from_instr(DLList  *instructions){
    TacInstruction *instr;
    DLL_GetInstr(instructions, &instr);
    DLList instructions_copy = *instructions;
    DLL_Next(&instructions_copy);
    if (DLL_IsActive(&instructions_copy)) {
        DLL_GetInstr(&instructions_copy, &instr);
    }
    if (instr->operation_code == OP_FUNCTION_BEGIN || instr->operation_code == OP_PARAM) {
        DLL_Previous(&instructions_copy);
        gen_function_begin(&instructions_copy);
    } else {
        DLL_GetInstr(instructions, &instr);
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

int generate_code(DLList *instructions, Symtable *table) {
    gen_init(table);
    DLL_First(instructions);
    while (DLL_IsActive(instructions)) {
        TacInstruction *instr;
        DLL_GetInstr(instructions, &instr);
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
            gen_param(instructions);
            gen_call(instr->arg1->data.label_name);
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
        DLL_Next(instructions);
    }
    gen_end();
    return 0;
}
