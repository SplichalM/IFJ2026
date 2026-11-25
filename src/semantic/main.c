/**
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 *
 * Soubor: main.c (semantic)
 * Popis: Sémantická analýza: kontrola typů, rozsahů platnosti a volání funkcí, vkládání implicitních konverzí
 *
 * Autoři:
 * Tomáš Schablický (xschabt00)
 * Azíz Hammadi ()
 */

#include "semantic.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define SEM_BLOCK_RETURNS ((DataType)-1)

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

static void sem_error(int err_code, const char* err_msg); // Pomocná funkce pro vracení error kódů
static void sem_analyze_function(symtable_t* functions, ASTNode* func_def);
static DataType sem_identifier(sem_context_t* context, ASTNode* node);
static DataType sem_call(sem_context_t* context, ASTNode* node);

static DataType sem_expr_type(sem_context_t* context, ASTNode* node);
static DataType sem_binary_op(sem_context_t* context, ASTNode* node);
static DataType sem_unary_op(sem_context_t* context, ASTNode* node);
static void sem_insert_conversion(ASTNode** subnode_ptr);
static bool sem_types_compatible(DataType target, DataType source);
static void sem_check_assignable(sem_context_t* context, DataType target, ASTNode** expr_ptr);
static void sem_check_condition(sem_context_t* context, ASTNode* cond);

static void sem_return(sem_context_t* context, ASTNode* node);
static DataType sem_block_result(sem_context_t* context, ASTNode* block);
static void sem_check_function_result(sem_context_t* context, sym_data_t* func);
static void sem_ensure_function_analyzed(symtable_t* functions, sym_data_t* func);

static void scope_push(sem_context_t* context);
static void scope_pop(sem_context_t* context);
static sym_data_t* scope_lookup(sem_context_t* context, const char* key);
static sym_data_t* scope_declare(sem_context_t* context, const char* key, DataType type, bool is_mutable);
static char* sem_unique_name(sem_context_t* context, const char* name);

static ASTNode* sem_check_main(ASTNode* tree);
static void sem_block(sem_context_t* context, ASTNode* block);
static void sem_statement(sem_context_t* context, ASTNode* current);
static void sem_var_decl(sem_context_t* context, ASTNode* node);
static void sem_assign(sem_context_t* context, ASTNode* node);
static void sem_if(sem_context_t* context, ASTNode* node);
static void sem_while(sem_context_t* context, ASTNode* node);
static void sem_add_builtin(symtable_t* symtable, const char* name, DataType return_type, int param_count, const DataType* param_types);
static char* sem_copy_name(const char* name);

static DataType sem_member(sem_context_t* context, ASTNode* node);
static DataType sem_if_expr(sem_context_t* context, ASTNode* node);
static void convert_block_tail_to_double(ASTNode** body_ptr);
static void sem_for(sem_context_t* context, ASTNode* node);


int sem_analyze(ASTNode* tree, symtable_t* functions) {
	ASTNode* main_func = sem_check_main(tree);
	ASTNode* current = tree->as.list.head;
	sym_data_t* func = NULL;
	while (current != NULL) {
		func = symtable_search(functions, current->as.func_def.name);
		sem_ensure_function_analyzed(functions, func);
		current = current->next;
	}

	if (main_func->as.func_def.return_type != TYPE_UNIT) {
		sem_error(6, "Main function does not return type unit");
	}

	return 0;
}

static void sem_analyze_function(symtable_t* functions, ASTNode* func_def) {
	sym_data_t* func = symtable_search(functions, func_def->as.func_def.name);

	sem_context_t context;
	context.functions = functions;
	context.top = NULL;
	context.curr_func = func;
	context.count = 0;

	scope_push(&context);

	ASTNode* current = func_def->as.func_def.params->as.list.head;
	while (current != NULL) {
		sym_data_t* var = scope_declare(&context, current->as.func_param.name, current->as.func_param.param_type, false);
		current->unique_name = sem_copy_name(var->as.var.unique_name);
		current = current->next;
	}
	sem_block(&context, func_def->as.func_def.body);
	sem_check_function_result(&context, func);
	scope_pop(&context);
}

