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

static void test_eof(void) {
    assert(lexer_init() == 0);
    assert_eof(lexer_peek());
    assert_eof(lexer_peek());
    assert_eof(lexer_next());
    assert_eof(lexer_next());
    lexer_free();
    lexer_free();
    assert(live_allocations == 0);
}

static void test_eol(void) {
    assert(lexer_init() == 0);
    Token first = lexer_peek();
    Token second = lexer_peek();
    Token third = lexer_next();
    assert(first.type == TOKEN_EOL && second.type == TOKEN_EOL && third.type == TOKEN_EOL);
    assert(first.length == 1 && second.length == 1 && third.length == 1);
    assert(first.value == 2 && second.value == 2 && third.value == 2);
    assert(first.context != NULL && second.context != NULL && third.context != NULL);
    assert(first.context != second.context && first.context != third.context);
    assert(strcmp(first.context, "\n") == 0);
    assert(strcmp(second.context, "\n") == 0);
    assert(strcmp(third.context, "\n") == 0);
    free(first.context);
    free(second.context);
    free(third.context);

    Token next = lexer_next();
    assert(next.type == TOKEN_EOL && next.length == 1 && next.value == 0);
    assert_eof(lexer_next());
    lexer_free();
    assert(strcmp(next.context, "\n") == 0);
    free(next.context);
    assert(live_allocations == 0);
}

static void test_init_failure(size_t failing_allocation) {
    fail_first = failing_allocation;
    assert(lexer_init() == 99);
    assert(live_allocations == 0);
}

static void test_init_read_failure(void) {
    assert(lexer_init() == 99);
    assert(live_allocations == 0);
}

static void test_peek_failure(int error_text_fails) {
    assert(lexer_init() == 0);
    fail_first = 2;
    if (error_text_fails) {
        fail_second = 3;
    }
    Token error = lexer_peek();
    assert(error.type == TOKEN_ERROR && error.value == 99);
    if (error_text_fails) {
        assert(error.context == NULL);
    } else {
        assert(error.context != NULL && strcmp(error.context, "99") == 0);
    }
    free(error.context);
    lexer_free();
    assert(live_allocations == 0);
}

static void test_unimplemented(void) {
    assert(lexer_init() == 0);
    Token error = lexer_next();
    assert(error.type == TOKEN_ERROR && error.value == 99);
    assert(error.context != NULL && strcmp(error.context, "99") == 0);
    free(error.context);
    lexer_free();
    assert(live_allocations == 0);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    if (strcmp(argv[1], "eof") == 0) {
        test_eof();
    } else if (strcmp(argv[1], "eol") == 0) {
        test_eol();
    } else if (strcmp(argv[1], "init-first") == 0) {
        test_init_failure(1);
    } else if (strcmp(argv[1], "init-grow") == 0) {
        test_init_failure(2);
    } else if (strcmp(argv[1], "init-grow-again") == 0) {
        test_init_failure(3);
    } else if (strcmp(argv[1], "init-read") == 0) {
        test_init_read_failure();
    } else if (strcmp(argv[1], "peek-error") == 0) {
        test_peek_failure(0);
    } else if (strcmp(argv[1], "peek-no-memory") == 0) {
        test_peek_failure(1);
    } else if (strcmp(argv[1], "unimplemented") == 0) {
        test_unimplemented();
    } else {
        return 2;
    }
    return 0;
}
