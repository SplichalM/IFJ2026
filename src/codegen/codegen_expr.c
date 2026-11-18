#include <stdio.h>
#include <string.h>
#include "codegen_expr.h"


static void gen_push_string(const char* str) {
    printf("PUSHS string@");
    if (str != NULL) {
        unsigned char c;
        while ((c = (unsigned char)*str++) != '\0') {
            
            if (c <= 32 || c == 35 || c == 92) {
                printf("\\%03d", c);
            } else {
                putchar(c);
            }
        }
    }
    printf("\n");
}

void gen_expr(ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_LITERAL:
            switch (node->as.literal.type) {
                case LIT_INT:
                    printf("PUSHS int@%lld\n", node->as.literal.val.i_val);
                    break;
                case LIT_DOUBLE:
                    printf("PUSHS float@%a\n", node->as.literal.val.d_val);
                    break;
                case LIT_STRING:
                    gen_push_string(node->as.literal.val.s_val);
                    break;
                case LIT_UNIT:
                    printf("PUSHS nil@nil\n");
                    break;
            }
            break;

        case NODE_IDENTIFIER: {
            const char* name = node->unique_name;
            if (name == NULL) {
                name = node->as.identifier.name;
            }
            printf("PUSHS LF@%s\n", name);
            break;
        }

        case NODE_INT2DOUBLE:
            gen_expr(node->as.op.left);
            puts("INT2FLOATS");
            break;
        
        case NODE_BINARY_OP:
            gen_expr(node->as.op.left);
            gen_expr(node->as.op.right);
            
            if (strcmp(node->as.op.op, "+") == 0){
                if (node->data_type == TYPE_STRING) {
                    printf("POPS GF@%%tmp2\n");
                    printf("POPS GF@%%tmp1\n");
                    printf("CONCAT GF@%%tmp1 GF@%%tmp1 GF@%%tmp2\n");  
                    printf("PUSHS GF@%%tmp1\n");
                } else {
                    printf("ADDS\n");
                }
            } else if (strcmp(node->as.op.op, "-") == 0) { 
                    printf("SUBS\n");
            } else if (strcmp(node->as.op.op, "*") == 0) {
                    printf("MULS\n");
            } else if (strcmp(node->as.op.op, "/") == 0) {
                if (node->data_type == TYPE_INT) {
                    printf("IDIVS\n");
                }
                else {
                    printf("DIVS\n");
                }
            } else if (strcmp(node->as.op.op, "==") == 0) {
                    printf("EQS\n");
            } else if (strcmp(node->as.op.op, "!=") == 0) {
                    printf("EQS\n");
                    printf("NOTS\n");
            } else if (strcmp(node->as.op.op, "<") == 0) {
                    printf("LTS\n");
            } else if (strcmp(node->as.op.op, ">=") == 0) {
                    printf("LTS\n");
                    printf("NOTS\n");
            } else if (strcmp(node->as.op.op, ">") == 0) {
                    printf("GTS\n");
            } else if (strcmp(node->as.op.op, "<=") == 0) {
                    printf("GTS\n");
                    printf("NOTS\n");
            }

            break;

        case NODE_UNARY_OP:
            if (strcmp (node->as.op.op, "-") == 0) {
                if (node->data_type == TYPE_INT) {
                    printf("PUSHS int@0\n");
                }else{
                    printf("PUSHS float@0x0p+0\n");
                }
                gen_expr(node->as.op.left);
                printf("SUBS\n");
            } else if (strcmp(node->as.op.op, "+") == 0) {
                gen_expr(node->as.op.left);
            }
            break;

        case NODE_FUNC_CALL: {
            const char* func_name = node->as.func_call.name;
            ASTNode* args_list = node->as.func_call.args;
            ASTNode* arg = NULL;
            if (args_list != NULL) {
                arg = args_list->as.list.head;
            }

            static int local_label_counter = 0; 
            int l_id = local_label_counter++;

            if (strcmp(func_name, "print") == 0) {
                gen_expr(arg);
                if (arg->data_type == TYPE_UNIT) {
                    printf("POPS GF@%%trash\n");
                    printf("WRITE string@()\n");
                } else {
                    printf("POPS GF@%%tmp1\n");
                    printf("WRITE GF@%%tmp1\n");
                }
                printf("PUSHS nil@nil\n"); 
            }
            else if (strcmp(func_name, "readLine") == 0) {
                printf("READ GF@%%tmp1 string\n");
                printf("JUMPIFNEQ $read_ok$%d GF@%%tmp1 nil@nil\n", l_id);
                printf("MOVE GF@%%tmp1 string@\n"); 
                printf("LABEL $read_ok$%d\n", l_id);
                printf("PUSHS GF@%%tmp1\n");
            }
            else if (strcmp(func_name, "readInt") == 0) {
                printf("READ GF@%%tmp1 int\n");
                printf("JUMPIFNEQ $read_ok$%d GF@%%tmp1 nil@nil\n", l_id);
                printf("MOVE GF@%%tmp1 int@0\n"); 
                printf("LABEL $read_ok$%d\n", l_id);
                printf("PUSHS GF@%%tmp1\n");
            }
            else if (strcmp(func_name, "readDouble") == 0) {
                printf("READ GF@%%tmp1 float\n");
                printf("JUMPIFNEQ $read_ok$%d GF@%%tmp1 nil@nil\n", l_id);
                printf("MOVE GF@%%tmp1 float@0x0p+0\n");
                printf("LABEL $read_ok$%d\n", l_id);
                printf("PUSHS GF@%%tmp1\n");
            }
            else if (strcmp(func_name, "toStr") == 0) {
                gen_expr(arg);
                if (arg->data_type == TYPE_INT) {
                    printf("INT2STRS\n");
                } else if (arg->data_type == TYPE_DOUBLE) {
                    printf("FLOAT2STRS\n");
                } else if (arg->data_type == TYPE_UNIT) {
                    printf("POPS GF@%%trash\n");
                    printf("PUSHS string@()\n");
                }
            }
            // TODO: Je potreba dopsat comentaře.
            else if (strcmp(func_name, "toInt") == 0) {
                gen_expr(arg);
                puts("FLOAT2INTS");
            }
            else if (strcmp(func_name, "strLength") == 0) {
                gen_expr(arg);
                puts("POPS GF@%tmp1");
                puts("STRLEN GF@%tmp2 GF@%tmp1");
                puts("PUSHS GF@%tmp2");
            }
            else if (strcmp(func_name, "chr") == 0) {
                gen_expr(arg);
                puts("INT2CHARS");
            }
            else if (strcmp(func_name, "ord") == 0) {
                gen_expr(arg);
                gen_expr(arg->next);

                puts("POPS GF@%tmp2");
                puts("POPS GF@%tmp1");

                puts("LT GF@%tmp3 GF@%tmp2 int@0");
                printf("JUMPIFEQ $builtin$ord$invalid$%d GF@%%tmp3 bool@true\n", l_id);

                puts("STRLEN GF@%tmp3 GF@%tmp1");
                puts("LT GF@%tmp3 GF@%tmp2 GF@%tmp3");
                printf("JUMPIFEQ $builtin$ord$invalid$%d GF@%%tmp3 bool@false\n", l_id);

                puts("STRI2INT GF@%tmp3 GF@%tmp1 GF@%tmp2");
                printf("JUMP $builtin$ord$done$%d\n", l_id);
                printf("LABEL $builtin$ord$invalid$%d\n", l_id);
                puts("MOVE GF@%tmp3 int@-1");
                printf("LABEL $builtin$ord$done$%d\n", l_id);
                puts("PUSHS GF@%tmp3");
            }
            else if (strcmp(func_name, "subStr") == 0) {
                for (ASTNode* current = arg; current != NULL; current = current->next) {
                    gen_expr(current);
                }
                puts("CALL $builtin$subStr");
            }
            else {
            
                ASTNode* curr_arg = arg;
                while (curr_arg) {
                    gen_expr(curr_arg);
                    curr_arg = curr_arg->next;
                }
            
                printf("CALL $fn$%s\n", func_name);
            }
            break;
        }

        default:
            fprintf(stderr, "Chyba generátoru: Neznamy uzel ve vyrazu\n");
            break;
    }
}