static void sem_block(sem_context_t* context, ASTNode* block) {
	scope_push(context);
	ASTNode* current = block->as.list.head;
	while (current != NULL) {
		sem_statement(context, current);
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

static void sem_error(int err_code, const char* err_msg) {
	fprintf(stderr, "%s\n", err_msg);
	exit(err_code);
}

static ASTNode* sem_check_main(ASTNode* tree) {
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
	if (main_func->as.func_def.return_type != TYPE_UNIT && main_func->as.func_def.return_type != TYPE_UNKNOWN) {
		sem_error(6, "Main function is not returning type unit or unknown");
	}
	return main_func;
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
		sem_expr_type(context, current);
		break;
	case NODE_RETURN:
		sem_return(context, current);
		break;
	case NODE_FOR:
		sem_for(context, current);
		break;
	default:
		sem_expr_type(context, current);
		break;
	}
}

static DataType sem_identifier(sem_context_t* context, ASTNode* node) {
	sym_data_t* var = scope_lookup(context, node->as.identifier.name);
	if (var == NULL) {
		sem_error(3, "Variable not defined");
	}
	node->unique_name = sem_copy_name(var->as.var.unique_name);
	return var->as.var.var_type;
}

static void sem_var_decl(sem_context_t* context, ASTNode* node) {
	DataType var_type = sem_expr_type(context, node->as.var_decl.value);

	if (node->as.var_decl.declared_type != TYPE_UNKNOWN) {
		var_type = node->as.var_decl.declared_type;
		sem_check_assignable(context, node->as.var_decl.declared_type, &node->as.var_decl.value);
	}
	sym_data_t* var = scope_declare(context, node->as.var_decl.name, var_type, node->as.var_decl.is_mutable);
	node->unique_name = sem_copy_name(var->as.var.unique_name);
}

static void sem_assign(sem_context_t* context, ASTNode* node) {
	sym_data_t* var = scope_lookup(context, node->as.assign.target_name);
	if (var == NULL) {
		sem_error(3, "Not defined");
	}
	if (var->as.var.is_mutable == false) {
		sem_error(7, "Non mutable");
	}
	node->unique_name = sem_copy_name(var->as.var.unique_name);
	sem_expr_type(context, node->as.assign.value);
	sem_check_assignable(context, var->as.var.var_type, &node->as.assign.value);
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

void register_user_function(symtable_t *functions, ASTNode *definition)
{
    sym_data_t *symbol = symtable_insert(
        functions, definition->as.func_def.name, SYM_FUNCTION
    );
    if (symbol == NULL) {
        sem_error(4, "Function already defined");
    }

    func_data_t *func = &symbol->as.func;
    func->return_type = definition->as.func_def.return_type;
    func->expl_return_type = func->return_type != TYPE_UNKNOWN;
    func->is_main = definition->as.func_def.is_main;
    func->definition = definition;
    func->param_count = 0;
    func->param_types = NULL;

    ASTNode *first_param = definition->as.func_def.params->as.list.head;
    for (ASTNode *param = first_param; param != NULL; param = param->next) {
        func->param_count++;
    }

    if (func->param_count == 0) {
        return;
    }

    func->param_types = malloc(func->param_count * sizeof(*func->param_types));
    if (func->param_types == NULL) {
        sem_error(99, "Could not allocate parameter types");
    }

    int index = 0;
    for (ASTNode *param = first_param; param != NULL; param = param->next) {
        func->param_types[index++] = param->as.func_param.param_type;
    }
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

	sem_add_builtin(symtable, "readBoolean", TYPE_BOOL, 0, NULL);
}

static DataType sem_call(sem_context_t* context, ASTNode* node) {
	sym_data_t* func = symtable_search(context->functions, node->as.func_call.name);
	if (func == NULL) {
		sem_error(3, "Function not defined");
	}
	ASTNode** slot = &node->as.func_call.args->as.list.head;
	int i = 0;
	while (*slot != NULL) {
		if (i >= func->as.func.param_count) {
			sem_error(5, "Too many arguments");
		}
		DataType type = sem_expr_type(context, *slot);
		if (!sem_types_compatible(func->as.func.param_types[i], type)) {
			sem_error(5, "Wrong parameter type");
		}
		if (func->as.func.param_types[i] == TYPE_DOUBLE && type == TYPE_INT) {
			sem_insert_conversion(slot);
		}
		i++;
		if ((*slot)->next == NULL) {
			node->as.func_call.args->as.list.tail = *slot;
		}
		slot = &(*slot)->next;
	}
	if (i < func->as.func.param_count) {
		sem_error(5, "Too little arguments");
	}

	if (func->as.func.expl_return_type == false) {
		sem_ensure_function_analyzed(context->functions, func);
	}
	return func->as.func.return_type;
}

static void sem_insert_conversion(ASTNode** subnode_ptr) {
	if (subnode_ptr == NULL || *subnode_ptr == NULL) return;

	ASTNode* orig = *subnode_ptr;


	ASTNode* conv = calloc(1, sizeof(ASTNode));
	if (conv == NULL) {
		sem_error(99, "Error allocating memory for conversion node");
	}
	conv->type = NODE_INT2DOUBLE;
	conv->data_type = TYPE_DOUBLE;
	conv->as.op.left = orig;
	conv->as.op.right = NULL;
	conv->as.op.op = NULL;

	conv->next = orig->next;
	orig->next = NULL;

	*subnode_ptr = conv;
}

static bool sem_types_compatible(DataType target, DataType source) {
	if (target == TYPE_ANY) {
		return true;
	}
	if (target == source) {
		return true;
	}
	if (target == TYPE_DOUBLE && source == TYPE_INT) {
		return true;
	}
	return false;
}

static void sem_check_assignable(sem_context_t* context, DataType target, ASTNode** expr_ptr) {
	(void)context;
	DataType source = (*expr_ptr)->data_type;

	if (!sem_types_compatible(target, source)) {
		sem_error(6, "Incompatible types in assignment or return");
	}
	if (target == TYPE_DOUBLE && source == TYPE_INT) {
		sem_insert_conversion(expr_ptr);
	}
}

static DataType sem_expr_type(sem_context_t* context, ASTNode* node) {
	if (node == NULL) {
		sem_error(99, "NULL expression node");
	}

	DataType result_type = TYPE_UNKNOWN;

	switch (node->type) {
	case NODE_LITERAL:
		switch (node->as.literal.type) {
		case LIT_INT:    result_type = TYPE_INT; break;
		case LIT_DOUBLE: result_type = TYPE_DOUBLE; break;
		case LIT_STRING: result_type = TYPE_STRING; break;
		case LIT_UNIT:   result_type = TYPE_UNIT; break;
		case LIT_BOOL:   result_type = TYPE_BOOL; break;
		default:         sem_error(99, "Unknown literal type");
		}
		break;

	case NODE_IDENTIFIER:
		result_type = sem_identifier(context, node);
		break;

	case NODE_BINARY_OP:
		result_type = sem_binary_op(context, node);
		break;

	case NODE_UNARY_OP:
		result_type = sem_unary_op(context, node);
		break;

	case NODE_FUNC_CALL:
		result_type = sem_call(context, node);
		break;

	case NODE_INT2DOUBLE:
		result_type = TYPE_DOUBLE;
		break;

	case NODE_MEMBER:
		result_type = sem_member(context, node);
		break;

	case NODE_IF_EXPR:
		result_type = sem_if_expr(context, node);
		break;

	default:
		sem_error(99, "Invalid AST node in expression");
	}

	node->data_type = result_type;
	return result_type;
}

static DataType sem_unary_op(sem_context_t* context, ASTNode* node) {
	DataType operand_type = sem_expr_type(context, node->as.op.left);
	const char* op = node->as.op.op;

	if (strcmp(op, "-") == 0 || strcmp(op, "+") == 0) {
		if (operand_type != TYPE_INT && operand_type != TYPE_DOUBLE) {
			sem_error(6, "Unary + or - requires numeric operand");
		}
		node->data_type = operand_type;
		return operand_type;
	}

	if (strcmp(op, "!") == 0) {
		if (operand_type != TYPE_BOOL) {
			sem_error(6, "Boolean operation on non boolean variable");
		}
		node->data_type = TYPE_BOOL;
		return TYPE_BOOL;
	}

	sem_error(6, "Unsupported unary operator");
	return TYPE_UNKNOWN;
}

static DataType sem_binary_op(sem_context_t* context, ASTNode* node) {
	DataType left_t = sem_expr_type(context, node->as.op.left);
	DataType right_t = sem_expr_type(context, node->as.op.right);
	const char* op = node->as.op.op;

	if ((strcmp(op, "&&") == 0 || strcmp(op, "||") == 0)) {
		if (left_t != TYPE_BOOL || right_t != TYPE_BOOL) {
			sem_error(6, "Boolean operation on non boolean variables");
		}
		node->data_type = TYPE_BOOL;
		return TYPE_BOOL;
	}
	

	bool left_is_num = (left_t == TYPE_INT || left_t == TYPE_DOUBLE);
	bool right_is_num = (right_t == TYPE_INT || right_t == TYPE_DOUBLE);
	bool both_num = left_is_num && right_is_num;


	if (strcmp(op, "+") == 0) {
		if (left_t == TYPE_STRING && right_t == TYPE_STRING) {
			node->data_type = TYPE_STRING;
			return TYPE_STRING;
		}
		if (both_num) {
			if (left_t == TYPE_INT && right_t == TYPE_DOUBLE) {
				sem_insert_conversion(&node->as.op.left);
				node->data_type = TYPE_DOUBLE;
				return TYPE_DOUBLE;
			}
			if (left_t == TYPE_DOUBLE && right_t == TYPE_INT) {
				sem_insert_conversion(&node->as.op.right);
				node->data_type = TYPE_DOUBLE;
				return TYPE_DOUBLE;
			}
			node->data_type = left_t;
			return left_t;
		}
		sem_error(6, "Type mismatch for operator '+'");
	}


	if (strcmp(op, "-") == 0 || strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
		if (!both_num) {
			sem_error(6, "Arithmetic operators '-', '*', '/' require numeric operands");
		}
		if (left_t == TYPE_INT && right_t == TYPE_DOUBLE) {
			sem_insert_conversion(&node->as.op.left);
			node->data_type = TYPE_DOUBLE;
			return TYPE_DOUBLE;
		}
		if (left_t == TYPE_DOUBLE && right_t == TYPE_INT) {
			sem_insert_conversion(&node->as.op.right);
			node->data_type = TYPE_DOUBLE;
			return TYPE_DOUBLE;
		}
		node->data_type = left_t; 
		return left_t;
	}


	if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
	    strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
		if (left_t == TYPE_STRING && right_t == TYPE_STRING) {
			node->data_type = TYPE_BOOL;
			return TYPE_BOOL;
		}
		if (both_num) {
			if (left_t == TYPE_INT && right_t == TYPE_DOUBLE) {
				sem_insert_conversion(&node->as.op.left);
			} else if (left_t == TYPE_DOUBLE && right_t == TYPE_INT) {
				sem_insert_conversion(&node->as.op.right);
			}
			node->data_type = TYPE_BOOL;
			return TYPE_BOOL;
		}
		sem_error(6, "Relational operators require two numbers or two strings");
	}


	if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
		if (both_num) {
			if (left_t == TYPE_INT && right_t == TYPE_DOUBLE) {
				sem_insert_conversion(&node->as.op.left);
			} else if (left_t == TYPE_DOUBLE && right_t == TYPE_INT) {
				sem_insert_conversion(&node->as.op.right);
			}
			node->data_type = TYPE_BOOL;
			return TYPE_BOOL;
		}
		if (left_t == right_t) {
			node->data_type = TYPE_BOOL;
			return TYPE_BOOL;
		}
		sem_error(6, "Equality operators require compatible types");
	}

	sem_error(6, "Unknown binary operator");
	return TYPE_UNKNOWN;
}

