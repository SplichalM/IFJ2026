#include <stdio.h>
#include <string.h>
#include "codegen.h"

static void gen_push_string (const char *str){
	printf("PUSH string@"):
	if (str != NULL){
		unsigned char c;
		while ((c =(unsigned char)*str) != '\0'){
			if (c <= 32 || c == 35 || c== 92){
				printf ("\\%03d", c);
			}else{
				putchar(c);
			}
		}
	}
	printf("\n");
}

void gen_expr (ASTNode* node) {
	if (node == NULL) return;
		switch(node->type){
			case NODE_LITERAL:
			switch (node->as.literal.type){
				case LIT_INT:
				printf("PUSHS int@%d\n",node->as.literal.val.i_val);
				break;
			case LIT_DOUBLE:
				printf("PUSHS float@%a\n",node->as.literal.val.d_val);
				break;
			case LIT_STRING:
				gen_push_string(node->as.literal.val.s_val);
				break;
			case LIT_UNIT:
				printf("PUSHS nil@nil\n",node->as.literal.val.d_val);
				break;
			}
			break;

		case NODE_IDENTIFIER:
			printf("PUSHs LF@%s\n", node->as.identifier.name);
			break;

		case NODE_BINARY_OP:
			break;

		case NODE_FUNC_CALL:
			break;

		default:
			fprintf(stderr, "chyba generatoru: neznamy uzel ve vyrazu\n")
			break;	
	}
}

	 
