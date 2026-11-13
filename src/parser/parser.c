#include "parser.h"
#include "lexer.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stack.h"
#include "precedence.h"

static Token current;
static size_t current_indent = 0;

// Práce s tokeny
// uvolní text aktuálního tokenu a načte další 
static int advance(void)
{
    free(current.context);

    current = lexer_next();

    if (current.type == TOKEN_ERROR) {
        int error_code = current.value;
        free(current.context);
        lexer_free();
        exit(error_code);
    }

    return 0;

}

// spuštění lexéru a načtení prvního tokenu
static int start(void){
    int status = lexer_init();
    if (status != 0) {
        return status;
    }

    return advance();
}

// ověření typu aktuálního tokenu (stav zůstává)
static bool current_is(TokenType type)
{
    return current.type == type;
}

// ověření textu aktuálního tokenu (stav zůstává)
static bool current_text_is(const char *text)
{
    size_t length = strlen(text);
    return current.context != NULL
        && current.length == length
        && memcmp(current.context, text, length) == 0;
}

// oveření zda aktuální token je klíčeové slovo
static bool current_is_keyword(const char *keyword)
{
    return current_is(TOKEN_KEYWORD) && current_text_is(keyword);
}

// ověření aktuálního tokenu a jeho testu, když by neodpovídal ukončí se 
static void expect(TokenType type, const char *text)
{
    if(current_is(TOKEN_ERROR)){
        exit(1);
    }

    if(!current_is(type) || (text != NULL && !current_text_is(text))){
        exit(2);
    }
    advance();
}

//zkopírování textu aktuálního tokenu
static char *copy_current_text(void)
{
    char *copy = malloc(current.length +1);
    if(copy == NULL){
        exit(99);
    }
    memcpy(copy, current.context, current.length);
    copy[current.length] = '\0';
    return copy;
}

// Rozpoznání podle následujícího tokenu
//ověření typu a textu nasledujícího tokenu
static bool identifier_followed_by(TokenType type, const char *text)
{
    if (!current_is(TOKEN_IDENTIFIER)) {
        return false;
    }

    Token next = lexer_peek();

    if (next.type == TOKEN_ERROR) {
        int error_code = next.value;
        free(next.context);
        free(current.context);
        lexer_free();
        exit(error_code);
    }

    size_t length = strlen(text);

    bool matches = next.type == type &&
                   next.context != NULL &&
                   next.length == length &&
                   memcmp(next.context, text, length) == 0;

    free(next.context);

    return matches;
}

//
static bool starts_function_call(void)
{
    return identifier_followed_by(TOKEN_SEPARATOR, "(");
}

// rozpoznání přiřazení podle následujícího tokenu
static bool starts_assignment(void)
{
    return identifier_followed_by(TOKEN_OPERATOR, "=");
}

// Konce řádků a příkazů
//přeskočí konec řádku a nastaví odsazení podle EOL
static size_t consume_line_ends(void)
{
    if (!current_is(TOKEN_EOL)) {
        exit(2);
    }

    size_t indentation;

    do {
        indentation = (size_t)current.value;
        advance();
    } while (current_is(TOKEN_EOL));

    current_indent = indentation;
    return indentation;
}

// zpracuje konec příkazu - ; nebo EOF
static void consume_statement_end(void)
{
    if(current_is(TOKEN_EOL)){
        consume_line_ends();
    }else if(current_is(TOKEN_EOF)){
        return;
    }else if(current_is(TOKEN_SEPARATOR) && current_text_is(";")){
        expect(TOKEN_SEPARATOR, ";");
        if(current_is(TOKEN_EOL)){
            consume_line_ends();
        }
    }else{
        exit(2);
    }    
}

// Typy, parametry, argumenty a výrazy
// rozpoznáni typu 
static DataType parse_type(void)
{
    if(current_is(TOKEN_ERROR)){
        exit(1);
    }
    if(!current_is(TOKEN_KEYWORD)){
        exit(2);
    }

    DataType type;
    if(current_text_is("Int")){
        type = TYPE_INT;
    } else if(current_text_is("Double")){
        type = TYPE_DOUBLE;
    } else if(current_text_is("String")){
        type = TYPE_STRING;
    } else if(current_text_is("Unit")){
        type = TYPE_UNIT;
    } else {
        exit(2);
    }

    advance();
    return type;
}

// vytvoření parametru do AST
static ASTNode *parse_parameter(void)
{
    if(current_is(TOKEN_ERROR)){
        exit(1);
    }
    if(!current_is(TOKEN_IDENTIFIER)){
        exit(2);
    }

    char *param_name = copy_current_text();
    advance();
    expect(TOKEN_SEPARATOR, ":");
    DataType param_type = parse_type();

    ASTNode *parameter = ast_create_func_param(param_name, param_type);
    free(param_name);
    return parameter;
}

