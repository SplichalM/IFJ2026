#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include "ast.h"

typedef enum {
	SYM_LOCAL_VAR,
	SYM_FUNCTION
} sym_type_t;

typedef enum {
	FUNC_NOT_ANALYZED,
	FUNC_IN_PROGRESS,
	FUNC_DONE
} func_state_t;

typedef struct var_data {
	DataType var_type;
	bool is_mutable;
	char* unique_name;
} var_data_t;

typedef struct func_data {
	DataType return_type;
	int param_count;
	DataType* param_types;
	func_state_t state;
	bool is_main;
	bool is_builtin;
	ASTNode* definition;
	bool expl_return_type;
} func_data_t;

typedef struct sym_data {
	sym_type_t type;
	union {
		var_data_t var;
		func_data_t func;
	} as;
} sym_data_t;

typedef struct symtable_node {
	char* key;
	int height;
	sym_data_t data;
	struct symtable_node* left;
	struct symtable_node* right;
} symtable_node_t;

typedef struct {
	symtable_node_t* root;
} symtable_t;

void symtable_init(symtable_t* symtable);
sym_data_t* symtable_insert(symtable_t* symtable, const char* key, sym_type_t type);
// void symtable_print(symtable_t* symtable);
void symtable_dispose(symtable_t* symtable);
sym_data_t* symtable_search(symtable_t* symtable, const char* key);

#endif
