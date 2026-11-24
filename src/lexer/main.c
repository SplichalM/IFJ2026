#include "lexer.h"

#include <limits.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
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

// Uvolneni pameti
void lexer_free(void) {
    free(lexer.input);
    lexer.input = NULL;
    lexer.length = 0;
    lexer.position = 0;
}

// Inicializace lexeru a nacteni vstupu ze standardniho vstupu
int lexer_init(void) {
    lexer_free();

	// Inicializace vstupu (zacinam na 16ti bajtech)
    size_t capacity = 16;
    lexer.input = malloc(capacity);
    if (lexer.input == NULL) {
        return 99;
    }

    // Nacte prvni znak
    int character = fgetc(stdin);
    while (character != EOF) {
		// Pokud je delka bufferu rovna kapacite, zvetsime buffer
        if (lexer.length == capacity) {
			// Kontrola maximalni velikosti systemove pameti pri zvetsovani bufferu
            if (capacity > SIZE_MAX / 2) {
                lexer_free();
                return 99;
            }

            // Vytvoreni noveho bufferu
            size_t new_capacity = capacity * 2;
            char *new_input = malloc(new_capacity);
            if (new_input == NULL) {
                lexer_free();
                return 99;
            }

            // Kopirovani obsahu
            memcpy(new_input, lexer.input, lexer.length);
            free(lexer.input);
            lexer.input = new_input;
            capacity = new_capacity;
        }

		// Prevod znaku z int v rozmezi 0-255 + EOF (-1) na char v rozmezi -128-127
        char input_character = (char)character;
        lexer.input[lexer.length] = input_character;
        lexer.length++;
        character = fgetc(stdin);
    }

	// kontrola chyby pri cteni ze standardniho vstupu
    if (ferror(stdin)) {
        lexer_free();
        return 99;
    }

    return 0;
}

// Vytvoreni tokenu pro chybu
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

// Kontrola, zda znak muze byt zacatkem identifikatoru
static int is_identifier_start(char character) {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z') || character == '_';
}

// Kontrola, zda znak muze byt soucasti identifikatoru, ale ne zacatkem
static int is_identifier_part(char character) {
    return is_identifier_start(character) || (character >= '0' && character <= '9');
}

