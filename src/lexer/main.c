#include "lexer.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *input;
    size_t length;
    size_t position;
} Lexer;

static Lexer lexer;

void lexer_free(void) {
    free(lexer.input);
    lexer.input = NULL;
    lexer.length = 0;
    lexer.position = 0;
}

int lexer_init(void) {
    lexer_free();

    size_t capacity = 16;
    lexer.input = malloc(capacity);
    if (lexer.input == NULL) {
        return 99;
    }

    int character;
    while ((character = fgetc(stdin)) != EOF) {
        if (lexer.length == capacity) {
            if (capacity > SIZE_MAX / 2) {
                lexer_free();
                return 99;
            }

            size_t new_capacity = capacity * 2;
            char *new_input = malloc(new_capacity);
            if (new_input == NULL) {
                lexer_free();
                return 99;
            }
            memcpy(new_input, lexer.input, lexer.length);
            free(lexer.input);
            lexer.input = new_input;
            capacity = new_capacity;
        }

        lexer.input[lexer.length] = (char)character;
        lexer.length++;
    }

    if (ferror(stdin)) {
        lexer_free();
        return 99;
    }

    return 0;
}

static Token lexer_internal_error(void) {
    Token token = {0};
    token.type = TOKEN_ERROR;
    token.value = 99;
    token.context = malloc(3);
    if (token.context != NULL) {
        memcpy(token.context, "99", 3);
    }
    return token;
}

Token lexer_next(void) {
    if (lexer.input == NULL) {
        return lexer_internal_error();
    }

    while (lexer.position < lexer.length &&
           (lexer.input[lexer.position] == ' ' ||
            lexer.input[lexer.position] == '\t' ||
            lexer.input[lexer.position] == '\r')) {
        lexer.position++;
    }

    if (lexer.position == lexer.length) {
        Token token = {0};
        token.type = TOKEN_EOF;
        return token;
    }

    if (lexer.input[lexer.position] == '\n') {
        size_t next = lexer.position + 1;
        size_t indentation = 0;
        while (next < lexer.length && lexer.input[next] == ' ') {
            if (indentation == INT_MAX) {
                return lexer_internal_error();
            }
            indentation++;
            next++;
        }

        char *text = malloc(2);
        if (text == NULL) {
            return lexer_internal_error();
        }
        text[0] = '\n';
        text[1] = '\0';

        Token token = {0};
        token.type = TOKEN_EOL;
        token.context = text;
        token.length = 1;
        token.value = (int)indentation;
        lexer.position++;
        return token;
    }

    return lexer_internal_error();
}

Token lexer_peek(void) {
    Token token = lexer_next();
    if (token.type != TOKEN_ERROR) {
        lexer.position -= token.length;
    }
    return token;
}
