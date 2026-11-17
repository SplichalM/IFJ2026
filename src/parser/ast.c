#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

static void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Chyba 99: Nedostatek pameti\n");
        exit(99); 
    }
    return ptr;
}

static ASTNode *create_node(NodeType type) {
    ASTNode *node = (ASTNode *)xmalloc(sizeof(ASTNode));
    node->type = type;
    node->data_type = TYPE_UNKNOWN;
    node->unique_name = NULL;
    node->next = NULL;
    return node;
}

ASTNode *ast_create_list(NodeType type) {
    ASTNode *node = create_node(type);
    node->as.list.head = NULL;
    node->as.list.tail = NULL;
    return node;
}

void ast_list_append(ASTNode *list_node, ASTNode *item) {
    if (!list_node || !item) return;

    if (list_node->type != NODE_LIST && 
        list_node->type != NODE_BLOCK && 
        list_node->type != NODE_PROGRAM) {
        return;
    }

    ASTNode *item_tail = item;
    while (item_tail->next) {
        item_tail = item_tail->next;
    }

    if (list_node->as.list.head == NULL) {
        list_node->as.list.head = item;
        list_node->as.list.tail = item_tail;
    } else {
        list_node->as.list.tail->next = item;
        list_node->as.list.tail = item_tail;
    }
}

ASTNode *ast_create_literal_int(int val) {
    ASTNode *node = create_node(NODE_LITERAL);
    node->as.literal.type = LIT_INT;
    node->as.literal.val.i_val = val;
    return node;
}

ASTNode *ast_create_literal_double(double val) {
    ASTNode *node = create_node(NODE_LITERAL);
    node->as.literal.type = LIT_DOUBLE;
    node->as.literal.val.d_val = val;
    return node;
}

ASTNode *ast_create_literal_string(char *val) {
    ASTNode *node = create_node(NODE_LITERAL);
    node->as.literal.type = LIT_STRING;
    node->as.literal.val.s_val = strdup(val);
    return node;
}

ASTNode *ast_create_literal_unit(void) {
    ASTNode *node = create_node(NODE_LITERAL);
    node->as.literal.type = LIT_UNIT;
    return node;
}

ASTNode *ast_create_ident(char *name) {
    ASTNode *node = create_node(NODE_IDENTIFIER);
    node->as.identifier.name = strdup(name);
    return node;
}

ASTNode *ast_create_binary_op(char *op, ASTNode *left, ASTNode *right) {
    ASTNode *node = create_node(NODE_BINARY_OP);
    node->as.op.op = strdup(op);
    node->as.op.left = left;
    node->as.op.right = right;
    return node;
}

ASTNode *ast_create_unary_op(char *op, ASTNode *expr) {
    ASTNode *node = create_node(NODE_UNARY_OP);
    node->as.op.op = strdup(op);
    node->as.op.left = expr;
    node->as.op.right = NULL;
    return node;
}

ASTNode *ast_create_var_decl(char *name, DataType type, ASTNode *value, bool is_mutable) {
    ASTNode *node = create_node(NODE_VAR_DECL);
    node->as.var_decl.name = strdup(name);
    node->as.var_decl.declared_type = type;
    node->as.var_decl.value = value;
    node->as.var_decl.is_mutable = is_mutable;
    return node;
}

ASTNode *ast_create_assign(char *target, ASTNode *value) {
    ASTNode *node = create_node(NODE_ASSIGN);
    node->as.assign.target_name = strdup(target);
    node->as.assign.value = value;
    return node;
}

ASTNode *ast_create_func_param(char *name, DataType type) {
    ASTNode *node = create_node(NODE_FUNC_PARAM);
    node->as.func_param.name = strdup(name);
    node->as.func_param.param_type = type;
    return node;
}

ASTNode *ast_create_func_def(char *name, ASTNode *params, DataType ret_type, ASTNode *body, bool is_main) {
    ASTNode *node = create_node(NODE_FUNC_DEF);
    node->as.func_def.name = strdup(name);
    node->as.func_def.params = params;
    node->as.func_def.return_type = ret_type;
    node->as.func_def.body = body;
    node->as.func_def.is_main = is_main;
    return node;
}

ASTNode *ast_create_func_call(char *name, ASTNode *args) {
    ASTNode *node = create_node(NODE_FUNC_CALL);
    node->as.func_call.name = strdup(name);
    node->as.func_call.args = args;
    return node;
}

ASTNode *ast_create_if(ASTNode *cond, ASTNode *then_block, ASTNode *else_block) {
    ASTNode *node = create_node(NODE_IF);
    node->as.if_stmt.cond = cond;
    node->as.if_stmt.then_block = then_block;
    node->as.if_stmt.else_block = else_block;
    return node;
}

ASTNode *ast_create_while(ASTNode *cond, ASTNode *body) {
    ASTNode *node = create_node(NODE_WHILE);
    node->as.while_stmt.cond = cond;
    node->as.while_stmt.body = body;
    return node;
}

ASTNode *ast_create_return(ASTNode *expr) {
    ASTNode *node = create_node(NODE_RETURN);
    node->as.ret_stmt.expr = expr;
    return node;
}

ASTNode *ast_create_block(ASTNode *stmt_list) {
    ASTNode *node = create_node(NODE_BLOCK);
    node->as.list.head = stmt_list;

    if (stmt_list) {
        ASTNode *curr = stmt_list;
        while (curr->next) {
            curr = curr->next;
        }
        node->as.list.tail = curr;
    } else {
        node->as.list.tail = NULL;
    }

    return node;
}

void ast_free(ASTNode *node) {
    if (!node) return;

    ASTNode *current = node;
    while (current) {
        ASTNode *next_node = current->next;

        switch (current->type) {
            case NODE_PROGRAM:
            case NODE_BLOCK:
            case NODE_LIST:
                ast_free(current->as.list.head);
                break;

            case NODE_VAR_DECL:
                free(current->as.var_decl.name);
                ast_free(current->as.var_decl.value);
                break;

            case NODE_ASSIGN:
                free(current->as.assign.target_name);
                ast_free(current->as.assign.value);
                break;

            case NODE_FUNC_PARAM:
                free(current->as.func_param.name);
                break;

            case NODE_FUNC_DEF:
                free(current->as.func_def.name);
                ast_free(current->as.func_def.params);
                ast_free(current->as.func_def.body);
                break;

            case NODE_FUNC_CALL:
                free(current->as.func_call.name);
                ast_free(current->as.func_call.args);
                break;

            case NODE_IF:
                ast_free(current->as.if_stmt.cond);
                ast_free(current->as.if_stmt.then_block);
                ast_free(current->as.if_stmt.else_block);
                break;

            case NODE_WHILE:
                ast_free(current->as.while_stmt.cond);
                ast_free(current->as.while_stmt.body);
                break;

            case NODE_RETURN:
                ast_free(current->as.ret_stmt.expr);
                break;

            case NODE_BINARY_OP:
                free(current->as.op.op);
                ast_free(current->as.op.left);
                ast_free(current->as.op.right);
                break;

            case NODE_UNARY_OP:
                free(current->as.op.op);
                ast_free(current->as.op.left);
                break;

            case NODE_INT2DOUBLE:
                ast_free(current->as.op.left);
                break;

            case NODE_IDENTIFIER:
                free(current->as.identifier.name);
                break;

            case NODE_LITERAL:
                if (current->as.literal.type == LIT_STRING) {
                    free(current->as.literal.val.s_val);
                }
                break;
        }

        free(current->unique_name);
        free(current);
        current = next_node;
    }
}