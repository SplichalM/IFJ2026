/**
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 *
 * Soubor: symtable.h
 * Popis: Tabulka symbolů implementovaná jako vv-BVS
 *
 * Autoři:
 * Tomáš Schablický (xschabt00)
 * Azíz Hammadi ()
 */

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

typedef struct symtable_node symtable_node_t;

typedef struct {
	symtable_node_t* root;
} symtable_t;

/*
Nastaví kořen stromu na NULL.
*/
void symtable_init(symtable_t* symtable);

/*
Zkontroluje existenci symtablu a rekurzivně vloží prvek.
Vrací adresu struktury pro doplnění dat.
Pokud prvek již existuje nebo je argument NULL, tak vrací NULL.
*/
sym_data_t* symtable_insert(symtable_t* symtable, const char* key, sym_type_t type);

/*
Rekurzivně uvolní pamět využitou symtablem.
*/
void symtable_dispose(symtable_t* symtable);

/*
Zkontroluje existenci symtablu a rekurzivně vyhledá prvek podle jména.
Vrací adresu na strukturu dat.
Pokud prvek neexistuje, vrací NULL.
*/
sym_data_t* symtable_search(symtable_t* symtable, const char* key);

#endif
