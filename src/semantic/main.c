#include "semantic.h"

int sem_analyze(ASTNode* tree, symtable_t* functions) {
	return 0;
}

static void scope_push(sem_context_t* context) {
	sem_scope_t* scope = malloc(sizeof(sem_scope_t));
	if (scope == NULL) {
		sem_error(99, "Error allocating memory for scope stack (semantic)");
	}

	symtable_init(&scope->vars);
	scope->next = context->top;
	context->top = scope;
}

static void scope_pop(sem_context_t* context) {
	if (context->top == NULL) {
		sem_error(99, "Could not pop top of stack (semantic)");
	}
	sem_scope_t* old = context->top;
	context->top = old->next;
	symtable_dispose(&old->vars);
	free(old);
}

static symtable_node_t* scope_lookup(sem_context_t* context, const char* key) {
	sem_scope_t* current = context->top;
	symtable_node_t* result = NULL;
	while (current != NULL && (result = symtable_search(&current->vars, key)) == NULL) {
		current = current->next;
	}
	return result;
}

void sem_error(int err_code, const char* err_msg) {
	fprintf(stderr, "%s\n", err_msg);
	exit(err_code);
}