static void sem_check_condition(sem_context_t* context, ASTNode* cond) {
	DataType cond_type = sem_expr_type(context, cond);
	if (cond_type != TYPE_BOOL) {
		sem_error(6, "Condition in if/while must be a relational expression of type Boolean");
	}
}

static void sem_return(sem_context_t* context, ASTNode* node) {
	if (context->curr_func == NULL) {
		sem_error(99, "Return statement outside of function context");
	}

	if (!context->curr_func->as.func.expl_return_type) {
		sem_error(6, "Function containing 'return' must have an explicit return type");
	}

	DataType expected_ret = context->curr_func->as.func.return_type;

	if (node->as.ret_stmt.expr == NULL) {

		if (expected_ret != TYPE_UNIT) {
			sem_error(6, "Empty return is only allowed in a function with explicit Unit return type");
		}
	} else {
		sem_expr_type(context, node->as.ret_stmt.expr);

		if (expected_ret != TYPE_UNIT) {
			sem_check_assignable(context, expected_ret, &node->as.ret_stmt.expr);
		}
	}
}

static DataType sem_block_result(sem_context_t* context, ASTNode* block) {
	if (block == NULL) {
		return TYPE_UNIT;
	}


	ASTNode* tail = (block->type == NODE_BLOCK) ? block->as.list.tail : block;
	if (tail == NULL) {
		return TYPE_UNIT;
	}

	switch (tail->type) {
	case NODE_VAR_DECL:
	case NODE_ASSIGN:
	case NODE_WHILE:
	case NODE_FOR:
		return TYPE_UNIT;

	case NODE_RETURN:
		return SEM_BLOCK_RETURNS;

	case NODE_IF: {
		DataType then_res = sem_block_result(context, tail->as.if_stmt.then_block);
		DataType else_res = (tail->as.if_stmt.else_block != NULL)
			? sem_block_result(context, tail->as.if_stmt.else_block)
			: TYPE_UNIT;


		if (context->curr_func->as.func.expl_return_type &&
		    context->curr_func->as.func.return_type == TYPE_UNIT) {
			return TYPE_UNIT;
		}

		if (then_res == SEM_BLOCK_RETURNS && else_res == SEM_BLOCK_RETURNS) {
			return SEM_BLOCK_RETURNS;
		}

		if (!context->curr_func->as.func.expl_return_type &&
		    then_res == TYPE_UNIT && else_res == TYPE_UNIT) {
			return TYPE_UNIT;
		}

		if (tail->as.if_stmt.else_block != NULL && then_res != SEM_BLOCK_RETURNS &&
			else_res != SEM_BLOCK_RETURNS) {

			if (then_res == else_res) {
				tail->type = NODE_IF_EXPR;
				tail->data_type = then_res;
				return then_res;
			}
			else if (then_res == TYPE_INT && else_res == TYPE_DOUBLE) {
				convert_block_tail_to_double(&tail->as.if_stmt.then_block);

				tail->type = NODE_IF_EXPR;
				tail->data_type = TYPE_DOUBLE;
				return TYPE_DOUBLE;
			}
			else if (then_res == TYPE_DOUBLE && else_res == TYPE_INT) {
				convert_block_tail_to_double(&tail->as.if_stmt.else_block);
				tail->type = NODE_IF_EXPR;
				tail->data_type = TYPE_DOUBLE;
				return TYPE_DOUBLE;
			}
		}

		sem_error(6, "Invalid 'if' statement at the end of function body");
		return TYPE_UNKNOWN;
	}

	default:

		return tail->data_type;
	}
}


