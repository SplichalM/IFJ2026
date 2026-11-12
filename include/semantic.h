#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symtable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#define SEM_BLOCK_RETURNS ((DataType)-1)

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
void sem_register_builtins(symtable_t* symtable);
void sem_error(int err_code, const char* err_msg);


void sem_analyze_function(symtable_t* functions, ASTNode* func_def);
DataType sem_identifier(sem_context_t* context, ASTNode* node);
DataType sem_call(sem_context_t* context, ASTNode* node);


DataType sem_expr_type(sem_context_t* context, ASTNode* node);
DataType sem_binary_op(sem_context_t* context, ASTNode* node);
DataType sem_unary_op(sem_context_t* context, ASTNode* node);
void sem_insert_conversion(ASTNode** subnode_ptr);
bool sem_types_compatible(DataType target, DataType source);
void sem_check_assignable(sem_context_t* context, DataType target, ASTNode** expr_ptr);
void sem_check_condition(sem_context_t* context, ASTNode* cond);

void sem_return(sem_context_t* context, ASTNode* node);
DataType sem_block_result(sem_context_t* context, ASTNode* block);
void sem_check_function_result(sem_context_t* context, sym_data_t* func);
void sem_ensure_function_analyzed(symtable_t* functions, sym_data_t* func);

#endif