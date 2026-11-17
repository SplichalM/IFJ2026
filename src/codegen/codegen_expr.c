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
                    printf("PUSHS int@%d\n", node->as.literal.val.i_val);
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

        case NODE_IDENTIFIER:
            printf("PUSHS LF@%s\n", node->as.identifier.name);
            break;
        
        case NODE_BINARY_OP:
            gen_expr(node->as.binary_op.left);
            gen_expr(node->as.binary_op.right);
            
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
                    printf("DIVS\n");
            } else if (strcmp(node->as.op.op, "==") == 0) {
                    printf("EQS\n");
            } else if (strcmp(node->as.op.op, "!=") == 0) {
                    printf("EQS\n");
                    printf("NOTS\n");
            } else if (strcmp(node->as.op.op, "<") == 0) {
                    printf("LTS\n");
            } else if (strcmp(node->as.op.op, "<=") == 0) {
                    printf("LTS\n");
                    printf("NOTS\n");
            } else if (strcmp(node->as.op.op, ">") == 0) {
                    printf("GTS\n");
            } else if (strcmp(node->as.op.op, ">=") == 0) {
                    printf("GTS\n");
                    printf("NOTS\n");
            }

            break;

        case NODE_UNNARY_OP:
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
            ASTNode* arg = (args_list != NULL) ? args_list->as.list.head : NULL;

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
            else if (strcmp(func_name, "toInt") == 0) {
                gen_expr(arg);
                printf("FLOAT2INTS\n"); 
            }
            else if (strcmp(func_name, "strLength") == 0) {
                gen_expr(arg);
                printf("POPS GF@%%tmp1\n");
                printf("STRLEN GF@%%tmp1 GF@%%tmp1\n");
                printf("PUSHS GF@%%tmp1\n");
            }
            else if (strcmp(func_name, "chr") == 0) {
                gen_expr(arg);
                printf("INT2CHARS\n");
            }
            else if (strcmp(func_name, "ord") == 0) {
                gen_expr(arg); 
                gen_expr(arg->next); 
                
                printf("POPS GF@%%tmp2\n");
                printf("POPS GF@%%tmp1\n");
                printf("STRLEN GF@%%tmp3 GF@%%tmp1\n"); 
                
                printf("PUSHS GF@%%tmp2\n");
                printf("PUSHS int@0\n");
                printf("LTS\n");
                
                printf("PUSHS GF@%%tmp2\n");
                printf("PUSHS GF@%%tmp3\n");
                printf("LTS\n");
                printf("NOTS\n");
                
                printf("ORS\n");
                printf("PUSHS bool@true\n");
                printf("JUMPIFEQS $ord_out_of_bounds$%d\n", l_id);
                
                printf("STRI2INT GF@%%tmp1 GF@%%tmp1 GF@%%tmp2\n");
                printf("PUSHS GF@%%tmp1\n");
                printf("JUMP $ord_end$%d\n", l_id);
               
                printf("LABEL $ord_out_of_bounds$%d\n", l_id);
                printf("PUSHS int@-1\n");
                printf("LABEL $ord_end$%d\n", l_id);
            }
            else if (strcmp(func_name, "subStr") == 0) {
                ASTNode* curr_arg = arg;
                while (curr_arg) {
                    gen_expr(curr_arg);
                    curr_arg = curr_arg->next;
                }
                printf("CALL $builtin$subStr\n");
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
    printf("\n# --- Pomocna funkce pro subStr ---\n");
    printf("LABEL $builtin$subStr\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    // Vytvoøení lokálních promìnných v rámci funkce
    printf("DEFVAR LF@s\n");
    printf("DEFVAR LF@start\n");
    printf("DEFVAR LF@end\n");
    printf("DEFVAR LF@res\n");
    printf("DEFVAR LF@len\n");
    printf("DEFVAR LF@char\n");

    // Výbìr argumentù ze zásobníku
    printf("POPS LF@end\n");
    printf("POPS LF@start\n");
    printf("POPS LF@s\n");

    printf("MOVE LF@res string@\n"); // Inicializace výsledku na prázdný øetìzec
    printf("STRLEN LF@len LF@s\n");  // Zjištìní délky pùvodního øetìzce

    // --- 1. SÉRIE KONTROL (zastaví s chybou 40, pokud indexy nesedí) ---
    // start < 0
    printf("PUSHS LF@start\n");
    printf("PUSHS int@0\n");
    printf("LTS\n");

    // end < 0
    printf("PUSHS LF@end\n");
    printf("PUSHS int@0\n");
    printf("LTS\n");
    printf("ORS\n"); // Spojíme pøedchozí kontrolu

    // start > end
    printf("PUSHS LF@start\n");
    printf("PUSHS LF@end\n");
    printf("GTS\n");
    printf("ORS\n");

    // start >= len -> využijeme negaci (start < len)
    printf("PUSHS LF@start\n");
    printf("PUSHS LF@len\n");
    printf("LTS\n");
    printf("NOTS\n");
    printf("ORS\n");

    // end > len
    printf("PUSHS LF@end\n");
    printf("PUSHS LF@len\n");
    printf("GTS\n");
    printf("ORS\n");

    // Pokud platí KTERÁKOLIV z chybových podmínek výše, jdeme na chybové návìští
    printf("PUSHS bool@true\n");
    printf("JUMPIFEQS $substr_error\n");

    // --- 2. CYKLUS KOPÍROVÁNÍ (od 'start' do 'end') ---
    printf("LABEL $substr_loop\n");
    
    // Podmínka cyklu: pokud (start < end) == false, vyskoè z cyklu
    printf("PUSHS LF@start\n");
    printf("PUSHS LF@end\n");
    printf("LTS\n");
    printf("PUSHS bool@false\n");
    printf("JUMPIFEQS $substr_end\n"); 

    // Naètení jednoho znaku na indexu 'start' a konkatenace k výsledku
    printf("GETCHAR LF@char LF@s LF@start\n");
    printf("CONCAT LF@res LF@res LF@char\n");

    // Inkrementace (start = start + 1)
    printf("ADD LF@start LF@start int@1\n");
    printf("JUMP $substr_loop\n");

    // --- 3. KONEC S ÚSPÌCHEM ---
    printf("LABEL $substr_end\n");
    printf("PUSHS LF@res\n"); // Vrácení jednoho výsledku na zásobník
    printf("POPFRAME\n");     // Úklid lokálního rámce
    printf("RETURN\n");

    // --- 4. KONEC S CHYBOU ---
    printf("LABEL $substr_error\n");
    printf("EXIT int@40\n"); // Ukonèení celého interpretu s chybou
    printf("# --- Konec pomocne funkce ---\n\n");
}