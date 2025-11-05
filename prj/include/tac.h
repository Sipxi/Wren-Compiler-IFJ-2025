/**
 * @file tac_gen.h
 * @brief Defines the structures for Three-Address Code (3AC) generation.
 *
 * This module translates the Abstract Syntax Tree (AST) into a
 * list of generic "quadruple" instructions, which are stored
 * in a Doubly-Linked List (DLL).
 */

#ifndef TAC_GEN_H
#define TAC_GEN_H

// --- 1. Your Project's Files ---

// We must include your other modules to link them together
#include "symtable.h" // Your symtable (provides TableEntry*)
// #include "ast.h"      // Your AST definitions (provides AstNode*)
#include "common.h"      // Your Doubly-Linked List (provides Dll*)


// --- 2. Structures for Operands (The "Nouns") ---

/**
 * @brief Holds a literal constant value (e.g., 10, "hello", nil).
 *
 * WHY: Your symtable stores *symbols* (like variable 'a').
 * It doesn't store *literal values* (like the number 10).
 * We need this struct to hold these values.
 */
typedef struct {
    DataType type; // Using your DataType enum (TYPE_INT, etc.)
    union {
        // Add fields to match your language's types
        int int_val;       // For TYPE_INT
        double num_val;    // For a 'Num' type
        char* string_val;  // For a 'String' type
        // bool bool_val;
    } value;
} TacConstant;

/**
 * @brief Defines the *kind* of noun (operand) we are using.
 *
 * WHY: This is the "tag" for the union in 'struct Operand'.
 * It tells our code: "Is this a variable? A constant? A label?"
 */
typedef enum {
    OPERAND_EMPTY,  // Represents a NULL or unused operand
    OPERAND_SYMBOL, // A variable/temp (points to a TableEntry*)
    OPERAND_CONST,  // A literal (points to a TacConstant*)
    OPERAND_VAR,   // A function parameter (points to a TableEntry*)

    OPERAND_LABEL   // A jump target (points to a char* name)
} OperandType;

/**
 * @brief Represents a single operand (a "noun") in an instruction.
 *
 * WHY: This is the "smart pointer" we discussed. Instead of just
 * a 'char*', this struct tells us *exactly* what the operand is
 * and holds all the info we need for it.
 */
typedef struct Operand {
    OperandType type; // The "tag" telling us what's in the union
    
    union {
        // A pointer to the symbol's entry in your symtable
        TableEntry* entry; 
        
        // A pointer to a struct holding a literal value
        TacConstant* constant;

        // A simple string for the label's name (e.g., "L_ELSE_1")
        char* label_name;
        
    } value;

} Operand;


// --- 3. Structures for Opcodes & Instructions (The "Verbs") ---

/**
 * @brief Defines all possible *generic* operations (the "verbs").
 *
 * WHY: This is our abstract, intermediate instruction set.
 * It is NOT IFJcode25. It's a simple, generic list of
 * actions that we will translate to IFJcode25 in the *next* phase.
 */
typedef enum {
    // Meta / Jumps
    OP_LABEL,     // L1:
    OP_JUMP,      // goto L1
    OP_JUMP_IF_FALSE, // if_false (t1) goto L1
    
    // Data & Arithmetic
    OP_ASSIGN,    // result = arg1
    OP_ADD,       // result = arg1 + arg2
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_GT,        // result = arg1 > arg2
    OP_LT,
    OP_EQ,
    OP_IDIV,

    // ... add all other ops you need ...
    
    // Functions
    OP_FUNC_BEGIN, // Marks function start
    OP_FUNC_END,   // ?Marks function end
    OP_CALL,       // result = call arg1
    OP_PARAM, // Push arg1
    OP_RETURN      // return arg1
    
} TacOpCode;

/**
 * @brief The final 3AC Instruction (a "Quadruple").
 *
 * WHY: This struct combines the "verb" (TacOpCode) with the
 * "nouns" (Operands) to form a complete instruction.
 * THIS IS WHAT YOU WILL STORE IN YOUR DLL.
 */
typedef struct Quadruple {
    TacOpCode op;     // The action (e.g., OP_ADD)
    
    Operand* result;  // Destination (e.g., t1)
    Operand* arg1;    // Source 1 (e.g., a)
    Operand* arg2;    // Source 2 (e.g., 10)
} Quadruple;


// --- 4. The Main Instruction List ---

/**
 * @brief We re-name your DLL to be our "InstructionList".
 *
 * WHY: This is just a 'typedef' for clarity. Your generator's
 * job is to create a 'DLList*' and fill it with 'Quadruple*' data.
 */
typedef DLList InstructionList;


// --- 5. Main Function (The Public API) ---

/**
 * @brief The main function for this module.
 * Walks the AST and generates a list of 3AC instructions.
 *
 * @param ast_root The root node of the (semantically valid) AST.
 * @param global_scope The global symbol table.
 * @return A pointer to a new InstructionList (your Dll*),
 * or NULL on failure.
 */
// !InstructionList* generate_tac(AstNode* ast_root, Symtable* global_scope);

/**
 * @brief Frees the entire instruction list and all its contents.
 *
 * @param list The instruction list to free.
 */
void free_tac_list(InstructionList* list);


#endif // TAC_GEN_H