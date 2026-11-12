#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symtable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct sem_scope_t {
	symtable_t vars;
	struct sem_scope_t* next;
} sem_scope_t;

typedef struct sem_context_t {
	symtable_t* functions;
	sem_scope_t* top;
	sym_data_t* curr_func;
	unsigned int count;
} sem_context_t;

int sem_analyze(ASTNode* tree, symtable_t* functions);

void sem_error(int err_code, const char* err_msg);

void sem_analyze_function(symtable_t* functions, ASTNode* func_def);
DataType sem_identifier(sem_context_t* context, ASTNode* node);

DataType sem_call(sem_context_t* context, ASTNode* node);

void sem_register_builtins(symtable_t* symtable);

#endif