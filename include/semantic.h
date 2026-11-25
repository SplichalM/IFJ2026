#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symtable.h"


int sem_analyze(ASTNode* tree, symtable_t* functions);
void sem_register_builtins(symtable_t* symtable);
void register_user_function(symtable_t* functions, ASTNode* definition);

#endif