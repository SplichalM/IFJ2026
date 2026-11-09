#include "symtable.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int max(int a, int b) {
	return a > b ? a : b;
}

int get_height(symtable_node_t* node) {
	if (node == NULL) return 0;
	return node->height;
}

symtable_node_t* right_rotate(symtable_node_t* y) {
	symtable_node_t* x = y->left;
	symtable_node_t* T2 = x->right;

	x->right = y;
	y->left = T2;

	y->height = max(get_height(y->left), get_height(y->right)) + 1;
	x->height = max(get_height(x->left), get_height(x->right)) + 1;

	return x;
}

symtable_node_t* left_rotate(symtable_node_t* x) {
	symtable_node_t* y = x->right;
	symtable_node_t* T2 = y->left;

	y->left = x;
	x->right = T2;

	x->height = max(get_height(x->left), get_height(x->right)) + 1;
	y->height = max(get_height(y->left), get_height(y->right)) + 1;

	return y;
}

symtable_node_t* new_node(const char* key, sym_type_t type) {
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

symtable_node_t* insert_rec(symtable_node_t* root, const char* key, sym_type_t type, sym_data_t** out) {
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

void symtable_init(symtable_t* symtable) {
	symtable->root = NULL;
}

sym_data_t* symtable_insert(symtable_t* symtable, const char* key, sym_type_t type) {
	if (symtable == NULL || key == NULL) return NULL;
	sym_data_t* result = NULL;
	symtable->root = insert_rec(symtable->root, key, type, &result);
	return result;
}

sym_data_t* search_rec(symtable_node_t* root, const char* key) {
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
/*
static void symtable_print_rec (symtable_node_t *node){
    if (node == NULL) return;
    symtable_print_rec (node->left);
    
    printf("%s (%s, params=%d, defined=%s)\n",
           node->key,
           node->type == SYM_FUNC ? "FUNC" : "VAR",
           node->param_count,
           node->defined ? "yes" : "no");

    symtable_print_rec(node->right);
}

void symtable_print(symtable_t* symtable) {
    if (symtable == NULL || symtable->root == NULL) {
        printf("(empty symtable)\n");
        return;
    }
    symtable_print_rec(symtable->root);
}
*/
void dispose_node(symtable_node_t  *node){
    if (node == NULL) return;
    dispose_node (node->left);
    dispose_node (node->right);

	if (node->data.type == SYM_LOCAL_VAR) {
		free(node->data.as.var.unique_name);
	}
	if (node->data.type == SYM_FUNCTION) {
		free(node->data.as.func.param_types);
	}
    free(node->key);
    free(node);
}

void symtable_dispose (symtable_t *symtable){
    if (symtable == NULL) return;
    dispose_node (symtable->root);
    symtable->root = NULL;
}