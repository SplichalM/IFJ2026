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
	sym_data_t* func = symtable_search(functions, func_def->as.func_def.name);

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

static sym_data_t* scope_lookup(sem_context_t* context, const char* key) {
	sem_scope_t* current = context->top;
	sym_data_t* result = NULL;
	while (current != NULL && (result = symtable_search(&current->vars, key)) == NULL) {
		current = current->next;
	}
	return result;
}

static sym_data_t* scope_declare(sem_context_t* context, const char* key, DataType type, bool is_mutable) {
	sym_data_t* var = symtable_insert(&context->top->vars, key, SYM_LOCAL_VAR);
	if (var == NULL) {
		sem_error(4, "Variable already exists");
	}
	var->as.var.var_type = type;
	var->as.var.is_mutable = is_mutable;
	var->as.var.unique_name = sem_unique_name(context, key);
	return var;
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

DataType sem_identifier(sem_context_t* context, ASTNode* node) {
	sym_data_t* var = scope_lookup(context, node->as.identifier.name);
	if (var == NULL) {
		sem_error(3, "Variable not defined");
	}
	return var->as.var.var_type;
}

static void sem_var_decl(sem_context_t* context, ASTNode* node) {
	DataType var_type = sem_expr_type(context, node->as.var_decl.value);

	if (node->as.var_decl.declared_type != TYPE_UNKNOWN) {
		var_type = node->as.var_decl.declared_type;
	}
	scope_declare(context, node->as.var_decl.name, var_type, node->as.var_decl.is_mutable);
}

static void sem_assign(sem_context_t* context, ASTNode* node) {
	sym_data_t* var = scope_lookup(context, node->as.assign.target_name);
	if (var == NULL) {
		sem_error(3, "Not defined");
	}
	if (var->as.var.is_mutable == false) {
		sem_error(7, "Non mutable");
	}
	sem_expr_type(context, node->as.assign.value);
}

static void sem_if(sem_context_t* context, ASTNode* node) {
	sem_check_condition(context, node->as.if_stmt.cond);
	sem_block(context, node->as.if_stmt.then_block);
	if (node->as.if_stmt.else_block != NULL) {
		sem_block(context, node->as.if_stmt.else_block);
	}
}

static void sem_while(sem_context_t* context, ASTNode* node) {
	sem_check_condition(context, node->as.while_stmt.cond);
	sem_block(context, node->as.while_stmt.body);
}

static void sem_add_builtin(symtable_t* symtable, const char* name, DataType return_type, int param_count, const DataType* param_types) {
	sym_data_t* func = symtable_insert(symtable, name, SYM_FUNCTION);
	if (func == NULL) {
		sem_error(99, "");
	}
	func->as.func.return_type = return_type;
	func->as.func.param_count = param_count;
	func->as.func.state = FUNC_DONE;
	func->as.func.is_builtin = true;
	func->as.func.expl_return_type = true;
	if (param_count != 0) {
		func->as.func.param_types = malloc(sizeof(DataType) * param_count);
		if (func->as.func.param_types == NULL) {
			sem_error(99, "");
		}
		memcpy(func->as.func.param_types, param_types, sizeof(DataType) * param_count);
	}
}

void sem_register_builtins(symtable_t* symtable) {
	sem_add_builtin(symtable, "print", TYPE_UNIT, 1, (DataType[]) { TYPE_ANY });
	sem_add_builtin(symtable, "readLine", TYPE_STRING, 0, NULL);
	sem_add_builtin(symtable, "readInt", TYPE_INT, 0, NULL);
	sem_add_builtin(symtable, "readDouble", TYPE_DOUBLE, 0, NULL);
	sem_add_builtin(symtable, "toStr", TYPE_STRING, 1, (DataType[]) { TYPE_ANY });
	sem_add_builtin(symtable, "toInt", TYPE_INT, 1, (DataType[]) { TYPE_DOUBLE });
	sem_add_builtin(symtable, "strLength", TYPE_INT, 1, (DataType[]) { TYPE_STRING });
	sem_add_builtin(symtable, "subStr", TYPE_STRING, 3, (DataType[]) { TYPE_STRING, TYPE_INT, TYPE_INT });
	sem_add_builtin(symtable, "ord", TYPE_INT, 2, (DataType[]) { TYPE_STRING, TYPE_INT });
	sem_add_builtin(symtable, "chr", TYPE_STRING, 1, (DataType[]) { TYPE_INT });
}