#include "semantic.h"

int sem_analyze(ASTNode* tree, symtable_t* functions) {
	return 0;
}

static void scope_push(sem_context_t* context) {
	sem_scope_t* scope = malloc(sizeof(sem_scope_t));
	if (scope == NULL) {
		sem_error(99, "Error allocating memory for scope stack");
	}

	symtable_init(&scope->vars);
	scope->next = context->top;
	context->top = scope;
}

void sem_error(int err_code, const char* err_msg) {
	fprintf(stderr, "%s\n", err_msg);
	exit(err_code);
}