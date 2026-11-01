#ifdefine SEMA_H
#define SEMA_H

#include <stdbool.h>
#include  "symtable.h"
#include "ast.h"

typedef struck sem_scope sem_scope_t;

typedef struck {
	symtable_t *functions;
	symtable_scope_t *top;
	symtable_node_t *curr_func;
	unsigned int count;
} sem_context_t;

void sem_error(int code, const char *msg,);


DataType sem_exp_type(sem_context_t *ctx, ASTNode *node);
DataType sem_binary_op(sem_context_t *ctx, ASTNode *node);
DataType sem_unary_op(sem_context_t *ctx, ASTNode *node);
void sem_insert_conversion(ASTNode **subnode_ptr);
bool sem_type_compatible(DataType target, DataType source);
void sem_check_assignable(sem_context_t *ctx, DataType target, ASTNode **expr_ptr);
void sem_check_conditiona(sem_context_t *ctx, ASTNode **cond_node);

void sem_return(sem_context_t *ctx, ASTNode **ret_node);
DataType sem_block_result(sem_context_t *ctx, ASTNode *block_node);
void sem_check_function_result(sem_context_t *ctx, symtable_node_t *func_node);
void sem_ensure_function_analyzed(sem_context_t *function, symtable_node_t *func_node);

#endif 