#include "codegen.h"
#include <stdio.h>

static unsigned int label_counter = 0;

static void gen_function(ASTNode* func_def);
static void gen_collect_locals(ASTNode* block);
static void gen_pop_params(ASTNode* param);


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

	printf("PUSHS nil@nil\n"); // nahradi se gen_body
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