static void convert_block_tail_to_double(ASTNode** body_ptr) {
	if (body_ptr == NULL || *body_ptr == NULL) return;

	ASTNode* body = *body_ptr;
	if (body->type != NODE_BLOCK) {
		sem_insert_conversion(body_ptr);
		return;
	}

	if (body->as.list.head == NULL) return;

	ASTNode** curr_ptr = &body->as.list.head;
	while ((*curr_ptr)->next != NULL) {
		curr_ptr = &((*curr_ptr)->next);
	}
	sem_insert_conversion(curr_ptr);
	body->as.list.tail = *curr_ptr;
}

static void sem_check_function_result(sem_context_t* context, sym_data_t* func) {
	ASTNode* body = func->as.func.definition->as.func_def.body;
	DataType body_res = sem_block_result(context, body);


	if (func->as.func.expl_return_type) {

		if (func->as.func.return_type == TYPE_UNIT) {
			return;
		}

		if (body_res == SEM_BLOCK_RETURNS) {
			return;
		}

		if (!sem_types_compatible(func->as.func.return_type, body_res)) {
			sem_error(6, "Function body result does not match explicit return type");
		}

		if (func->as.func.return_type == TYPE_DOUBLE && body_res == TYPE_INT) {
			convert_block_tail_to_double(&func->as.func.definition->as.func_def.body);
		}
	} else {

		func->as.func.return_type = body_res;
		func->as.func.definition->as.func_def.return_type = body_res;
	}
}

