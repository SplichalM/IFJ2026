/**
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 *
 * Soubor: codegen.c
 * Popis: Generování výsledného kódu podle AST
 *
 * Autoři:
 * Tomáš Schablický (xschabt00)
 */

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

static void gen_collect_expr_locals(ASTNode* expr);
static void gen_for(ASTNode* node);
static void gen_value_block(ASTNode* block);


// Funkce vrací číslo pro unikátní návěští
unsigned int gen_new_label(void) {
	label_counter++;
	return label_counter - 1;
}

// Vypíše hlavičku, pomocné globální proměnné a volání main funkce
// pak nechá vygenerovat všechny funkce
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

// Vygeneruje návěští, rámec, parametry a lokální proměnné
// vybere parametry ze zásobníku a tělo funkce
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

// Projde blok a to i vnořené a vypíše proměnné na začátek funkce
static void gen_collect_locals(ASTNode* block) {
	ASTNode* current = block->as.list.head;

	while (current != NULL) {
		if (current->type == NODE_VAR_DECL) {
			printf("DEFVAR LF@%s\n", current->unique_name);
			gen_collect_expr_locals(current->as.var_decl.value);
		}
		else if (current->type == NODE_IF) {
			gen_collect_locals(current->as.if_stmt.then_block);
			if (current->as.if_stmt.else_block != NULL) {
				gen_collect_locals(current->as.if_stmt.else_block);
			}
			gen_collect_expr_locals(current->as.if_stmt.cond);
		}
		else if (current->type == NODE_WHILE) {
			gen_collect_locals(current->as.while_stmt.body);
			gen_collect_expr_locals(current->as.while_stmt.cond);
		}
		else if (current->type == NODE_ASSIGN) {
			gen_collect_expr_locals(current->as.assign.value);
		}
		else if (current->type == NODE_RETURN) {
			gen_collect_expr_locals(current->as.ret_stmt.expr);
		}
		else if (current->type == NODE_FOR) {
			printf("DEFVAR LF@%s\n", current->unique_name);
			printf("DEFVAR LF@%s\n", current->as.for_stmt.to_name);
			printf("DEFVAR LF@%s\n", current->as.for_stmt.step_name);

			gen_collect_expr_locals(current->as.for_stmt.from);
			gen_collect_expr_locals(current->as.for_stmt.to);
			gen_collect_expr_locals(current->as.for_stmt.step);

			if (current->as.for_stmt.guards != NULL) {
				ASTNode* guard = current->as.for_stmt.guards->as.list.head;
				while (guard != NULL) {
					gen_collect_expr_locals(guard);
					guard = guard->next;
				}
			}

			gen_collect_locals(current->as.for_stmt.body);
		}
		else {
			gen_collect_expr_locals(current);
		}

		current = current->next;
	}
}

// Rekurzivně vybere parametry ze zásobníku v opačném pořadí jak byly přidány
static void gen_pop_params(ASTNode* param) {
	if (param == NULL) return;
	gen_pop_params(param->next);
	printf("POPS LF@%s\n", param->unique_name);
}

// Vygeneruje všechny příkazy v bloku
static void gen_block(ASTNode* block) {
	ASTNode* curr_stmt = block->as.list.head;
	while (curr_stmt != NULL) {
		gen_statement(curr_stmt);
		curr_stmt = curr_stmt->next;
	}
}

// Rozbočovací funkce podle typu příkazu
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
	case NODE_FOR:
		gen_for(node);
		break;
	default:
		gen_expr(node);
		printf("POPS GF@%%trash\n");
		break;
	}
}

// Vyhodnotí výraz a uloží podle unikátního jména
static void gen_var_decl(ASTNode* node) {
	gen_expr(node->as.var_decl.value);
	printf("POPS LF@%s\n", node->unique_name);
}

// Vyhodnotí výraz a uloží podle unikátního jména
static void gen_assign(ASTNode* node) {
	gen_expr(node->as.assign.value);
	printf("POPS LF@%s\n", node->unique_name);
}

// Vygeneruje podmínku, skok na "jinak", pak větve if a návěští
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

// Vygeneruje návěští, podmínku, skok na konec, samotné tělo a skok zpátky
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

// Vyhodnotí návratovou hodnotu a ukončí funkci
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

// Vygeneruje tělo funkce, poslední výraz zůstává jako návratová hodnota na zásobníku
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

