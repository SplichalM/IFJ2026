#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_INT,
    TOKEN_DOUBLE,
    TOKEN_STRING,
    TOKEN_OPERATOR,
    TOKEN_SEPARATOR,
    TOKEN_EOL,
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_ANNOTATION
} TokenType;

typedef struct {
    TokenType type;             // Druh tokenu.
    char *context;              // Kontext tokenu
    int value;                  // Hodnota tokenu
	double float_value;         // Hodnota tokenu pro double
	size_t length;              // Delka tokenu
} Token;

int lexer_init(void);
Token lexer_next(void);
Token lexer_peek(void);
void lexer_free(void);

#endif
