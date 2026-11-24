// Autor: Jan Pospíšil (xpospij01)
#ifndef PARSER_H
#define PARSER_H
#include "ast.h"
#include "symtable.h"

ASTNode *parse_program(
    symtable_t *functions,
    void (*register_function)(symtable_t *, ASTNode *)
);

#endif