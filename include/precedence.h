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

// token a funkce, přes které precedence spolupracuje s parserem.
typedef struct {
    Token *current;
    int (*advance)(void);
    bool (*identifier_followed_by)(TokenType type, const char *text);
    void (*parse_function_call)(ASTNode **);
    void (*parse_if_expression)(ASTNode **);
} ExpressionInput;

ASTNode *parse_precedence_expression(
    const ExpressionInput *input,
    size_t line_indent,
    const char *const *stop_words
);
void precedence_free_pending(void);
#endif