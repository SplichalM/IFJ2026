#include "parser.h"
#include "ast.h"
#include "semantic.h"
#include "codegen.h"

int main(void)
{
    symtable_t functions;
    symtable_init(&functions);
    sem_register_builtins(&functions);

    ASTNode *program = parse_program(&functions, register_user_function);
    int status = sem_analyze(program, &functions);

    if (status == 0) {
        gen_program(program);
    }

    symtable_dispose(&functions);
    ast_free(program);
    return status;
}
