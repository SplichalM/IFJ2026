#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
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
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;           // Druh tokenu.
    const char *value;        // Hodnota tokenu, pokud ji ma.
    const char *content;      // Puvodni text tokenu ve vstupu.
    size_t length;            // Delka puvodniho textu v bajtech.
    size_t line;              // Radek zacatku tokenu, cislovany od 1.
    size_t column;            // Sloupec zacatku tokenu, cislovany od 1.
    size_t indentation;       // Pocet mezer na zacatku logickeho radku.
    bool starts_logical_line; // Zda token zacina logicky radek.
    bool newline_before;      // Zda pred tokenem nastal fyzicky konec radku.
} Token;

int lexer_init(void);
int lexer_next(Token *token);
int lexer_peek(Token *token);
void lexer_free(void);

#endif