static void sem_ensure_function_analyzed(symtable_t* functions, sym_data_t* func) {
	if (func == NULL) return;

	if (func->as.func.state == FUNC_DONE) {
		return;
	}
	if (func->as.func.state == FUNC_IN_PROGRESS) {
		sem_error(6, "Recursive function without explicit return type");
	}

	func->as.func.state = FUNC_IN_PROGRESS;
	sem_analyze_function(functions, func->as.func.definition);
	func->as.func.state = FUNC_DONE;
}

static char* sem_copy_name(const char* name) {
	char* tmp = malloc((strlen(name) + 1) * sizeof(char));
	if (tmp == NULL) {
		sem_error(99, "Could not allocate memory");
	}
	strcpy(tmp, name);
	return tmp;
}

static DataType sem_member(sem_context_t* context, ASTNode* node) {
	DataType recv = sem_expr_type(context, node->as.member.receiver);
	const char* name = node->as.member.name;

	if (strcmp(name, "toString") == 0) {
		node->data_type = TYPE_STRING;
		return TYPE_STRING;
	}
	if (strcmp(name, "toInt") == 0) {
		if (recv != TYPE_DOUBLE && recv != TYPE_INT) {
			sem_error(6, "Not double or int");
		}
		node->data_type = TYPE_INT;
		return TYPE_INT;
	}
	if (strcmp(name, "floor") == 0) {
		if (recv != TYPE_DOUBLE) {
			sem_error(6, "Not double");
		}
		node->data_type = TYPE_DOUBLE;
		return TYPE_DOUBLE;
	}
	if (strcmp(name, "length") == 0) {
		if (recv != TYPE_STRING) {
			sem_error(6, "Not a string");
		}
		node->data_type = TYPE_INT;
		return TYPE_INT;
	}

	sem_error(3, "Unknown member");
	return TYPE_UNKNOWN;
}

