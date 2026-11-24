#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    const char *texts[] = {"Boolean", "true", "false", "Boolean2", "trueValue", "false_", "!", "!=", "&&", "||"};
    TokenType types[] = {TOKEN_KEYWORD, TOKEN_BOOL, TOKEN_BOOL, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_OPERATOR, TOKEN_OPERATOR, TOKEN_OPERATOR};
    int values[] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
    int result = 0;

    if (lexer_init() != 0) {
        return 99;
    }

    for (size_t i = 0; i < sizeof texts / sizeof texts[0]; i++) {
        Token peek = lexer_peek();
        Token token = lexer_next();

        if (peek.type != types[i] || token.type != types[i]) result = 1;
        if (peek.value != values[i] || token.value != values[i]) result = 1;
        if (peek.float_value != 0.0 || token.float_value != 0.0) result = 1;
        if (peek.length != strlen(texts[i]) || token.length != strlen(texts[i])) result = 1;
        if (peek.context == NULL || token.context == NULL) {
            result = 1;
        } else {
            if (strcmp(peek.context, texts[i]) != 0) result = 1;
            if (strcmp(token.context, texts[i]) != 0) result = 1;
        }
        if (result != 0) {
            fprintf(stderr, "BOOLTHEN token %zu neodpovida\n", i);
        }

        free(peek.context);
        free(token.context);
        if (result != 0) {
            break;
        }
    }

    lexer_free();
    return result;
}
