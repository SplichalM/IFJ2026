#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

unsigned int gen_new_label();

void gen_program(ASTNode* tree);

#endif