// vytvoří hodnotu pro identifikátor 
static ASTNode *parse_term(void)
{
    ASTNode *term;

    switch (current.type) {
        case TOKEN_IDENTIFIER: {
            term = ast_create_ident(current.context);
            break;
        }
        case TOKEN_INT: {
            term = ast_create_literal_int(current.value);
            break;
        }
        case TOKEN_DOUBLE: {
            term = ast_create_literal_double(current.float_value);
            break;
        }
        case TOKEN_STRING: {
            term = ast_create_literal_string(current.context);
            break;
        }
        case TOKEN_SEPARATOR: {
            if(!current_text_is("(")){
                exit(2);
            }

            expect(TOKEN_SEPARATOR, "(");

            while (current_is(TOKEN_EOL)){
                advance();
            }

            expect(TOKEN_SEPARATOR, ")");

            return ast_create_literal_unit();
        }
        default:
            exit(2);
    }

    advance();
    return term;
}

// přečte argumenty v závorce a vrátí je jako list
static ASTNode *parse_arguments(void)
{
    expect(TOKEN_SEPARATOR, "(");
    ASTNode *args = ast_create_list(NODE_LIST);

    while(current_is(TOKEN_EOL)){
        advance();
    }

    if(!current_text_is(")")){
        while(true){
            ASTNode *arg = parse_term();
            ast_list_append(args, arg);

            while(current_is(TOKEN_EOL)){
                advance();
            }

            if(!current_text_is(",")){
                break;
            }

            expect(TOKEN_SEPARATOR, ",");

            bool newline_after_comma = false;

            while(current_is(TOKEN_EOL)){
                newline_after_comma = true;
                advance();
            }

            if(current_text_is(")")){
                if(!newline_after_comma){
                    exit(2);
                }
                break;
            }
        }
    }

    expect(TOKEN_SEPARATOR, ")");
    return args;
}

// vytvoří volání funkce 
static ASTNode *parse_function_call(void)
{
    if(!current_is(TOKEN_IDENTIFIER)){
        exit(2);
    }

    char *func_name = copy_current_text();
    advance();
    ASTNode *args = parse_arguments();

    ASTNode *function_call = ast_create_func_call(func_name, args);
    free(func_name);
    return function_call;
}

// zavolá bud volání funkce nebo to pošle do precedence parseru
static ASTNode *parse_value(void)
{
    if(starts_function_call()){
        return parse_function_call();
    }
     
    return parse_precedence_expression(&current, advance, current_indent);
}

// zpracování podmínky (pro if a while)
static ASTNode *parse_condition(void){
    return parse_precedence_expression(&current, advance, current_indent);
}

// Příkazy a bloky

static ASTNode *parse_block(size_t block_indent);

// deklarace proměnné
static ASTNode *parse_var_decl(void)
{
    bool is_mutable = current_is_keyword("var");

    if(!is_mutable && !current_is_keyword("val")){
        exit(2);
    }

    advance();

    if(!current_is(TOKEN_IDENTIFIER)){
        exit(2);
    }

    char *var_name = copy_current_text();
    advance();

    DataType type = TYPE_UNKNOWN;

    if(current_is(TOKEN_SEPARATOR) && current_text_is(":")){
        expect(TOKEN_SEPARATOR, ":");
        type = parse_type();
    }

    expect(TOKEN_OPERATOR, "=");

    ASTNode *value = parse_value();

    consume_statement_end();

    ASTNode *var_decl = ast_create_var_decl(var_name, type, value, is_mutable);
    free(var_name);
    return var_decl;
}

// zpracovávám přiřazení hodnoty
static ASTNode *parse_assignment(void)
{
    if (!current_is(TOKEN_IDENTIFIER)) {
        exit(2);
    }

    char *name = copy_current_text();
    advance();

    expect(TOKEN_OPERATOR, "=");

    ASTNode *value = parse_value();

    consume_statement_end();

    ASTNode *assignment = ast_create_assign(name, value);
    free(name);

    return assignment;
}

// zpracování returnu
static ASTNode *parse_return(void){
    expect(TOKEN_KEYWORD, "return");
    ASTNode *value = NULL;

    if(!current_is(TOKEN_EOL) && !current_is(TOKEN_EOF) && !(current_is(TOKEN_SEPARATOR) && current_text_is(";"))){
        value = parse_value();
    }

    consume_statement_end();
    return ast_create_return(value);
}

// zpracování if
static ASTNode *parse_if(size_t block_indent)
{
    expect(TOKEN_KEYWORD, "if");
    ASTNode *condition = parse_condition();

    expect(TOKEN_KEYWORD, "then");
    size_t then_indent = consume_line_ends();

    if (current_is(TOKEN_EOF) || then_indent <= block_indent) {
        exit(2);
    }

    ASTNode *then_block = parse_block(then_indent);

    if (current_is(TOKEN_EOF) || current_indent != block_indent) {
        exit(2);
    }

    expect(TOKEN_KEYWORD, "else");

    size_t else_indent = consume_line_ends();

    if (current_is(TOKEN_EOF) || else_indent <= block_indent) {
        exit(2);
    }

    ASTNode *else_block = parse_block(else_indent);

    return ast_create_if(condition, then_block, else_block);
}