// Kontrola, zda je kontext klicovym slovem
static int is_keyword(const char *context) {
    static const char *const keywords[] = {
        "def", "do", "Double", "else", "for", "if", "import", "Int", "return",
        "String", "then", "Unit", "val", "var", "while"
    };
    for (size_t i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        if (strcmp(context, keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Rozpozna Boolean, true a false
static int check_bool_word(Token *token) {
    if (strcmp(token->context, "Boolean") == 0) {
        token->type = TOKEN_KEYWORD;
        return 1;
    }
    if (strcmp(token->context, "true") == 0) {
        token->type = TOKEN_BOOL;
        token->value = 1;
        return 1;
    }
    if (strcmp(token->context, "false") == 0) {
        token->type = TOKEN_BOOL;
        token->value = 0;
        return 1;
    }
    return 0;
}

// Nacteni identifikatoru nebo klicoveho slova
static Token read_identifier_or_keyword(void) {
	// Pocitani delky identifikatoru/klicoveho slova
    size_t start = lexer.position;
    size_t end = start + 1;
    while (end < lexer.length && is_identifier_part(lexer.input[end])) {
        end++;
    }

	// Pocitani delky tokenu a kontrola preteceni
    size_t length = end - start;
    if (length == SIZE_MAX) {
        return lexer_internal_error();
    }

    if (length == 1 && lexer.input[start] == '_') {
        Token error;
        error.type = TOKEN_ERROR;
        error.value = 99;
        error.float_value = 0.0;
        error.length = 0;
        error.context = malloc(2);
        if (error.context != NULL) {
            error.context[0] = '1';
            error.context[1] = '\0';
            error.value = 1;
        }
        return error;
    }

	// Vytvoreni tokenu
    Token token = {0};
    token.context = malloc(length + 1);
    if (token.context == NULL) {
        return lexer_internal_error();
    }

    memcpy(token.context, lexer.input + start, length);
    token.context[length] = '\0';
    if (!check_bool_word(&token)) {
        if (is_keyword(token.context)) {
            token.type = TOKEN_KEYWORD;
        } else {
            token.type = TOKEN_IDENTIFIER;
        }
    }
    token.length = length;
    lexer.position = end;
    return token;
}

// Kontrola, zda je kontext celym cislem
static int is_integer(Token *token) {
    size_t start = lexer.position;
    size_t end = start;
    if (lexer.input[start] < '0' || lexer.input[start] > '9') {
        return 0;
    }
    while (end < lexer.length &&
           ((lexer.input[end] >= '0' && lexer.input[end] <= '9') || lexer.input[end] == '_')) {
        end++;
    }
    if (end < lexer.length) {
        char next = lexer.input[end];
        if (next == 'e' || next == 'E') {
            return 0;
        }
        if (next == '.' && (end + 1 == lexer.length || !is_identifier_start(lexer.input[end + 1]))) {
            return 0;
        }
    }
    int invalid = 0;
    int nonzero = 0;
    int64_t value = 0;
    for (size_t i = start; i < end; i++) {
        char character = lexer.input[i];
        if (character == '_') {
            if (i == start || i + 1 == end || lexer.input[i - 1] == '_' || lexer.input[i + 1] == '_') {
                invalid = 1;
            }
        } else {
            int digit = character - '0';
            if (digit != 0) {
                nonzero = 1;
            }
            if (value > (INT64_MAX - digit) / 10) {
                invalid = 1;
            } else {
                value = value * 10 + digit;
            }
        }
    }
    if (lexer.input[start] == '0' && nonzero && end - start > 1) {
        invalid = 1;
    }
    if (invalid) {
        token->type = TOKEN_ERROR;
        token->value = 99;
        token->float_value = 0.0;
        token->length = 0;
        token->context = malloc(2);
        if (token->context != NULL) {
            memcpy(token->context, "1", 2);
            token->value = 1;
        }
        return 1;
    }
    size_t length = end - start;
    token->context = malloc(length + 1);
    if (token->context == NULL) {
        *token = lexer_internal_error();
        return 1;
    }
    memcpy(token->context, lexer.input + start, length);
    token->context[length] = '\0';
    token->type = TOKEN_INT;
    token->value = value;
    token->float_value = 0.0;
    token->length = length;
    lexer.position = end;
    return 1;
}

// Kontrola, zda je kontext desetinym cislem
static int is_float(Token *token) {
    size_t start = lexer.position;
    size_t end = start;
    int has_digits = 0;
    int invalid = 0;
    if (lexer.input[start] != '.' && (lexer.input[start] < '0' || lexer.input[start] > '9')) {
        return 0;
    }
    if (lexer.input[start] == '.' && (start + 1 == lexer.length || lexer.input[start + 1] < '0' || lexer.input[start + 1] > '9')) {
        return 0;
    }
    while (end < lexer.length && ((lexer.input[end] >= '0' && lexer.input[end] <= '9') || lexer.input[end] == '_')) {
        if (lexer.input[end] == '_') {
            if (end == start || end + 1 == lexer.length || lexer.input[end - 1] < '0' || lexer.input[end - 1] > '9' || lexer.input[end + 1] < '0' || lexer.input[end + 1] > '9') {
                invalid = 1;
            }
        } else {
            has_digits = 1;
        }
        end++;
    }
    int has_dot = 0;
    if (end < lexer.length && lexer.input[end] == '.') {
        has_dot = 1;
        end++;
        size_t first = end;
        while (end < lexer.length && ((lexer.input[end] >= '0' && lexer.input[end] <= '9') || lexer.input[end] == '_')) {
            if (lexer.input[end] == '_' && (end == first || end + 1 == lexer.length || lexer.input[end - 1] < '0' || lexer.input[end - 1] > '9' || lexer.input[end + 1] < '0' || lexer.input[end + 1] > '9')) {
                invalid = 1;
            }
            end++;
        }
        if (end == first) {
            invalid = 1;
        }
    }
    if (end < lexer.length && (lexer.input[end] == 'e' || lexer.input[end] == 'E')) {
        end++;
        if (end < lexer.length && (lexer.input[end] == '+' || lexer.input[end] == '-')) {
            end++;
        }
        size_t first = end;
        while (end < lexer.length && ((lexer.input[end] >= '0' && lexer.input[end] <= '9') || lexer.input[end] == '_')) {
            if (lexer.input[end] == '_' && (end == first || end + 1 == lexer.length || lexer.input[end - 1] < '0' || lexer.input[end - 1] > '9' || lexer.input[end + 1] < '0' || lexer.input[end + 1] > '9')) {
                invalid = 1;
            }
            end++;
        }
        if (end == first || (!has_digits && !has_dot)) {
            invalid = 1;
        }
    } else if (!has_dot) {
        return 0;
    }
    if (end < lexer.length) {
        char suffix = lexer.input[end];
        if (suffix == 'd' || suffix == 'D') {
            end++;
        } else if ((suffix == 'f' || suffix == 'F') &&
                   (end + 1 == lexer.length || !is_identifier_part(lexer.input[end + 1]))) {
            invalid = 1;
        }
    }
    if (invalid) {
        token->type = TOKEN_ERROR;
        token->value = 99;
        token->float_value = 0.0;
        token->length = 0;
        token->context = malloc(2);
        if (token->context != NULL) {
            memcpy(token->context, "1", 2);
            token->value = 1;
        }
        return 1;
    }
    size_t length = end - start;
    token->context = malloc(length + 1);
    if (token->context == NULL) {
        *token = lexer_internal_error();
        return 1;
    }
    memcpy(token->context, lexer.input + start, length);
    token->context[length] = '\0';
    char *number = malloc(length + 1);
    if (number == NULL) {
        free(token->context);
        *token = lexer_internal_error();
        return 1;
    }
    size_t count = 0;
    for (size_t i = 0; i < length; i++) {
        if (token->context[i] != '_' && token->context[i] != 'd' && token->context[i] != 'D') {
            number[count] = token->context[i];
            count++;
        }
    }
    number[count] = '\0';
    errno = 0;
    token->float_value = strtod(number, NULL);
    bool outside_range = errno == ERANGE &&
                         (token->float_value == 0.0 || token->float_value == HUGE_VAL);
    free(number);
    if (outside_range) {
        free(token->context);
        token->type = TOKEN_ERROR;
        token->value = 99;
        token->float_value = 0.0;
        token->length = 0;
        token->context = malloc(2);
        if (token->context != NULL) {
            memcpy(token->context, "1", 2);
            token->value = 1;
        }
        return 1;
    }
    token->type = TOKEN_DOUBLE;
    token->value = 0;
    token->length = length;
    lexer.position = end;
    return 1;
}

// Kontrola zda je viceradkovy retezec ("""...""")
static int is_multiline_string(Token *token) {
    size_t start = lexer.position;
    if (lexer.length - start < 3 || memcmp(lexer.input + start, "\"\"\"", 3) != 0) {
        return 0;
    }

    size_t content_start = start + 3;
    size_t end = content_start;
    int closed = 0;
    while (end < lexer.length && lexer.input[end] != '\0') {
        if (lexer.length - end >= 3 && memcmp(lexer.input + end, "\"\"\"", 3) == 0) {
            end += 3;
            while (end < lexer.length && lexer.input[end] == '"') {
                end++;
            }
            closed = 1;
            break;
        }
        end++;
    }

    token->float_value = 0.0;
    if (!closed) {
        token->type = TOKEN_ERROR;
        token->length = 0;
        token->context = malloc(2);
        if (token->context == NULL) {
            token->value = 99;
        } else {
            memcpy(token->context, "1", 2);
            token->value = 1;
        }
        return 1;
    }

    size_t length = end - content_start - 3;
    token->context = malloc(length + 1);
    if (token->context == NULL) {
        *token = lexer_internal_error();
        return 1;
    }

    memcpy(token->context, lexer.input + content_start, length);
    token->context[length] = '\0';
    token->type = TOKEN_STRING;
    token->value = 0;
    token->length = end - start;
    lexer.position = end;
    return 1;
}

// Kontrola, zda jejednoradkovy retezec ("...")
static int is_line_string(Token *token) {
    size_t start = lexer.position;
    if (lexer.input[start] != '"') {
        return 0;
    }
    const char escapes[] = "\\\"nrt";
    const char decoded[] = "\\\"\n\r\t";
    size_t end = start + 1;
    size_t length = 0;
    while (end < lexer.length && lexer.input[end] != '"') {
        char character = lexer.input[end];
        if (character == '\n' || character == '\0') {
            break;
        }
        if (character == '\\') {
            end++;
            if (end == lexer.length ||
                memchr(escapes, lexer.input[end], sizeof escapes - 1) == NULL) {
                break;
            }
        }
        end++;
        length++;
    }

    token->float_value = 0.0;
    if (end == lexer.length || lexer.input[end] != '"') {
        token->type = TOKEN_ERROR;
        token->length = 0;
        token->context = malloc(2);
        if (token->context == NULL) {
            token->value = 99;
        } else {
            memcpy(token->context, "1", 2);
            token->value = 1;
        }
        return 1;
    }

    token->context = malloc(length + 1);
    if (token->context == NULL) {
        *token = lexer_internal_error();
        return 1;
    }

    size_t position = start + 1;
    for (size_t i = 0; i < length; i++) {
        char character = lexer.input[position];
        if (character == '\\') {
            position++;
            const char *escape = memchr(escapes, lexer.input[position], sizeof escapes - 1);
            character = decoded[escape - escapes];
        }
        token->context[i] = character;
        position++;
    }
    token->context[length] = '\0';
    token->type = TOKEN_STRING;
    token->value = 0;
    token->length = end - start + 1;
    lexer.position = end + 1;
    return 1;
}

// Rozpozna boolean operatory (&&, ||, !)
static size_t check_bool_operator(char first, char next) {
    if (first == '!') {
        if (next == '=') {
            return 2;
        }
        return 1;
    }
    if ((first == '&' && next == '&') || (first == '|' && next == '|')) {
        return 2;
    }
    return 0;
}

// Rozpozna kontrolni sipku (<-)
static int is_control_arrow(char first, char next) {
    return first == '<' && next == '-';
}

// Kontrola, zda je znak operatorem
static int is_operator(Token *token) {
    size_t start = lexer.position;
    char first = lexer.input[start];
    char next = '\0';
    if (start + 1 < lexer.length) {
        next = lexer.input[start + 1];
    }
    size_t length = 1;

    switch (first) {
        case '+':
        case '-':
        case '*':
            break;
        case '=':
        case '<':
        case '>':
            if (next == '=' || is_control_arrow(first, next)) {
                length = 2;
            }
            break;
        case '!':
        case '&':
        case '|':
            length = check_bool_operator(first, next);
            if (length == 0) {
                return 0;
            }
            break;
        case '/':
            if (next == '/' || next == '*') {
                return 0;
            }
            break;
        default:
            return 0;
    }

    token->type = TOKEN_OPERATOR;
    token->value = 0;
    token->float_value = 0.0;
    token->length = length;
    token->context = malloc(length + 1);
    if (token->context == NULL) {
        *token = lexer_internal_error();
        return 1;
    }
    memcpy(token->context, lexer.input + start, length);
    token->context[length] = '\0';
    lexer.position += length;
    return 1;
}

// Kontrola, zda je znak oddelovac
static int is_separator(Token *token) {
    size_t start = lexer.position;
    char first = lexer.input[start];

    switch (first) {
        case '(':
        case ')':
        case ',':
        case ':':
        case ';':
            break;
        case '.':
            if (start + 1 < lexer.length) {
                char next = lexer.input[start + 1];
                if (next >= '0' && next <= '9') {
                    return 0;
                }
            }
            break;
        default:
            return 0;
    }

    token->type = TOKEN_SEPARATOR;
    token->value = 0;
    token->float_value = 0.0;
    token->length = 1;
    token->context = malloc(2);
    if (token->context == NULL) {
        *token = lexer_internal_error();
        return 1;
    }
    token->context[0] = first;
    token->context[1] = '\0';
    lexer.position++;
    return 1;
}

// Kontrola, zda je znak anotace (@main)
static int is_annotation(Token *token) {
    static const char annotation[] = "@main";
    size_t start = lexer.position;
    if (lexer.input[start] != '@') {
        return 0;
    }

    size_t length = strlen(annotation);
    size_t available = lexer.length - start;
    int matches = 0;
    if (available >= length) {
        if (memcmp(lexer.input + start, annotation, length) == 0) {
            matches = 1;
        }
    }
    if (matches && available > length) {
        char next = lexer.input[start + length];
        if (is_identifier_part(next)) {
            matches = 0;
        }
    }

    if (!matches) {
        token->type = TOKEN_ERROR;
        token->value = 99;
        token->float_value = 0.0;
        token->length = 0;
        token->context = malloc(2);
        if (token->context != NULL) {
            token->context[0] = '1';
            token->context[1] = '\0';
            token->value = 1;
        }
        return 1;
    }

    token->type = TOKEN_ANNOTATION;
    token->value = 0;
    token->float_value = 0.0;
    token->length = length;
    token->context = malloc(length + 1);
    if (token->context == NULL) {
        *token = lexer_internal_error();
        return 1;
    }
    memcpy(token->context, annotation, length + 1);
    lexer.position += length;
    return 1;
}


// Jednoradkove komentare (//)
static int is_line_comment(Token *token) {
    size_t start = lexer.position;
    if (lexer.length - start < 2) {
        return 0;
    }
    if (lexer.input[start] != '/' || lexer.input[start + 1] != '/') {
		return 0;   // Na teto pozici zadny radkovy komentar
    }

    lexer.position += 2;
    while (lexer.position < lexer.length && lexer.input[lexer.position] != '\n') {
        lexer.position++;
    }
    if (lexer.position < lexer.length) {
		return 2;   // Pokud za komentarem nasleduje '\n'
    }

    token->type = TOKEN_EOL;
    token->context = NULL;
    token->value = 0;
    token->float_value = 0.0;
    token->length = lexer.position - start;
	return 1;       // Pokud soubor konci hned za komentarem, predat pripraveny TOKEN_EOL
}

// Blokove komentare (/* */)
static int is_block_comment(Token *token) {
    size_t start = lexer.position;
    if (lexer.length - start < 2) {
        return 0;
    }
    if (lexer.input[start] != '/' || lexer.input[start + 1] != '*') {
        return 0;
    }

    size_t comment_in = 1;
    size_t comment_out = 0;
    int has_newline = 0;
    lexer.position += 2;
    while (lexer.position < lexer.length && comment_in != comment_out) {
        if (lexer.input[lexer.position] == '\n') {
            has_newline = 1;
        }
        if (lexer.length - lexer.position >= 2) {
            char first = lexer.input[lexer.position];
            char second = lexer.input[lexer.position + 1];
            if (first == '/' && second == '*') {
                comment_in++;
                lexer.position += 2;
                continue;
            }
            if (first == '*' && second == '/') {
                comment_out++;
                lexer.position += 2;
                continue;
            }
        }
        lexer.position++;
    }

    if (comment_in != comment_out) {
        token->type = TOKEN_ERROR;
        token->context = malloc(2);
        token->value = 99;
        token->float_value = 0.0;
        token->length = 0;
        if (token->context != NULL) {
            token->context[0] = '1';
            token->context[1] = '\0';
            token->value = 1;
        }
        return 1;
    }
    if (!has_newline) {
        return 2;
    }

    while (lexer.position < lexer.length &&
           (lexer.input[lexer.position] == ' ' || lexer.input[lexer.position] == '\t' ||
            lexer.input[lexer.position] == '\r')) {
        lexer.position++;
    }
    if (lexer.position < lexer.length && lexer.input[lexer.position] != '\n') {
        token->type = TOKEN_ERROR;
        token->context = malloc(2);
        token->value = 99;
        token->float_value = 0.0;
        token->length = 0;
        if (token->context != NULL) {
            token->context[0] = '2';
            token->context[1] = '\0';
            token->value = 2;
        }
        return 1;
    }
    if (lexer.position < lexer.length) {
        lexer.position++;
    }

    size_t next = lexer.position;
    int space_count = 0;
    while (next < lexer.length && lexer.input[next] == ' ') {
        if (space_count == INT_MAX) {
            *token = lexer_internal_error();
            return 1;
        }
        space_count++;
        next++;
    }
    token->type = TOKEN_EOL;
    token->context = NULL;
    token->value = space_count;
    token->float_value = 0.0;
    token->length = lexer.position - start;
    return 1;
}

// Ziskani dalsiho tokenu
Token lexer_next(void) {
	// Prazdny vstup
    if (lexer.input == NULL) {
        return lexer_internal_error();
    }

	// Preskoceni bilych znaku (mezery, tabulatory, carriage return)
    Token comment;
    bool start_of_line = lexer.position == 0 || lexer.input[lexer.position - 1] == '\n';
    bool char_tabulator = false;
    while (lexer.position < lexer.length) {
        char character = lexer.input[lexer.position];
        if (character == ' ' || character == '\t' || character == '\r') {
            if (start_of_line && character == '\t') {
                char_tabulator = true;
            }
            lexer.position++;
            continue;
        }
        int result = is_line_comment(&comment);
        if (result == 1) {
            return comment;
        }
        if (result == 2) {
            continue;
        }
        result = is_block_comment(&comment);
        if (result == 1) {
            return comment;
        }
        if (result == 2) {
            continue;
        }
        break;
    }

	// Konec vstupu
    if (lexer.position == lexer.length) {
        Token token = {0};
        token.type = TOKEN_EOF;
        return token;
    }

    // Zpracovani konce radku
    if (lexer.input[lexer.position] == '\n') {
        size_t line_end = lexer.position;
        int space_count;
        while (true) {
            size_t next = line_end + 1;
            bool next_has_tabulator = false;
            space_count = 0;
            while (next < lexer.length) {
                char character = lexer.input[next];
                if (character == ' ') {
                    if (space_count == INT_MAX) {
                        return lexer_internal_error();
                    }
                    space_count++;
                } else if (character == '\t') {
                    next_has_tabulator = true;
                } else if (character != '\r') {
                    break;
                }
                next++;
            }
            if (!next_has_tabulator || next == lexer.length || lexer.input[next] != '\n') {
                break;
            }
            line_end = next;
        }

		// Vytvoreni tokenu pro konec radku
        Token token = {0};
        token.type = TOKEN_EOL;
        token.length = 1;
        token.value = space_count;
        lexer.position = line_end + 1;
        return token;
    }

    if (char_tabulator || lexer.input[lexer.position] == '`') {
        Token error;
        error.type = TOKEN_ERROR;
        error.value = 99;
        error.float_value = 0.0;
        error.length = 0;
        error.context = malloc(2);
        if (error.context != NULL) {
            error.context[0] = '1';
            error.context[1] = '\0';
            error.value = 1;
        }
        return error;
    }

	// Zpracovani identifikatoru nebo klicoveho slova
    if (is_identifier_start(lexer.input[lexer.position])) {
        return read_identifier_or_keyword();
    }

	// Zpracovani operatoru, oddelovacu, anotace celych cisel a desetinnych cisel
    Token token;
    if (is_integer(&token)) {
        return token;
    }
    if (is_float(&token)) {
        return token;
    }
    if (is_multiline_string(&token)) {
        return token;
    }
    if (is_line_string(&token)) {
        return token;
    }
    if (is_operator(&token)) {
        return token;
    }
    if (is_separator(&token)) {
        return token;
    }
    if (is_annotation(&token)) {
        return token;
    }

    token.type = TOKEN_ERROR;
    token.value = 99;
    token.float_value = 0.0;
    token.length = 0;
    token.context = malloc(2);
    if (token.context != NULL) {
        token.context[0] = '1';
        token.context[1] = '\0';
        token.value = 1;
    }
    return token;
}

// Ziskani dalsiho tokenu bez posunu pozice
Token lexer_peek(void) {
    size_t position = lexer.position;
    Token token = lexer_next();
    lexer.position = position;
    return token;
}
