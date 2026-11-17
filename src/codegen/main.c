#include "codegen.h"
#include <stdio.h>

static unsigned int label_counter = 0;

static void gen_function(ASTNode* func_def);
static void gen_collect_locals(ASTNode* block);


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

static void gen_function(ASTNode* func_def) { // prozatimni
	printf("LABEL $fn$%s\n", func_def->as.func_def.name);
	printf("CREATEFRAME\nPUSHFRAME\nPUSHS nil@nil\nPOPFRAME\nRETURN\n");
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