//zpracování while
static ASTNode *parse_while(size_t block_indent){
    expect(TOKEN_KEYWORD, "while");
    ASTNode *condition = parse_condition();

    expect(TOKEN_KEYWORD, "do");

    size_t body_indent = consume_line_ends();
    if (current_is(TOKEN_EOF) || body_indent <= block_indent) {
        exit(2);
    }

    ASTNode *body = parse_block(body_indent);

    return ast_create_while(condition, body);
}

// zpracování příkazu - vybere podle aktuálního tokenu
static ASTNode *parse_statement(size_t block_indent){
    if(current_is_keyword("if")){
        return parse_if(block_indent);
    }
    if(current_is_keyword("while")){
        return parse_while(block_indent);
    }
    if(current_is_keyword("return")){
        return parse_return();
    }

    if(starts_function_call()){
        ASTNode *func_call = parse_function_call();
        consume_statement_end();
        return func_call;
    }

    if(starts_assignment()){
        return parse_assignment();
    }

    if(current_is_keyword("var") || current_is_keyword("val")){
        return parse_var_decl();
    }

    ASTNode *expression = parse_precedence_expression(&current, advance, current_indent);
    consume_statement_end();
    return expression;
}

// zpracování "bloku" - jestli tomu tak smíme říkat
static ASTNode *parse_block(size_t block_indent)
{
    ASTNode *block = ast_create_list(NODE_BLOCK);
    bool has_statements = false;

    while(!current_is(TOKEN_EOF)){
        if (current_indent < block_indent) {
            break;
        }

        if (current_indent > block_indent) {
            exit(2);
        }

        ASTNode *statement = parse_statement(block_indent);
        ast_list_append(block, statement);
        has_statements = true;
    }

    if(!has_statements){
        exit(2);
    }

    return block;
}

// Definice funkcí a celý program
// zpracování definice funkce
static ASTNode *parse_function(size_t function_indent, bool is_main)
{
    expect(TOKEN_KEYWORD, "def");
    
    if (current_is(TOKEN_ERROR)) {
        exit(1);
    }

    if (!current_is(TOKEN_IDENTIFIER)) {
        exit(2);
    }

    char *func_name = copy_current_text();

    advance();
    expect(TOKEN_SEPARATOR, "(");
    ASTNode *params = ast_create_list(NODE_LIST);

    while (current_is(TOKEN_EOL)) {
        advance();
    }
    if (!current_text_is(")")) {
        while (true) {
            ASTNode *parameter = parse_parameter();
            ast_list_append(params, parameter);

            while (current_is(TOKEN_EOL)) {
                advance();
            }

            if (!current_text_is(",")) {
                break;
            }

            expect(TOKEN_SEPARATOR, ",");

            bool newline_after_comma = false;

            while (current_is(TOKEN_EOL)) {
                newline_after_comma = true;
                advance();
            }

            if (current_text_is(")")) {
                if (!newline_after_comma) {
                    exit(2);
                }

                break;
            }
        }
    }

    expect(TOKEN_SEPARATOR, ")");

    DataType return_type = TYPE_UNKNOWN;

    if (current_text_is(":")) {
        expect(TOKEN_SEPARATOR, ":");
        return_type = parse_type();
    }

    expect(TOKEN_OPERATOR, "=");

     ASTNode *body;

    if (current_is(TOKEN_EOL)) {
        size_t body_indent = consume_line_ends();

        if (current_is(TOKEN_EOF) || body_indent <= function_indent) {
            exit(2);
        }

        body = parse_block(body_indent);
    } else {
        ASTNode *value = parse_value();
        consume_statement_end();

        body = ast_create_list(NODE_BLOCK);
        ast_list_append(body, value);
    }

    ASTNode *function = ast_create_func_def(
        func_name,
        params,
        return_type,
        body,
        is_main
    );

    free(func_name);

    return function;
}

// zpracování prologu 
static void parse_prolog(void){
    expect(TOKEN_KEYWORD, "import");
    expect(TOKEN_IDENTIFIER, "ifj26");
    expect(TOKEN_SEPARATOR, ".");
    expect(TOKEN_OPERATOR, "*");
}

// zpracování celého programu
ASTNode *parse_program(
    symtable_t *functions,
    void (*register_function)(symtable_t *, ASTNode *)
)
{
    int status = start();
    if(status != 0){
        exit(status);
    }

    while(current_is(TOKEN_EOL)){
        advance();
    }
    parse_prolog();

    consume_line_ends();
    ASTNode *program = ast_create_list(NODE_PROGRAM);

    while(!current_is(TOKEN_EOF)){
        bool is_main = false;
        size_t function_indent = current_indent;

        if(current_is(TOKEN_ANNOTATION) && current_text_is("@main")){
            is_main = true;
            advance();

            if(current_is(TOKEN_EOL)){
                consume_line_ends();

                if(current_indent != function_indent){
                    exit(2);
                }  
            }
        }

        if(!current_is_keyword("def")){
            exit(2);
        }
        ASTNode *function = parse_function(function_indent, is_main);
        ast_list_append(program, function);

        if (register_function != NULL) {
            register_function(functions, function);
        }
    }
    free(current.context);
    lexer_free();
    return program;
}    
