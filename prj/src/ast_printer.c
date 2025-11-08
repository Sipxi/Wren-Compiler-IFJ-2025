#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

static const char* node_type_to_string(NodeType type)
{
    switch (type) {
        case NODE_PROGRAM: return "NODE_PROGRAM";
        case NODE_FUNCTION_DEF: return "NODE_FUNCTION_DEF";
        case NODE_SETTER_DEF: return "NODE_SETTER_DEF";
        case NODE_GETTER_DEF: return "NODE_GETTER_DEF";
        case NODE_PARAM_LIST: return "NODE_PARAM_LIST";
        case NODE_PARAM: return "NODE_PARAM";
        case NODE_BLOCK: return "NODE_BLOCK";
        case NODE_IF: return "NODE_IF";
        case NODE_WHILE: return "NODE_WHILE";
        case NODE_VAR_DEF: return "NODE_VAR_DEF";
        case NODE_RETURN: return "NODE_RETURN";
        case NODE_ASSIGNMENT: return "NODE_ASSIGNMENT";
        case NODE_CALL_STATEMENT: return "NODE_CALL_STATEMENT";
        case NODE_ARGUMENT_LIST: return "NODE_ARGUMENT_LIST";
        case NODE_OP_PLUS: return "NODE_OP_PLUS (+)";
        case NODE_OP_MINUS: return "NODE_OP_MINUS (-)";
        case NODE_OP_MUL: return "NODE_OP_MUL (*)";
        case NODE_OP_DIV: return "NODE_OP_DIV (/)";
        case NODE_OP_LT: return "NODE_OP_LT (<)";
        case NODE_OP_GT: return "NODE_OP_GT (>)";
        case NODE_OP_LTE: return "NODE_OP_LTE (<=)";
        case NODE_OP_GTE: return "NODE_OP_GTE (>=)";
        case NODE_OP_EQ: return "NODE_OP_EQ (==)";
        case NODE_OP_NEQ: return "NODE_OP_NEQ (!=)";
        case NODE_OP_IS: return "NODE_OP_IS (is)";
        case NODE_ID: return "NODE_ID";
        case NODE_LITERAL_NUM: return "NODE_LITERAL_NUM";
        case NODE_LITERAL_STRING: return "NODE_LITERAL_STRING";
        case NODE_LITERAL_NULL: return "NODE_LITERAL_NULL";
        case NODE_TYPE_NAME: return "NODE_TYPE_NAME";
        default: return "UNKNOWN_NODE";
    }
}


static void ast_print_recursive(AstNode* node, int indent_level) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < indent_level; ++i) {
        printf("  ");
    }
    printf("%s (line %d)\n", node_type_to_string(node->type), node->line_number);
    AstNode* child = node->child;
    while (child != NULL) {
        ast_print_recursive(child, indent_level + 1);
        child = child->sibling;
    }
}


static void ast_print_debug(AstNode* node)
{
    printf("--- [ AST DEBUG PRINT ] ---\n");
    ast_print_recursive(node, 0);
    printf("---------------------------\n");
}
