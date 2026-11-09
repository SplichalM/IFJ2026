#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symtable.h"
#include <stdlib.h>
#include <stdio.h>

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

static void scope_push(sem_context_t* context);
static void scope_pop(sem_context_t* context);
static sym_data_t* scope_lookup(sem_context_t* context, const char* key);
static sym_data_t* scope_declare(sem_context_t* context, const char* key, DataType type, bool is_mutable);

static char* sem_unique_name(sem_context_t* context, const char* name);
static void sem_check_main(ASTNode* tree);
void sem_analyze_function(symtable_t* functions, ASTNode* func_def);
static void sem_block(sem_context_t* context, ASTNode* block);
static void sem_statement(sem_context_t* context, ASTNode* current);
DataType sem_identifier(sem_context_t* context, ASTNode* node);
static void sem_var_decl(sem_context_t* context, ASTNode* node);
static void sem_assign(sem_context_t* context, ASTNode* node);
static void sem_if(sem_context_t* context, ASTNode* node);
static void sem_while(sem_context_t* context, ASTNode* node);

static void sem_add_builtin(symtable_t* symtable, const char* name, DataType return_type, int param_count, const DataType* param_types);
DataType sem_call(sem_context_t* context, ASTNode* node);

#endif