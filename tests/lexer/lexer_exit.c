#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int same_text(const char *a, const char *b) {
    if (a == NULL || b == NULL) return a == b;
    return strcmp(a, b) == 0;
}

static int same_token(const Token *a, const Token *b) {
    return a->type == b->type && a->value == b->value &&
           a->float_value == b->float_value && a->length == b->length &&
           same_text(a->context, b->context);
}

int main(void) {
    int status = lexer_init();
    if (status != 0) return status;
    for (;;) {
        Token first = lexer_peek();
        Token second = lexer_peek();
        Token token = lexer_next();
        if (!same_token(&first, &second) || !same_token(&first, &token)) {
            fprintf(stderr, "lexer_peek neodpovida dalsimu tokenu\n");
            free(first.context); free(second.context); free(token.context);
            lexer_free(); return 99;
        }
        free(first.context); free(second.context);
        if (token.type == TOKEN_ERROR || token.type == TOKEN_EOF) {
            status = token.type == TOKEN_ERROR ? (int)token.value : 0;
            free(token.context);
            lexer_free();
            return status;
        }
        free(token.context);
    }
}
