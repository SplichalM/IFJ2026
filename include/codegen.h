#ifndef CODEGEN_EXPR_H
#define CODEGEN_EXPR_H

#include "ast.h"

void gen_expr(ASTNode *node);

void gen_builtin_helpers(void);

#endif