// Funkce hledá if ve výrazu a deklarace v jeho větvích
static void gen_collect_expr_locals(ASTNode* expr) {
	if (expr == NULL) return;

	if (expr->type == NODE_IF_EXPR) {
		gen_collect_expr_locals(expr->as.if_stmt.cond);
		gen_collect_locals(expr->as.if_stmt.then_block);
		gen_collect_locals(expr->as.if_stmt.else_block);
	}
	if (expr->type == NODE_BINARY_OP) {
		gen_collect_expr_locals(expr->as.op.left);
		gen_collect_expr_locals(expr->as.op.right);
	}
	if (expr->type == NODE_UNARY_OP || expr->type == NODE_INT2DOUBLE) {
		gen_collect_expr_locals(expr->as.op.left);
	}
	if (expr->type == NODE_FUNC_CALL) {
		if (expr->as.func_call.args == NULL) return;
		ASTNode* argument = expr->as.func_call.args->as.list.head;
		while (argument != NULL) {
			gen_collect_expr_locals(argument);
			argument = argument->next;
		}
	}
	if (expr->type == NODE_MEMBER) {
		gen_collect_expr_locals(expr->as.member.receiver);
	}
}

// CONTROL
// Funkce uloží meze a krok, nenulový krok se kontroluje za běhu, podmínka se vybere podle znaménka
// kroku, pokud se nesplní stráž tak přeskočí tělo
static void gen_for(ASTNode* node) {
	unsigned int label = gen_new_label();

	gen_expr(node->as.for_stmt.from);
	printf("POPS LF@%s\n", node->unique_name);
	gen_expr(node->as.for_stmt.to);
	printf("POPS LF@%s\n", node->as.for_stmt.to_name);

	if (node->as.for_stmt.step != NULL) {
		gen_expr(node->as.for_stmt.step);
		printf("POPS LF@%s\n", node->as.for_stmt.step_name);

		if (node->as.for_stmt.step->type != NODE_LITERAL) {
			printf("JUMPIFNEQ $for$%u$ok LF@%s int@0\n", label, node->as.for_stmt.step_name);
			printf("EXIT int@40\n");
			printf("LABEL $for$%u$ok\n", label);
		}
	}
	else {
		printf("MOVE LF@%s int@1\n", node->as.for_stmt.step_name);
	}

	printf("LABEL $for$%u$start\n", label);
	printf("LT GF@%%tmp1 LF@%s int@0\n", node->as.for_stmt.step_name);
	printf("JUMPIFEQ $for$%u$neg GF@%%tmp1 bool@true\n", label);

	if (node->as.for_stmt.inclusive) {
		printf("GT GF@%%tmp1 LF@%s LF@%s\n", node->unique_name, node->as.for_stmt.to_name);
		printf("JUMPIFEQ $for$%u$end GF@%%tmp1 bool@true\n", label);
	}
	else {
		printf("LT GF@%%tmp1 LF@%s LF@%s\n", node->unique_name, node->as.for_stmt.to_name);
		printf("JUMPIFEQ $for$%u$end GF@%%tmp1 bool@false\n", label);
	}

	printf("JUMP $for$%u$body\n", label);
	printf("LABEL $for$%u$neg\n", label);

	if (node->as.for_stmt.inclusive) {
		printf("LT GF@%%tmp1 LF@%s LF@%s\n", node->unique_name, node->as.for_stmt.to_name);
		printf("JUMPIFEQ $for$%u$end GF@%%tmp1 bool@true\n", label);
	}
	else {
		printf("GT GF@%%tmp1 LF@%s LF@%s\n", node->unique_name, node->as.for_stmt.to_name);
		printf("JUMPIFEQ $for$%u$end GF@%%tmp1 bool@false\n", label);
	}

	printf("LABEL $for$%u$body\n", label);

	if (node->as.for_stmt.guards != NULL) {
		ASTNode* guard = node->as.for_stmt.guards->as.list.head;
		while (guard != NULL) {
			gen_expr(guard);
			printf("PUSHS bool@false\n");
			printf("JUMPIFEQS $for$%u$next\n", label);
			guard = guard->next;
		}
	}

	gen_block(node->as.for_stmt.body);
	printf("LABEL $for$%u$next\n", label);
	printf("ADD LF@%s LF@%s LF@%s\n", node->unique_name, node->unique_name, node->as.for_stmt.step_name);
	printf("JUMP $for$%u$start\n", label);
	printf("LABEL $for$%u$end\n", label);
}

// CONTROL
// Generuje if jako výraz, hodnota následně zůstává na zásobníku
void gen_if_expr(ASTNode* node) {
	unsigned int label = gen_new_label();
	gen_expr(node->as.if_stmt.cond);
	
	printf("PUSHS bool@false\n");
	printf("JUMPIFEQS $if$%u$else\n", label);

	gen_value_block(node->as.if_stmt.then_block);

	printf("JUMP $if$%u$end\n", label);
	printf("LABEL $if$%u$else\n", label);

	gen_value_block(node->as.if_stmt.else_block);

	printf("LABEL $if$%u$end\n", label);
}

// Stejné jako gen_block ale poslední výraz nechá na zásobníku
static void gen_value_block(ASTNode* block) {
	ASTNode* curr_stmt = block->as.list.head;
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