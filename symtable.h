#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>

typedef enum {
	SYM_LOCAL_VAR,
	SYM_FUNCTION
} sym_type_t;

typedef struct symtable_node {
	char* key;
	int height;
	int param_count;
	bool defined;
	struct symtable_node* left;
	struct symtable_node* right;
	sym_type_t type;
} symtable_node_t;

typedef struct {
	symtable_node_t* root;
} symtable_t;

void symtable_init(symtable_t* symtable);
bool symtable_insert(symtable_t* symtable, const char* key, sym_type_t type, int param_count);
void symtable_print(symtable_t* symtable);
void symtable_dispose(symtable_t* symtable);
symtable_node_t* symtable_search(symtable_t* symtable, const char* key);

#endif // !SYMTABLE_H
