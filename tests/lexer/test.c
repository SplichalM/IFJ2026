#include "lexer.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t size);
void __real_free(void *pointer);

static size_t allocations;
static size_t live_allocations;
static size_t fail_first;
static size_t fail_second;

void *__wrap_malloc(size_t size) {
    allocations++;
    if (allocations == fail_first || allocations == fail_second) {
        return NULL;
    }
    void *pointer = __real_malloc(size);
    if (pointer != NULL) {
        live_allocations++;
    }
    return pointer;
}

void __wrap_free(void *pointer) {
    if (pointer != NULL) {
        assert(live_allocations > 0);
        live_allocations--;
    }
    __real_free(pointer);
}

static void assert_eof(Token token) {
    assert(token.type == TOKEN_EOF);
    assert(token.length == 0);
    assert(token.context == NULL);
}

static void test_lx02_repeated_eof(void) {
    assert(lexer_init() == 0);
    assert_eof(lexer_peek());
    assert_eof(lexer_peek());
    assert_eof(lexer_next());
    assert_eof(lexer_next());
    lexer_free();
    lexer_free();
    assert(live_allocations == 0);
}

static void test_lx02_peek_and_read_eol(void) {
    assert(lexer_init() == 0);
    Token first = lexer_peek();
    Token second = lexer_peek();
    Token third = lexer_next();
    assert(first.type == TOKEN_EOL && second.type == TOKEN_EOL && third.type == TOKEN_EOL);
    assert(first.length == 1 && second.length == 1 && third.length == 1);
    assert(first.value == 2 && second.value == 2 && third.value == 2);
    assert(first.context == NULL && second.context == NULL && third.context == NULL);

    Token next = lexer_next();
    assert(next.type == TOKEN_EOL && next.length == 1 && next.value == 0);
    assert(next.context == NULL);
    assert_eof(lexer_next());
    lexer_free();
    assert(live_allocations == 0);
}

static void test_lx02_init_allocation_failure(size_t failing_allocation) {
    fail_first = failing_allocation;
    assert(lexer_init() == 99);
    assert(live_allocations == 0);
}

static void test_lx02_init_read_failure(void) {
    assert(lexer_init() == 99);
    assert(live_allocations == 0);
}

static void test_lx02_peek_error(int error_text_fails) {
    assert(lexer_init() == 0);
    if (error_text_fails) {
        fail_first = 2;
    }
    Token error = lexer_peek();
    assert(error.type == TOKEN_ERROR);
    if (error_text_fails) {
        assert(error.value == 99);
        assert(error.context == NULL);
    } else {
        assert(error.value == 1);
        assert(error.context != NULL);
        assert(strcmp(error.context, "1") == 0);
    }
    free(error.context);
    lexer_free();
    assert(live_allocations == 0);
}

static void test_lx02_unimplemented_character(void) {
    assert(lexer_init() == 0);
    Token error = lexer_next();
    assert(error.type == TOKEN_ERROR && error.value == 99);
    assert(error.context != NULL && strcmp(error.context, "99") == 0);
    free(error.context);
    lexer_free();
    assert(live_allocations == 0);
}

static void assert_lx03_identifier(Token token, const char *name) {
    assert(token.type == TOKEN_IDENTIFIER);
    assert(token.length == strlen(name));
    assert(token.context != NULL && strcmp(token.context, name) == 0);
}

