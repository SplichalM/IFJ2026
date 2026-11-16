#ifndef STACK_H
#define STACK_H

#include <stddef.h>
#include <stdbool.h>
#include "ast.h"

typedef enum {
    PREC_OPERAND,
    PREC_ADD,
    PREC_MULTIPLY,
    PREC_RELATION,
    PREC_EQUALITY,
    PREC_UNARY_MINUS,
    PREC_LEFT_PAREN,
    PREC_RIGHT_PAREN,
    PREC_END,

    PREC_EXPRESSION,
    PREC_MARKER,
    PREC_INVALID
} PrecedenceSymbol;

typedef struct {
    PrecedenceSymbol symbol;
    ASTNode *node;
    char operator_text[3];
} StackItem;

typedef struct {
    StackItem *items;
    size_t count;
    size_t capacity;
} PrecedenceStack;

void stack_init(PrecedenceStack *stack);
void stack_push(PrecedenceStack *stack, StackItem item);
bool stack_is_empty(const PrecedenceStack *stack);
StackItem stack_top(const PrecedenceStack *stack);
void stack_pop(PrecedenceStack *stack);
void stack_free(PrecedenceStack *stack);
size_t stack_top_terminal_index(const PrecedenceStack *stack);
void stack_insert_marker(PrecedenceStack *stack);
#endif