void gen_builtin_helpers(void) {
    fputs("LABEL $builtin$subStr\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@text\n"
          "DEFVAR LF@start\n"
          "DEFVAR LF@end\n"
          "DEFVAR LF@length\n"
          "DEFVAR LF@check\n"
          "DEFVAR LF@character\n"
          "DEFVAR LF@result\n"
          "POPS LF@end\n"
          "POPS LF@start\n"
          "POPS LF@text\n"
          "STRLEN LF@length LF@text\n", stdout);

    fputs("LT LF@check LF@start int@0\n"
          "JUMPIFEQ $builtin$subStr$invalid LF@check bool@true\n"
          "LT LF@check LF@end int@0\n"
          "JUMPIFEQ $builtin$subStr$invalid LF@check bool@true\n"
          "GT LF@check LF@start LF@end\n"
          "JUMPIFEQ $builtin$subStr$invalid LF@check bool@true\n"
          "LT LF@check LF@start LF@length\n"
          "JUMPIFEQ $builtin$subStr$invalid LF@check bool@false\n"
          "GT LF@check LF@end LF@length\n"
          "JUMPIFEQ $builtin$subStr$invalid LF@check bool@true\n", stdout);

    fputs("MOVE LF@result string@\n"
          "LABEL $builtin$subStr$loop\n"
          "JUMPIFEQ $builtin$subStr$done LF@start LF@end\n"
          "GETCHAR LF@character LF@text LF@start\n"
          "CONCAT LF@result LF@result LF@character\n"
          "ADD LF@start LF@start int@1\n"
          "JUMP $builtin$subStr$loop\n"
          "LABEL $builtin$subStr$done\n"
          "PUSHS LF@result\n"
          "POPFRAME\n"
          "RETURN\n"
          "LABEL $builtin$subStr$invalid\n"
          "EXIT int@40\n", stdout);
}