static void test_lx03_identifiers(void) {
    assert(lexer_init() == 0);
    Token first = lexer_peek();
    Token second = lexer_peek();
    Token read = lexer_next();
    assert_lx03_identifier(first, "val2");
    assert_lx03_identifier(second, "val2");
    assert_lx03_identifier(read, "val2");
    assert(first.context != second.context && first.context != read.context &&
           second.context != read.context);
    free(first.context);
    free(second.context);
    free(read.context);

    assert(lexer_next().type == TOKEN_EOL);
    const char *names[] = {"_x", "x_2", "main", "Val", "int"};
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        Token token = lexer_next();
        assert_lx03_identifier(token, names[i]);
        free(token.context);
    }

    const char *keywords[] = {
        "val", "Int", "if", "while", "def", "do", "Double", "else",
        "import", "return", "String", "then", "Unit", "var"
    };
    for (size_t i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        Token token = lexer_next();
        assert(token.type == TOKEN_KEYWORD);
        assert(token.length == strlen(keywords[i]));
        assert(token.context != NULL && strcmp(token.context, keywords[i]) == 0);
        free(token.context);
    }

    assert_eof(lexer_next());
    lexer_free();
    assert(live_allocations == 0);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    if (strcmp(argv[1], "lx05-integer") == 0) {
        assert(lexer_init() == 0);
        Token token = lexer_next();
        assert(token.type == TOKEN_INT);
        assert(token.value == 1000000);
        assert(strcmp(token.context, "1_000_000") == 0);
        free(token.context);
        token = lexer_next();
        assert(token.type == TOKEN_OPERATOR);
        assert(strcmp(token.context, "-") == 0);
        free(token.context);
        token = lexer_next();
        assert(token.type == TOKEN_INT);
        assert(token.value == 42);
        free(token.context);
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx06-float") == 0) {
        assert(lexer_init() == 0);
        Token token = lexer_peek();
        assert(token.type == TOKEN_DOUBLE);
        assert(token.float_value == 0.5);
        assert(strcmp(token.context, ".5") == 0);
        free(token.context);
        token = lexer_next();
        assert(token.type == TOKEN_DOUBLE);
        assert(token.float_value == 0.5);
        free(token.context);
        token = lexer_next();
        assert(token.type == TOKEN_DOUBLE);
        assert(token.float_value == 125.0);
        assert(strcmp(token.context, "1.25e+2D") == 0);
        free(token.context);
        token = lexer_next();
        assert(token.type == TOKEN_DOUBLE);
        assert(token.float_value == 1000.0);
        free(token.context);
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx07-strings") == 0) {
        assert(lexer_init() == 0);
        Token empty = lexer_next();
        assert(empty.type == TOKEN_STRING);
        assert(empty.length == 2);
        assert(empty.context != NULL);
        assert(empty.context[0] == '\0');
        free(empty.context);

        Token peek = lexer_peek();
        Token token = lexer_next();
        assert(peek.type == TOKEN_STRING);
        assert(token.type == TOKEN_STRING);
        assert(token.length == 13);
        assert(peek.length == token.length);
        assert(token.context != NULL);
        assert(peek.context != NULL);
        assert(peek.context != token.context);
        assert(strcmp(token.context, "a\\\"\n\r\t") == 0);
        assert(strcmp(peek.context, token.context) == 0);
        assert(token.value == 0);
        assert(token.float_value == 0.0);
        free(peek.context);
        free(token.context);

        Token separator = lexer_next();
        assert(separator.type == TOKEN_SEPARATOR);
        assert(separator.context != NULL);
        assert(strcmp(separator.context, ")") == 0);
        free(separator.context);
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx08-strings") == 0) {
        assert(lexer_init() == 0);
        Token empty = lexer_next();
        assert(empty.type == TOKEN_STRING);
        assert(empty.length == 6);
        assert(empty.context != NULL);
        assert(empty.context[0] == '\0');
        free(empty.context);

        Token peek = lexer_peek();
        Token token = lexer_next();
        assert(peek.type == TOKEN_STRING);
        assert(token.type == TOKEN_STRING);
        assert(token.length == 18);
        assert(peek.length == token.length);
        assert(token.context != NULL);
        assert(peek.context != NULL);
        assert(peek.context != token.context);
        assert(strcmp(token.context, "A\n  B\\n \"\"x\"") == 0);
        assert(strcmp(peek.context, token.context) == 0);
        assert(token.value == 0);
        assert(token.float_value == 0.0);
        free(peek.context);
        free(token.context);

        Token eol = lexer_next();
        assert(eol.type == TOKEN_EOL);
        assert(eol.length == 1);
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx05-invalid") == 0 || strcmp(argv[1], "lx06-invalid") == 0 ||
               strcmp(argv[1], "lx07-invalid") == 0 || strcmp(argv[1], "lx08-invalid") == 0) {
        assert(lexer_init() == 0);
        Token token = lexer_next();
        assert(token.type == TOKEN_ERROR);
        assert(token.value == 1);
        free(token.context);
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx02-eof") == 0) {
        test_lx02_repeated_eof();
    } else if (strcmp(argv[1], "lx02-eol") == 0) {
        test_lx02_peek_and_read_eol();
    } else if (strcmp(argv[1], "lx02-init-first") == 0) {
        test_lx02_init_allocation_failure(1);
    } else if (strcmp(argv[1], "lx02-init-grow") == 0) {
        test_lx02_init_allocation_failure(2);
    } else if (strcmp(argv[1], "lx02-init-grow-again") == 0) {
        test_lx02_init_allocation_failure(3);
    } else if (strcmp(argv[1], "lx02-init-read") == 0) {
        test_lx02_init_read_failure();
    } else if (strcmp(argv[1], "lx02-peek-error") == 0) {
        test_lx02_peek_error(0);
    } else if (strcmp(argv[1], "lx02-peek-no-memory") == 0) {
        test_lx02_peek_error(1);
    } else if (strcmp(argv[1], "lx02-unimplemented") == 0) {
        test_lx02_unimplemented_character();
    } else if (strcmp(argv[1], "lx03-identifiers") == 0) {
        test_lx03_identifiers();
    } else if (strcmp(argv[1], "lx03-underscore") == 0 ||
               strcmp(argv[1], "lx03-backtick") == 0 ||
               strcmp(argv[1], "lx04-invalid-symbol") == 0) {
        int result = lexer_init();
        assert(result == 0);
        Token error = lexer_next();
        assert(error.type == TOKEN_ERROR);
        assert(error.value == 1);
        assert(error.context != NULL);
        assert(strcmp(error.context, "1") == 0);
        free(error.context);
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx04-symbols") == 0) {
        int result = lexer_init();
        assert(result == 0);
        const struct {
            TokenType type;
            const char *text;
        } expected[] = {
            {TOKEN_IDENTIFIER, "a"},
            {TOKEN_OPERATOR, "<="},
            {TOKEN_IDENTIFIER, "b"},
            {TOKEN_OPERATOR, "<"},
            {TOKEN_IDENTIFIER, "c"},
            {TOKEN_OPERATOR, "="},
            {TOKEN_IDENTIFIER, "d"},
            {TOKEN_OPERATOR, "=="},
            {TOKEN_IDENTIFIER, "e"},
            {TOKEN_OPERATOR, ">"},
            {TOKEN_OPERATOR, ">="},
            {TOKEN_OPERATOR, "!="},
            {TOKEN_OPERATOR, "+"},
            {TOKEN_OPERATOR, "-"},
            {TOKEN_OPERATOR, "*"},
            {TOKEN_OPERATOR, "/"},
            {TOKEN_KEYWORD, "import"},
            {TOKEN_IDENTIFIER, "ifj26"},
            {TOKEN_SEPARATOR, "."},
            {TOKEN_OPERATOR, "*"},
            {TOKEN_ANNOTATION, "@main"},
            {TOKEN_KEYWORD, "def"},
            {TOKEN_IDENTIFIER, "main"},
            {TOKEN_SEPARATOR, "("},
            {TOKEN_SEPARATOR, ")"},
            {TOKEN_SEPARATOR, ":"},
            {TOKEN_KEYWORD, "Unit"},
            {TOKEN_SEPARATOR, ","},
            {TOKEN_SEPARATOR, ";"},
            {TOKEN_ANNOTATION, "@main"}
        };
        size_t expected_count = sizeof expected / sizeof expected[0];
        for (size_t i = 0; i < expected_count; i++) {
            Token first = lexer_peek();
            Token second = lexer_peek();
            Token read = lexer_next();

            assert(first.type == expected[i].type);
            assert(second.type == first.type);
            assert(read.type == first.type);

            assert(first.length == strlen(expected[i].text));
            assert(second.length == first.length);
            assert(read.length == first.length);

            assert(first.context != NULL);
            assert(second.context != NULL);
            assert(read.context != NULL);
            assert(strcmp(first.context, expected[i].text) == 0);
            assert(strcmp(second.context, first.context) == 0);
            assert(strcmp(read.context, first.context) == 0);

            assert(first.context != second.context);
            assert(first.context != read.context);
            assert(second.context != read.context);
            free(first.context);
            free(second.context);
            free(read.context);
        }
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx04-deferred-token") == 0) {
        int result = lexer_init();
        assert(result == 0);
        Token token = lexer_next();
        assert(token.type != TOKEN_OPERATOR);
        assert(token.type != TOKEN_SEPARATOR);
        free(token.context);
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx09-line") == 0) {
        int result = lexer_init();
        assert(result == 0);
        Token token = lexer_next();
        assert_lx03_identifier(token, "a");
        free(token.context);
        Token first = lexer_peek();
        Token second = lexer_peek();
        token = lexer_next();
        assert(first.type == TOKEN_EOL);
        assert(second.type == TOKEN_EOL);
        assert(token.type == TOKEN_EOL);
        assert(first.value == 2);
        assert(second.value == 2);
        assert(token.value == 2);
        token = lexer_next();
        assert_lx03_identifier(token, "b");
        free(token.context);
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx09-comment-line") == 0) {
        int result = lexer_init();
        assert(result == 0);
        Token token = lexer_next();
        assert_lx03_identifier(token, "a");
        free(token.context);
        token = lexer_next();
        assert(token.type == TOKEN_EOL);
        assert(token.value == 2);
        token = lexer_next();
        assert(token.type == TOKEN_EOL);
        assert(token.value == 4);
        token = lexer_next();
        assert_lx03_identifier(token, "b");
        free(token.context);
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx09-eof") == 0) {
        int result = lexer_init();
        assert(result == 0);
        Token first = lexer_peek();
        Token token = lexer_next();
        assert(first.type == TOKEN_EOL);
        assert(token.type == TOKEN_EOL);
        assert(first.length == token.length);
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx10-nested") == 0) {
        int result = lexer_init();
        assert(result == 0);
        Token token = lexer_next();
        assert_lx03_identifier(token, "a");
        free(token.context);
        Token first = lexer_peek();
        Token second = lexer_peek();
        token = lexer_next();
        assert_lx03_identifier(first, "b");
        assert_lx03_identifier(second, "b");
        assert_lx03_identifier(token, "b");
        free(first.context);
        free(second.context);
        free(token.context);
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx10-unclosed") == 0 ||
               strcmp(argv[1], "lx11-trailing") == 0) {
        int result = lexer_init();
        assert(result == 0);
        Token error = lexer_next();
        assert(error.type == TOKEN_ERROR);
        if (strcmp(argv[1], "lx10-unclosed") == 0) {
            assert(error.value == 1);
        } else {
            assert(error.value == 2);
        }
        free(error.context);
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx12-block-eol") == 0) {
        int result = lexer_init();
        assert(result == 0);
        Token first = lexer_peek();
        Token token = lexer_next();
        assert(first.type == TOKEN_EOL);
        assert(token.type == TOKEN_EOL);
        assert(first.value == 2);
        assert(token.value == 2);
        assert(first.length == token.length);
        token = lexer_next();
        assert_lx03_identifier(token, "b");
        free(token.context);
        assert_eof(lexer_next());
        lexer_free();
        assert(live_allocations == 0);
    } else if (strcmp(argv[1], "lx04-token-no-memory") == 0 ||
               strcmp(argv[1], "lx07-token-no-memory") == 0 ||
               strcmp(argv[1], "lx08-token-no-memory") == 0) {
        int result = lexer_init();
        assert(result == 0);
        fail_first = 2;
        Token error = lexer_peek();
        assert(error.type == TOKEN_ERROR);
        assert(error.value == 99);
        assert(error.context != NULL);
        assert(strcmp(error.context, "99") == 0);
        free(error.context);
        lexer_free();
        assert(live_allocations == 0);
    } else {
        return 2;
    }
    return 0;
}
