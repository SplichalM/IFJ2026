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
        Token error = {.type = TOKEN_ERROR, .value = 99};
        error.context = malloc(sizeof "1");
        if (error.context != NULL) {
            memcpy(error.context, "1", sizeof "1");
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

// Ziskani dalsiho tokenu
Token lexer_next(void) {
	// Prazdny vstup
    if (lexer.input == NULL) {
        return lexer_internal_error();
    }

	// Preskoceni bilych znaku (mezery, tabulatory, carriage return)
    while (lexer.position < lexer.length &&
           (lexer.input[lexer.position] == ' ' ||
            lexer.input[lexer.position] == '\t' ||
            lexer.input[lexer.position] == '\r')) {
        lexer.position++;
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

    if (lexer.input[lexer.position] == '`') {
        Token error = {.type = TOKEN_ERROR, .value = 99};
        error.context = malloc(sizeof "1");
        if (error.context != NULL) {
            memcpy(error.context, "1", sizeof "1");
            error.value = 1;
        }
        return error;
    }

    return lexer_internal_error();
}

// Ziskani dalsiho tokenu bez posunu pozice
Token lexer_peek(void) {
    Token token = lexer_next();
    if (token.type != TOKEN_ERROR) {
        lexer.position -= token.length;
    }
    return token;
}
