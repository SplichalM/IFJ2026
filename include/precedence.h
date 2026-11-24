// Autor: Jan Pospíšil (xpospij01)
#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "ast.h"
#include "lexer.h"
#include <stdbool.h>

typedef enum {
    ACTION_SHIFT,
    ACTION_EQUAL,
    ACTION_REDUCE,
    ACTION_ERROR,
    ACTION_ACCEPT
} PrecedenceAction;

ASTNode *parse_precedence_expression(Token *current, int (*advance)(void), size_t line_indent, bool (*starts_function_call)(void), ASTNode *(*parse_function_call)(void));
#endif