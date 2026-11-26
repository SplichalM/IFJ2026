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

ASTNode *parse_precedence_expression(Token *current, int (*advance)(void), size_t line_indent, bool (*starts_function_call)(void), void (*parse_function_call)(ASTNode **), void (*parse_if_expression)(ASTNode **), const char *const *stop_words);
void precedence_free_pending(void);
#endif