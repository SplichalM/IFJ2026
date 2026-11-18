#include "codegen.h"
#include <stdio.h>

static unsigned int label_counter = 0;

static ASTNode* current_function = NULL;

static void gen_function(ASTNode* func_def);
static void gen_collect_locals(ASTNode* block);
static void gen_pop_params(ASTNode* param);
static void gen_block(ASTNode* block);
static void gen_statement(ASTNode* node);

static void gen_var_decl(ASTNode* node);
static void gen_assign(ASTNode* node);
static void gen_if(ASTNode* node);
static void gen_while(ASTNode* node);
static void gen_return(ASTNode* node);

static void gen_body(ASTNode* func_def);


unsigned int gen_new_label() {
	label_counter++;
	return label_counter - 1;
}

void gen_program(ASTNode* tree) {
	printf(".IFJcode26\n"
		"DEFVAR GF@%%trash\n"
		"DEFVAR GF@%%tmp1\n"
		"DEFVAR GF@%%tmp2\n"
		"DEFVAR GF@%%tmp3\n");

	ASTNode* curr_func = tree->as.list.head;
	while (curr_func != NULL) {
		if (curr_func->as.func_def.is_main) {
			printf("CALL $fn$%s\n", curr_func->as.func_def.name);
		}
		curr_func = curr_func->next;
	}
	printf("EXIT int@0\n");

	curr_func = tree->as.list.head;
	while (curr_func != NULL) {
		gen_function(curr_func);
		curr_func = curr_func->next;
	}

	gen_builtin_helpers();
}

static void gen_function(ASTNode* func_def) {
	current_function = func_def;
	printf("LABEL $fn$%s\n", func_def->as.func_def.name);
	printf("CREATEFRAME\n"
		"PUSHFRAME\n");

	ASTNode* curr_param = func_def->as.func_def.params->as.list.head;
	while (curr_param != NULL) {
		printf("DEFVAR LF@%s\n", curr_param->unique_name);
		curr_param = curr_param->next;
	}

	gen_collect_locals(func_def->as.func_def.body);
	gen_pop_params(func_def->as.func_def.params->as.list.head);

	gen_body(func_def);
	printf("POPFRAME\n"
		"RETURN\n");
}

static void gen_collect_locals(ASTNode* block) {
	ASTNode* current = block->as.list.head;

	while (current != NULL) {
		if (current->type == NODE_VAR_DECL) {
			printf("DEFVAR LF@%s\n", current->unique_name);
		}
		else if (current->type == NODE_IF) {
			gen_collect_locals(current->as.if_stmt.then_block);
			if (current->as.if_stmt.else_block != NULL) {
				gen_collect_locals(current->as.if_stmt.else_block);
			}
		}
		else if (current->type == NODE_WHILE) {
			gen_collect_locals(current->as.while_stmt.body);
		}

		current = current->next;
	}
}

static void gen_pop_params(ASTNode* param) {
	if (param == NULL) return;
	gen_pop_params(param->next);
	printf("POPS LF@%s\n", param->unique_name);
}

static void gen_block(ASTNode* block) {
	ASTNode* curr_stmt = block->as.list.head;
	while (curr_stmt != NULL) {
		gen_statement(curr_stmt);
		curr_stmt = curr_stmt->next;
	}
}

static void gen_statement(ASTNode* node) {
	switch (node->type) {
	case NODE_VAR_DECL:
		gen_var_decl(node);
		break;
	case NODE_ASSIGN:
		gen_assign(node);
		break;
	case NODE_IF:
		gen_if(node);
		break;
	case NODE_WHILE:
		gen_while(node);
		break;
	case NODE_RETURN:
		gen_return(node);
		break;
	default:
		gen_expr(node);
		printf("POPS GF@%%trash\n");
		break;
	}
}

static void gen_var_decl(ASTNode* node) {
	gen_expr(node->as.var_decl.value);
	printf("POPS LF@%s\n", node->unique_name);
}

static void gen_assign(ASTNode* node) {
	gen_expr(node->as.assign.value);
	printf("POPS LF@%s\n", node->unique_name);
}


static void gen_if(ASTNode* node) {
	unsigned int label = gen_new_label();

	gen_expr(node->as.if_stmt.cond);

	printf("PUSHS bool@false\n");
	printf("JUMPIFEQS $if$%u$else\n", label);
	
	gen_block(node->as.if_stmt.then_block);

	printf("JUMP $if$%u$end\n", label);
	printf("LABEL $if$%u$else\n", label);
	if (node->as.if_stmt.else_block != NULL) {
		gen_block(node->as.if_stmt.else_block);
	}
	printf("LABEL $if$%u$end\n", label);
}

static void gen_while(ASTNode* node) {
	unsigned int label = gen_new_label();

	printf("LABEL $while$%u$start\n", label);
	gen_expr(node->as.while_stmt.cond);

	printf("PUSHS bool@false\n");
	printf("JUMPIFEQS $while$%u$end\n", label);

	gen_block(node->as.while_stmt.body);

	printf("JUMP $while$%u$start\n", label);
	printf("LABEL $while$%u$end\n", label);
}

static void gen_return(ASTNode* node) {
	if (node->as.ret_stmt.expr == NULL) {
		printf("PUSHS nil@nil\n");
	}
	else {
		gen_expr(node->as.ret_stmt.expr);
	}

	if (current_function->as.func_def.return_type == TYPE_UNIT && node->as.ret_stmt.expr != NULL) {
		printf("POPS GF@%%trash\n"
			"PUSHS nil@nil\n");
	}
	printf("POPFRAME\n"
		"RETURN\n");
}

static void gen_body(ASTNode* func_def) {
	if (func_def->as.func_def.return_type == TYPE_UNIT) {
		gen_block(func_def->as.func_def.body);
		printf("PUSHS nil@nil\n");
	}
	else {
		ASTNode* curr_stmt = func_def->as.func_def.body->as.list.head;
		while (curr_stmt != NULL) {
			if (curr_stmt->next == NULL) {
				if (curr_stmt->type == NODE_RETURN || curr_stmt->type == NODE_IF) {
					gen_statement(curr_stmt);
				}
				else {
					gen_expr(curr_stmt);
				}
			}
			else {
				gen_statement(curr_stmt);
			}
			curr_stmt = curr_stmt->next;
		}
	}
	
}