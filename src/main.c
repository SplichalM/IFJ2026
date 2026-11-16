#include "parser.h"
#include "ast.h"
#include <stddef.h>

#ifdef IFJ_ENABLE_SEMANTICS
#include "semantic.h"
#endif

int main(void)
{
#ifdef IFJ_ENABLE_SEMANTICS
    symtable_t functions;
    symtable_init(&functions);
    sem_register_builtins(&functions);

    ASTNode *program = parse_program(&functions, register_user_function);
    int status = sem_analyze(program, &functions);

    symtable_dispose(&functions);
#else
    ASTNode *program = parse_program(NULL, NULL);
    int status = 0;
#endif

    ast_free(program);
    return status;
}