static DataType sem_if_expr(sem_context_t* context, ASTNode* node) {
	sem_check_condition(context, node->as.if_stmt.cond);

	if (node->as.if_stmt.else_block == NULL) {
		sem_error(6, "No else defined");
	}

	sem_block(context, node->as.if_stmt.then_block);
	sem_block(context, node->as.if_stmt.else_block);

	DataType then_t = sem_block_result(context, node->as.if_stmt.then_block);
	DataType else_t = sem_block_result(context, node->as.if_stmt.else_block);

	if (then_t == else_t) {
		return then_t;
	}
	else if (then_t == TYPE_INT && else_t == TYPE_DOUBLE) {
		convert_block_tail_to_double(&node->as.if_stmt.then_block);
		return TYPE_DOUBLE;
	}
	else if (then_t == TYPE_DOUBLE && else_t == TYPE_INT) {
		convert_block_tail_to_double(&node->as.if_stmt.else_block);
		return TYPE_DOUBLE;
	}
	else {
		sem_error(6, "Incompatible data types");
	}
	return TYPE_UNKNOWN;
}

static void sem_for(sem_context_t* context, ASTNode* node) {
	if (sem_expr_type(context, node->as.for_stmt.from) != TYPE_INT) {
		sem_error(6, "From variable type not int");
	}
	if (sem_expr_type(context, node->as.for_stmt.to) != TYPE_INT) {
		sem_error(6, "To variable type not int");
	}
	if (node->as.for_stmt.step != NULL) {
		if (sem_expr_type(context, node->as.for_stmt.step) != TYPE_INT) {
			sem_error(6, "Step variable type not int");
		}
	}

	if (node->as.for_stmt.step != NULL && node->as.for_stmt.step->type == NODE_LITERAL 
		&& node->as.for_stmt.step->as.literal.type == LIT_INT
		&& node->as.for_stmt.step->as.literal.val.i_val == 0) {

		sem_error(10, "Step literal cannot be 0");
	}

	node->as.for_stmt.to_name = sem_unique_name(context, "$to");
	node->as.for_stmt.step_name = sem_unique_name(context, "$step");

	scope_push(context);
	sym_data_t* var = scope_declare(context, node->as.for_stmt.var_name, TYPE_INT, false);
	node->unique_name = sem_copy_name(var->as.var.unique_name);

	ASTNode* current = node->as.for_stmt.guards->as.list.head;
	while (current != NULL) {
		sem_check_condition(context, current);
		current = current->next;
	}

	sem_block(context, node->as.for_stmt.body);
	scope_pop(context);
}