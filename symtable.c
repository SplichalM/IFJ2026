/**
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 *
 * Soubor: symtable.c
 * Popis: Tabulka symbolů implementovaná jako vv-BVS
 *
 * Autoři:
 * Tomáš Schablický (xschabt00)
 * Azíz Hammadi (xhammaa00)
 */

#include "symtable.h"
#include <string.h>
#include <stdlib.h>


struct symtable_node {
	char* key;
	int height;
	sym_data_t data;
	struct symtable_node* left;
	struct symtable_node* right;
};


static void dispose_node(symtable_node_t* node);
static symtable_node_t* new_node(const char* key, sym_type_t type);
static symtable_node_t* left_rotate(symtable_node_t* x);
static symtable_node_t* right_rotate(symtable_node_t* y);
static int get_height(symtable_node_t* node);
static int max(int a, int b);
static symtable_node_t* insert_rec(symtable_node_t* root, const char* key, sym_type_t type, sym_data_t** out);
static sym_data_t* search_rec(symtable_node_t* root, const char* key);


static int max(int a, int b) {
	return a > b ? a : b;
}

static int get_height(symtable_node_t* node) {
	if (node == NULL) return 0;
	return node->height;
}

// Provede rotaci podstromu doprava a vratí jeho nový kořen
static symtable_node_t* right_rotate(symtable_node_t* y) {
	symtable_node_t* x = y->left;
	symtable_node_t* T2 = x->right;

	x->right = y;
	y->left = T2;

	y->height = max(get_height(y->left), get_height(y->right)) + 1;
	x->height = max(get_height(x->left), get_height(x->right)) + 1;

	return x;
}

// Provede rotaci podstromu doleva a vrátí jeho nový kořen
static symtable_node_t* left_rotate(symtable_node_t* x) {
	symtable_node_t* y = x->right;
	symtable_node_t* T2 = y->left;

	y->left = x;
	x->right = T2;

	x->height = max(get_height(x->left), get_height(x->right)) + 1;
	y->height = max(get_height(y->left), get_height(y->right)) + 1;

	return y;
}

// Alokuje paměť pro prvek a jeho jméno a vrátí adresu prvku
// Nastaví jméno z parametru a výšku na 1
static symtable_node_t* new_node(const char* key, sym_type_t type) {
	symtable_node_t* node = calloc(1, sizeof(symtable_node_t));
	if (node == NULL) {
		exit(99);
	}
	
	node->key = malloc(sizeof(char) * (strlen(key) + 1));
	if (node->key == NULL) {
		exit(99);
	}

	strcpy(node->key, key);

	node->data.type = type;
	node->height = 1;

	return node;
}

// Pomocná rekurzivní funkce pro vložení prvku, pak podle výšky podstromů volá pomocné funkce pro rotaci
static symtable_node_t* insert_rec(symtable_node_t* root, const char* key, sym_type_t type, sym_data_t** out) {
	if (root == NULL) {
		symtable_node_t* node = new_node(key, type);
		*out = &node->data;
		return node;
	}
	int cmp = strcmp(key, root->key);
	if (cmp < 0) {
		root->left = insert_rec(root->left, key, type, out);
	}
	else if (cmp > 0) {
		root->right = insert_rec(root->right, key, type, out);
	}
	else {
		*out = NULL;
		return root;
	}

	root->height = 1 + max(get_height(root->left), get_height(root->right));
	int balance_fac = get_height(root->left) - get_height(root->right);

	if (balance_fac > 1 && strcmp(key, root->left->key) < 0) {
		return right_rotate(root);
	}

	if (balance_fac < -1 && strcmp(key, root->right->key) > 0) {
		return left_rotate(root);
	}

	if (balance_fac > 1 && strcmp(key, root->left->key) > 0) {
		root->left = left_rotate(root->left);
		return right_rotate(root);
	}

	if (balance_fac < -1 && strcmp(key, root->right->key) < 0) {
		root->right = right_rotate(root->right);
		return left_rotate(root);
	}

	return root;
}

/* Inicializuje prazdnou tabulku symbolu*/
void symtable_init(symtable_t* symtable) {
	symtable->root = NULL;
}

/* Vlozi novy symbol pod zadanym klicem a vyvazi strom; pri duplicite vraci NULL, jinak ukazatel na data. */
sym_data_t* symtable_insert(symtable_t* symtable, const char* key, sym_type_t type) {
	if (symtable == NULL || key == NULL) return NULL;
	sym_data_t* result = NULL;
	symtable->root = insert_rec(symtable->root, key, type, &result);
	return result;
}

// Pomocná rekurzivní funkce pro vyhledání prvku
static sym_data_t* search_rec(symtable_node_t* root, const char* key) {
	if (root == NULL) return NULL;

	int cmp = strcmp(key, root->key);

	if (cmp == 0) return &root->data;
	else if (cmp < 0) return search_rec(root->left, key);
	else return search_rec(root->right, key);
}

sym_data_t* symtable_search(symtable_t* symtable, const char* key) {
	if (symtable == NULL || key == NULL) return NULL;
	return search_rec(symtable->root, key);
}

// Pomocná rekurzivní funkce pro uvolnění paměti symtablu
static void dispose_node(symtable_node_t* node){
	if (node == NULL) return;
	dispose_node(node->left);
	dispose_node(node->right);

	if (node->data.type == SYM_LOCAL_VAR) {
		free(node->data.as.var.unique_name);
	}
	if (node->data.type == SYM_FUNCTION) {
		free(node->data.as.func.param_types);
	}
	free(node->key);
	free(node);
}

void symtable_dispose (symtable_t* symtable) {
	if (symtable == NULL) return;
	dispose_node(symtable->root);
	symtable->root = NULL;
}