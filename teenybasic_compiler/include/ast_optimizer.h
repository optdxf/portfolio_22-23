#ifndef AST_OPT_H
#define AST_OPT_H

#include "ast.h"

// Given a valid AST, optimizes the AST (i.e. modifies it)
// Nodes that are removed are freed accordingly
// Behavior is undefined if the AST is not valid
node_t *optimize_ast(node_t *ast);

#endif