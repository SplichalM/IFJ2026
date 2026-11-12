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
        "def", "do", "Double", "else", "if", "import", "Int", "return",
        "String", "then", "Unit", "val", "var", "while"
    };
    for (size_t i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        if (strcmp(context, keywords[i]) == 0) {
            return 1;
        }
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
    if (is_keyword(token.context)) {
        token.type = TOKEN_KEYWORD;
    } else {
        token.type = TOKEN_IDENTIFIER;
    }
    token.length = length;
    lexer.position = end;
    return token;
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
            if (next == '=') {
                length = 2;
            }
            break;
        case '!':
            if (next != '=') {
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
            length = 2;
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
        return 0;
    }

    lexer.position += 2;
    while (lexer.position < lexer.length && lexer.input[lexer.position] != '\n') {
        lexer.position++;
    }
    if (lexer.position < lexer.length) {
        return 2;
    }

    token->type = TOKEN_EOL;
    token->context = NULL;
    token->value = 0;
    token->float_value = 0.0;
    token->length = lexer.position - start;
    return 1;
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
    while (lexer.position < lexer.length) {
        char character = lexer.input[lexer.position];
        if (character == ' ' || character == '\t' || character == '\r') {
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
        size_t next = lexer.position + 1;
        size_t space_count = 0;
		// Pocitani poctu mezer na zacatku radku
        while (next < lexer.length) {
            if (lexer.input[next] != ' ') {
                break;
            }

			// Kontrola preteceni int pri pocitani mezer
            if (space_count == INT_MAX) {
                return lexer_internal_error();
            }
            space_count++;
            next++;
        }

		// Vytvoreni tokenu pro konec radku
        Token token = {0};
        token.type = TOKEN_EOL;
        token.length = 1;
        token.value = (int)space_count;
        lexer.position++;
        return token;
    }

	// Zpracovani identifikatoru nebo klicoveho slova
    if (is_identifier_start(lexer.input[lexer.position])) {
        return read_identifier_or_keyword();
    }

	// Zpracovani operatoru, oddelovacu a anotace
    Token token;
    if (is_operator(&token)) {
        return token;
    }
    if (is_separator(&token)) {
        return token;
    }
    if (is_annotation(&token)) {
        return token;
    }

    if (lexer.input[lexer.position] == '`') {
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

    return lexer_internal_error();
}

// Ziskani dalsiho tokenu bez posunu pozice
Token lexer_peek(void) {
    size_t position = lexer.position;
    Token token = lexer_next();
    lexer.position = position;
    return token;
}
