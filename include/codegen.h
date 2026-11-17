#ifndef CODEGEN_EXPR_H
#define CODEGEN_EXPR_H
#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

void gen_expr(ASTNode *node);
unsigned int gen_new_label();

void gen_builtin_helpers(void);
void gen_program(ASTNode* tree);

#endif
