#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    const char *texts[] = {
        "for", "forEach", "for_", "to", "until", "by", "i", "<-", "1", "<", "<=", "<", "-"
    };
    const TokenType types[] = {
        TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER,
        TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_OPERATOR,
        TOKEN_INT, TOKEN_OPERATOR, TOKEN_OPERATOR, TOKEN_OPERATOR, TOKEN_OPERATOR
    };

    int result = lexer_init();
    if (result != 0) {
        return result;
    }

    for (size_t i = 0; i < sizeof texts / sizeof texts[0]; i++) {
        Token token = lexer_next();
        if (token.type != types[i]) {
            result = 1;
        }
        if (token.length != strlen(texts[i])) {
            result = 1;
        }
        if (token.context == NULL || strcmp(token.context, texts[i]) != 0) {
            result = 1;
        }
        free(token.context);

        if (result != 0) {
            fprintf(stderr, "CONTROL: ocekavan token %s\n", texts[i]);
            break;
        }
    }

    lexer_free();
    return result;
}
