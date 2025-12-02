/**
 * @file ast.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Implementace abstraktního syntaktického stromu (AST)
 *
 * @author
 *     - Mykhailo Tarnavskyi (272479)
 */

#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * Vytváří se nový uzel AST.
 * @param type Typ vytvářeného uzlu.
 * @param line_number Číslo řádku pro ladění.
 * @return Ukazatel na nový uzel.
 */
AstNode *ast_node_create(NodeType type, int line_number)
{
    AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
    if (node == NULL)
    {
        return NULL;
    }

    node->type = type;
    node->line_number = line_number;

    return node;
}

/**
 * Přidává se potomek k rodičovskému uzlu.
 * Pokud rodič nemá žádné potomky, stává se nový uzel prvním potomkem (child).
 * Pokud již potomky má, je nový uzel přidán na konec seznamu sourozenců (sibling).
 * @param parent Rodičovský uzel.
 * @param new_child Přidávaný potomek.
 */
void ast_node_add_child(AstNode *parent, AstNode *new_child)
{
    if (parent == NULL || new_child == NULL)
    {
        return;
    }

    if (parent->child == NULL)
    {
        parent->child = new_child;
    }
    else
    {
        AstNode *current_child = parent->child;
        while (current_child->sibling != NULL)
        {
            current_child = current_child->sibling;
        }
        current_child->sibling = new_child;
    }
}

/**
 * Rekurzivně se uvolňuje paměť celého podstromu.
 * Uvolňuje se uzel, jeho potomci i sourozenci. Zároveň se uvolňují alokovaná data uvnitř uzlů (identifikátory, řetězce).
 * @param node Kořen podstromu k uvolnění.
 */
void ast_node_free_recursive(AstNode *node)
{
    if (node == NULL)
    {
        return;
    }

    ast_node_free_recursive(node->child);
    ast_node_free_recursive(node->sibling);
    switch (node->type)
    {
    case NODE_ID:
    case NODE_VAR_DEF:
    case NODE_PARAM:
    case NODE_FUNCTION_DEF:
    case NODE_SETTER_DEF:
    case NODE_GETTER_DEF:
    case NODE_TYPE_NAME:
        free(node->data.identifier);
        break;
    case NODE_LITERAL_STRING:
        free(node->data.literal_string);
        break;
    default:
        break;
    }

    free(node);
}


AstNode *ast_new_id_node(NodeType type, int line, const char *id) {
    AstNode *node = ast_node_create(type, line);
    if (node == NULL) return NULL; 

    node->data.identifier = strdup_c99(id);
    if (node->data.identifier == NULL && id != NULL) {
        free(node);
        return NULL;
    }

    return node;
}


AstNode *ast_new_num_node(double value, int line) {
    AstNode *node = ast_node_create(NODE_LITERAL_NUM, line);
    if (node == NULL) return NULL;

    node->data.literal_num = value;
    return node;
}


AstNode *ast_new_string_node(const char *value, int line) {
    AstNode *node = ast_node_create(NODE_LITERAL_STRING, line);
    if (node == NULL) return NULL;

    node->data.literal_string = strdup_c99(value);
    if (node->data.literal_string == NULL && value != NULL) {
        free(node);
        return NULL;
    }

    return node;
}


AstNode *ast_new_null_node(int line) {
    AstNode *node = ast_node_create(NODE_LITERAL_NULL, line);
    return node;
}


AstNode *ast_new_bin_op(NodeType type, int line, AstNode *left, AstNode *right) {
    AstNode *node = ast_node_create(type, line);
    if (node == NULL) return NULL;

    ast_node_add_child(node, left);
    ast_node_add_child(node, right);

    return node;
}