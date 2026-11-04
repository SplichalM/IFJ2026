#include "semantic.h"

int sem_analyze(ASTNode* tree, symtable_t* functions) {
	sem_check_main(tree);
	ASTNode* current = tree->as.list.head;
	while (current != NULL) {
		sem_analyze_function(functions, current);
		current = current->next;
	}

	return 0;
}

void sem_analyze_function(symtable_t* functions, ASTNode* func_def) {
	symtable_node_t* func = symtable_search(functions, func_def->as.func_def.name);

	sem_context_t context;
	context.functions = functions;
	context.top = NULL;
	context.curr_func = func;
	context.count = 0;

	scope_push(&context);

	ASTNode* current = func_def->as.func_def.params->as.list.head;
	while (current != NULL) {
		scope_declare(&context, current->as.func_param.name, current->as.func_param.param_type, false);
		current = current->next;
	}
	sem_block(&context, func_def->as.func_def.body);
	scope_pop(&context);
}

static void sem_block(sem_context_t* context, ASTNode* block) {
	scope_push(context);
	ASTNode* current = block->as.list.head;
	while (current != NULL) {
		sem_statement(context, current); // jeste neni
		current = current->next;
	}
	scope_pop(context);
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

static symtable_node_t* scope_declare(sem_context_t* context, const char* key, DataType type, bool is_mutable) {
	// type a is_mutable zatim nevyuzite, musim zmenit symtable - tomas
	symtable_insert(&context->top->vars, key, SYM_LOCAL_VAR, 0);
	symtable_node_t* result = symtable_search(&context->top->vars, key);
	return result;
}

static char* sem_unique_name(sem_context_t* context, const char* name) {
	int length = snprintf(NULL, 0, "%s$%u", name, context->count);
	char* buffer = malloc(sizeof(char) * (length + 1));
	if (buffer == NULL) {
		sem_error(99, "Error allocating memory for unique name");
	}
	snprintf(buffer, length + 1, "%s$%u", name, context->count);
	context->count++;
	return buffer;
}

void sem_error(int err_code, const char* err_msg) {
	fprintf(stderr, "%s\n", err_msg);
	exit(err_code);
}

static void sem_check_main(ASTNode* tree) {
	ASTNode* current = tree->as.list.head;
	int main_count = 0;
	ASTNode* main_func = NULL;
	while (current != NULL) {
		if (current->as.func_def.is_main == true) {
			main_count++;
			main_func = current;
		}
		current = current->next;
	}
	if (main_count == 0) {
		sem_error(3, "Main function not defined");
	}
	else if (main_count > 1) {
		sem_error(4, "Multiple definitions of main function");
	}
	if (main_func->as.func_def.params->as.list.head != NULL) {
		sem_error(5, "Main function has parameters");
	}
	if (main_func->as.func_def.return_type != TYPE_UNIT) {
		sem_error(6, "Main function is not returning type unit");
	}
}

static void sem_statement(sem_context_t* context, ASTNode* current) {
	switch (current->type) {
	case NODE_VAR_DECL:
		sem_var_decl(context, current);
		break;
	case NODE_ASSIGN:
		sem_assign(context, current);
		break;
	case NODE_IF:
		sem_if(context, current);
		break;
	case NODE_WHILE:
		sem_while(context, current);
		break;
	case NODE_FUNC_CALL:
		break;
	case NODE_RETURN:
		if (current->as.ret_stmt.expr != NULL) {
			sem_expr_type(context, current->as.ret_stmt.expr);
		}
		break;
	default:
		sem_expr_type(context, current);
		break;
	}
}