/*
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 * Soubor: lexer.h
 * Autor: Martin Šplíchal (xsplicm00)
 * Rozhraní lexeru a údaje tokenu
 */

#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>

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
    TOKEN_ANNOTATION,
    TOKEN_BOOL
} TokenType;

typedef struct {
    TokenType type;             // Druh tokenu.
    char *context;              // Kontext tokenu
    int64_t value;              // Hodnota tokenu
	double float_value;         // Hodnota tokenu pro double
	size_t length;              // Delka tokenu
} Token;

// Načte vstup.
int lexer_init(void);

// Vrátí další token a posune se ve vstupu.
Token lexer_next(void);

// Vrátí další token bez posunu ve vstupu.
Token lexer_peek(void);

// Uvolní načtený vstup.
void lexer_free(void);

#endif
