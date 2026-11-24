#include <stdio.h>
#include <string.h>
#include "builtins_p.h"
#include <stdbool.h>
#include "codegen.h"


static void gen_read_number(const char *type, const char *fallback)
{
    unsigned int label = gen_new_label();

    fprintf(stdout,
            "READ GF@%%tmp1 %s\n"
            "JUMPIFNEQ $builtin$read$%u GF@%%tmp1 nil@nil\n"
            "MOVE GF@%%tmp1 %s\n"
            "LABEL $builtin$read$%u\n"
            "PUSHS GF@%%tmp1\n",
            type, label, fallback, label);
}

static void gen_print(void)
{
    unsigned int label = gen_new_label();
    fprintf(stdout,
            "POPS GF@%%tmp1\n"
            "JUMPIFNEQ $builtin$print$%u GF@%%tmp1 nil@nil\n"
            "MOVE GF@%%tmp1 string@()\n"
            "LABEL $builtin$print$%u\n"
            "WRITE GF@%%tmp1\n"
            "PUSHS nil@nil\n",
            label, label);
}

static void gen_to_str(){
    unsigned int label = gen_new_label();
    fprintf(stdout,
            "POPS GF@%%tmp1\n"
            "TYPE GF@%%tmp2 GF@%%tmp1\n"
            "JUMPIFNEQ $builtin$toStr$checkFloat$%u GF@%%tmp2 string@int\n"
            "INT2STR GF@%%tmp1 GF@%%tmp1\n"
            "JUMP $builtin$toStr$end$%u\n"
            "LABEL $builtin$toStr$checkFloat$%u\n"
            "JUMPIFNEQ $builtin$toStr$checkUnit$%u GF@%%tmp2 string@float\n"
            "FLOAT2STR GF@%%tmp1 GF@%%tmp1\n"
            "JUMP $builtin$toStr$end$%u\n"
            "LABEL $builtin$toStr$checkUnit$%u\n"
            "JUMPIFNEQ $builtin$toStr$end$%u GF@%%tmp2 string@nil\n"
            "MOVE GF@%%tmp1 string@()\n"
            "LABEL $builtin$toStr$end$%u\n"
            "PUSHS GF@%%tmp1\n",
            label, label, label, label, label, label, label, label);
}
bool gen_builtin_call(const char *name)
{
    if (strcmp(name, "readLine") == 0) {
        gen_read_number("string", "string@");
        return true;
    }

    if (strcmp(name, "readInt") == 0) {
        gen_read_number("int", "int@0");
        return true;
    }

    if (strcmp(name, "readDouble") == 0) {
        gen_read_number("float", "float@0x0p+0");
        return true;
    }

    if (strcmp(name, "print") == 0) {
        gen_print();
        return true;
    }

    if (strcmp(name, "toStr") == 0) {
        gen_to_str();
        return true;
    }

    return false;
}