// Autor: Jan Pospíšil (xpospij01)

#include "parser.h"
#include "lexer.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "precedence.h"

static Token current;                                           // aktuální token
static size_t current_indent = 0;                               // aktuální úroveň odsazení
static ASTNode *program_root = NULL;                            // kořen AST programu
static Token buffered_token;                                    // dočasně odložený token 
static bool has_buffered_token = false;                         // zda máme odložený token
static const char *const *expression_stop_words = NULL;         // seznam klíčových slov, které ukončují výraz

// přečte další token a uloží ho do current, popřípadě vrátí chybu
static int read_next_token(void)
{
    free(current.context);

    if (has_buffered_token) {
        current = buffered_token;
        has_buffered_token = false;
    } else {
        current = lexer_next();
    }

    return current.type == TOKEN_ERROR ? (int)current.value : 0;
}

// uvolní parser a lexer a ukončí program 
static void finish_parser_error(int error_code)
{
    if (error_code == 2) {
        fprintf(stderr, "Syntax error (code %d)\n", error_code);
    }
    free(current.context);
    if (has_buffered_token) {
        free(buffered_token.context);
        has_buffered_token = false;
    }
    lexer_free();
    precedence_free_pending();
    ast_free(program_root);
    exit(error_code);
}

// vypíše chybu lexeru a zavolá funkci na uvolnení a ukončení se
static void finish_lexer_error(int error_code)
{
    fprintf(stderr, "Lexer error (code %d)\n", error_code);
    finish_parser_error(error_code);
}

// načte další token, ale v případě chyby se korektně ukončí
static void advance(void)
{
    int error_code = read_next_token();
    if (error_code != 0) {
        finish_lexer_error(error_code);
    }
    return ;
}

// spuštění lexéru a načtení prvního tokenu
static int start(void){
    int status = lexer_init();
    if (status != 0) {
        return status;
    }
    advance();
    return 0;
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

// ověření aktuálního tokenu a jeho textu, když by neodpovídal ukončí se 
static void expect(TokenType type, const char *text)
{
    if(current_is(TOKEN_ERROR)){
        finish_lexer_error((int)current.value);
    }

    if(!current_is(type) || (text != NULL && !current_text_is(text))){
        finish_parser_error(2);
    }
    advance();
}

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
        finish_lexer_error(error_code);
    }

    size_t length = strlen(text);

    bool matches = next.type == type &&
                   next.context != NULL &&
                   next.length == length &&
                   memcmp(next.context, text, length) == 0;

    free(next.context);

    return matches;
}

// obnoví EOL po blokovém výrazu a odloží už načtený token
static void restore_line_end(void)
{
    if (current_is(TOKEN_EOL) || current_is(TOKEN_EOF)) {
        return;
    }

    if (has_buffered_token) {
        finish_parser_error(99);
    }

    buffered_token = current;
    has_buffered_token = true;

    current = (Token){
        .type = TOKEN_EOL,
        .value = (int64_t)current_indent
    };
}

//přeskočí konec řádku a nastaví odsazení podle EOL a vrátí ho 
static size_t consume_line_ends(void)
{
    if (!current_is(TOKEN_EOL)) {
        finish_parser_error(2);
    }

    size_t indentation;

    do {
        indentation = (size_t)current.value;
        advance();
    } while (current_is(TOKEN_EOL));

    if (current_is(TOKEN_OPERATOR)) {
        finish_parser_error(2);
    }

    current_indent = indentation;
    return indentation;
}

// zpracuje řádky který pokračují a ověří odsazení vůči zadanému identu
static bool consume_continuation_lines(size_t line_indent)
{
    if (!current_is(TOKEN_EOL)) {
        return false;
    }

    size_t indentation;
    do {
        indentation = (size_t)current.value;
        advance();
    } while (current_is(TOKEN_EOL));

    if (indentation <= line_indent) {
        finish_parser_error(2);
    }

    return true;
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
        finish_parser_error(2);
    }    
}

// rozpoznáni typu 
static DataType parse_type(void)
{
    if(current_is(TOKEN_ERROR)){
        finish_lexer_error((int)current.value);
    }
    if(!current_is(TOKEN_KEYWORD)){
        finish_parser_error(2);
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
    } else if(current_text_is("Boolean")){
        type = TYPE_BOOL;
    }
    else {
        finish_parser_error(2);
    }

    advance();
    return type;
}

// vytvoření parametru do AST seznamu
static void parse_parameter(ASTNode *params)
{
    if(current_is(TOKEN_ERROR)){
        finish_lexer_error((int)current.value);
    }
    if(!current_is(TOKEN_IDENTIFIER)){
        finish_parser_error(2);
    }

    ASTNode *parameter = ast_create_func_param(current.context, TYPE_UNKNOWN);
    ast_list_append(params, parameter);
    advance();

    consume_continuation_lines(current_indent);

    expect(TOKEN_SEPARATOR, ":");

    consume_continuation_lines(current_indent);
    parameter->as.func_param.param_type = parse_type();
}

static ASTNode *parse_expression_until(const char *const *stop_words);

// přečte argumenty v závorce a přidá je do seznamu
static void parse_arguments(ASTNode *args)
{
    expect(TOKEN_SEPARATOR, "(");

    consume_continuation_lines(current_indent);

    if(!current_text_is(")")){
        while(true){
            ASTNode *argument = parse_expression_until(NULL);
            ast_list_append(args, argument);

            consume_continuation_lines(current_indent);

            if(!current_text_is(",")){
                break;
            }

            expect(TOKEN_SEPARATOR, ",");

            bool newline_after_comma = consume_continuation_lines(current_indent);

            if(current_text_is(")")){
                if(!newline_after_comma){
                    finish_parser_error(2);
                }
                break;
            }
        }
    }

    expect(TOKEN_SEPARATOR, ")");
}

// vytvoří volání funkce a zavolá parsování argumentů 
static void parse_function_call(ASTNode **destination)
{
    if(!current_is(TOKEN_IDENTIFIER)){
        finish_parser_error(2);
    }

    ASTNode *call = ast_create_func_call(current.context, NULL);
    *destination = call;
    call->as.func_call.args = ast_create_list(NODE_LIST);

    advance();
    parse_arguments(call->as.func_call.args);

}
static void parse_if_expression(ASTNode **destination);

// zpracování výrazu s precedence tabulkou a popřípadně vyhlásí chybu 
static ASTNode *parse_expression_until(const char *const *stop_words)
{
    const char *const *previous_stop_words = expression_stop_words;
    expression_stop_words = stop_words;

    ExpressionInput input = {
        .current = &current,
        .advance = read_next_token,
        .identifier_followed_by = identifier_followed_by,
        .parse_function_call = parse_function_call,
        .parse_if_expression = parse_if_expression
    };
    ASTNode *expression = parse_precedence_expression(&input, current_indent, stop_words);
    expression_stop_words = previous_stop_words;

    if (expression == NULL) {
        if (current.type == TOKEN_ERROR) {
            finish_lexer_error((int)current.value);
        }

        finish_parser_error(2);
    }
    return expression;
}

static void parse_block(size_t block_indent, ASTNode **destination);

// deklarace proměnné
static void parse_var_decl(ASTNode *block)
{
    bool is_mutable = current_is_keyword("var");

    if(!is_mutable && !current_is_keyword("val")){
        finish_parser_error(2);
    }

    advance();

    if(!current_is(TOKEN_IDENTIFIER)){
        finish_parser_error(2);
    }

    ASTNode *var_decl = ast_create_var_decl(current.context, TYPE_UNKNOWN, NULL, is_mutable);
    ast_list_append(block, var_decl);
    advance();

    DataType type = TYPE_UNKNOWN;

    if(current_is(TOKEN_SEPARATOR) && current_text_is(":")){
        expect(TOKEN_SEPARATOR, ":");
        type = parse_type();
    }

    expect(TOKEN_OPERATOR, "=");

    var_decl->as.var_decl.declared_type = type;
    var_decl->as.var_decl.value = parse_expression_until(NULL);

    consume_statement_end();
}

// zpracovávám přiřazení hodnoty
static void parse_assignment(ASTNode *block)
{
    if (!current_is(TOKEN_IDENTIFIER)) {
        finish_parser_error(2);
    }

    ASTNode *assignment = ast_create_assign(current.context, NULL);
    ast_list_append(block, assignment);
    advance();

    expect(TOKEN_OPERATOR, "=");

    assignment->as.assign.value = parse_expression_until(NULL);

    consume_statement_end();

}

// zpracování returnu
static void parse_return(ASTNode *block){
    expect(TOKEN_KEYWORD, "return");
    ASTNode *node = ast_create_return(NULL);
    ast_list_append(block, node);

    if(!current_is(TOKEN_EOL) && !current_is(TOKEN_EOF) && !(current_is(TOKEN_SEPARATOR) && current_text_is(";"))){
        node->as.ret_stmt.expr = parse_expression_until(NULL);
    }

    consume_statement_end();
}

// načte odsazený blok nebo jednořádkový výraz.
static void parse_branch(size_t parent_indent, const char *const *stop_words, ASTNode **destination)
{
    if (current_is(TOKEN_EOL)) {
        size_t body_indent = consume_line_ends();

        if (current_is(TOKEN_EOF) || body_indent <= parent_indent) {
            finish_parser_error(2);
        }

        parse_block(body_indent, destination);
        return;
    }

    ASTNode *block = ast_create_list(NODE_BLOCK);
    *destination = block;
    ASTNode *value = parse_expression_until(stop_words);
    ast_list_append(block, value);

}

// zpracování if
static void parse_if(size_t block_indent, bool is_expression, ASTNode *node)
{
    const char *const *stop_words = is_expression ? expression_stop_words : NULL;

    expect(TOKEN_KEYWORD, "if");
    node->as.if_stmt.cond = parse_expression_until(NULL);

    expect(TOKEN_KEYWORD, "then");
    parse_branch(block_indent, stop_words, &node->as.if_stmt.then_block);

    if (current_is(TOKEN_EOL)) {
        consume_line_ends();
    }
    
    if (!current_is_keyword("else") || current_indent < block_indent) {
        if (is_expression) {
            finish_parser_error(2);
        }
        return;
    }

    if (current_indent > block_indent) {
        finish_parser_error(2);
    }

    expect(TOKEN_KEYWORD, "else");

    if(current_is_keyword("if")){
        ASTNode *else_block = ast_create_list(NODE_BLOCK);
        // else if uložíme jako další if uvnitř else bloku a else jako další větev 
        //(tohle připomíná meme když zapojuješ usb do sebe)
        node->as.if_stmt.else_block = else_block;
        ASTNode *nelse_if = ast_create_if(NULL, NULL, NULL);
        ast_list_append(else_block, nelse_if);
        parse_if(block_indent, is_expression, nelse_if);

        if (is_expression) {
            node->type = NODE_IF_EXPR;
        }

        return;
    }

    bool else_is_block = current_is(TOKEN_EOL);
    parse_branch(block_indent, stop_words, &node->as.if_stmt.else_block);

    if (is_expression && else_is_block) {
        restore_line_end();
    }
    
    if (!is_expression && current_is(TOKEN_EOL)) {
        consume_line_ends();
    }

    if (is_expression) {
        node->type = NODE_IF_EXPR;
    }

}

// zpracování if na místě výrazu.
static void parse_if_expression(ASTNode **destination)
{
    ASTNode *node = ast_create_if(NULL, NULL, NULL);
    *destination = node;
    parse_if(current_indent, true, node);
}

//zpracování while
static void parse_while(size_t block_indent, ASTNode *block){
    expect(TOKEN_KEYWORD, "while");
    ASTNode *node = ast_create_while(NULL, NULL);
    ast_list_append(block, node);
    node->as.while_stmt.cond = parse_expression_until(NULL);

    expect(TOKEN_KEYWORD, "do");

    size_t body_indent = consume_line_ends();
    if (current_is(TOKEN_EOF) || body_indent <= block_indent) {
        finish_parser_error(2);
    }

    parse_block(body_indent, &node->as.while_stmt.body);
}

static void parse_statement(size_t block_indent, ASTNode *block);

// zpracování for.
static void parse_for(size_t block_indent, ASTNode *block)
{
    expect(TOKEN_KEYWORD, "for");

    bool multiline = current_is(TOKEN_EOL);
    size_t header_indent = block_indent;

    if (multiline) {
        header_indent = consume_line_ends();
        if (current_is(TOKEN_EOF) || header_indent <= block_indent) {
            finish_parser_error(2);
        }
    }

    if (!current_is(TOKEN_IDENTIFIER)) {
        finish_parser_error(2);
    }

    ASTNode *node = ast_create_for(current.context);
    ast_list_append(block, node);
    node->as.for_stmt.guards = ast_create_list(NODE_LIST);

    advance();
    expect(TOKEN_OPERATOR, "<-");

    // určení mezí, to zahrnuje until vynechává
    const char *range_end[] = {"to", "until", NULL};
    node->as.for_stmt.from = parse_expression_until(range_end);

    if (!current_is(TOKEN_IDENTIFIER) ||
        (!current_text_is("to") && !current_text_is("until"))) {
        finish_parser_error(2);
    }

    node->as.for_stmt.inclusive = current_text_is("to");
    advance();

    const char *upper_end[] = {"by", "if", "do", NULL};
    node->as.for_stmt.to = parse_expression_until(upper_end);

    if (current_is(TOKEN_IDENTIFIER) && current_text_is("by")) {
        advance();
        const char *step_end[] = {"if", "do", NULL};
        node->as.for_stmt.step = parse_expression_until(step_end);
    }

    // načtení podmínek 
    const char *guard_end[] = {"if", "do", NULL};
    while (true) {
        if (multiline) {
            if (!current_is(TOKEN_EOL)) {
                finish_parser_error(2);
            }
            consume_line_ends();

            // do na stejné urovni
            if (current_is_keyword("do")) {
                if (current_indent != block_indent) {
                    finish_parser_error(2);
                }
                break;
            }

            if (current_indent != header_indent) {
                finish_parser_error(2);
            }
        } else if (current_is_keyword("do")) {
            break;
        }

        expect(TOKEN_KEYWORD, "if");
        ASTNode *guard = parse_expression_until(guard_end);
        ast_list_append(node->as.for_stmt.guards, guard);
    }

    expect(TOKEN_KEYWORD, "do");

    if (current_is(TOKEN_EOL)) {
        size_t body_indent = consume_line_ends();
        if (current_is(TOKEN_EOF) || body_indent <= block_indent) {
            finish_parser_error(2);
        }
        parse_block(body_indent, &node->as.for_stmt.body);
    } else {
        node->as.for_stmt.body = ast_create_list(NODE_BLOCK);
        parse_statement(block_indent, node->as.for_stmt.body);
    }

}

// podle aktuálního tokenu vybere kam to poslat dál 
static void parse_statement(size_t block_indent, ASTNode *block){
    if(current_is_keyword("for")){
        parse_for(block_indent, block);
        return;
    }
    if(current_is_keyword("if")){
        ASTNode *node = ast_create_if(NULL, NULL, NULL);
        ast_list_append(block, node);
        parse_if(block_indent, false, node);
        return;
    }
    if(current_is_keyword("while")){
        parse_while(block_indent, block);
        return;
    }
    if(current_is_keyword("return")){
        parse_return(block);
        return;
    }

    if(identifier_followed_by(TOKEN_OPERATOR, "=")){
        parse_assignment(block);
        return;
    }

    if(current_is_keyword("var") || current_is_keyword("val")){
        parse_var_decl(block);
        return;
    }

    ASTNode *expression = parse_expression_until(NULL);
    ast_list_append(block, expression);
    consume_statement_end();
}

// zpracování "bloku" - jestli tomu tak smíme říkat
static void parse_block(size_t block_indent, ASTNode **destination)
{
    ASTNode *block = ast_create_list(NODE_BLOCK);
    *destination = block;
    bool has_statements = false;

    while(!current_is(TOKEN_EOF)){
        if (current_indent < block_indent) {
            break;
        }

        if (current_indent > block_indent) {
            finish_parser_error(2);
        }

        parse_statement(block_indent, block);
        has_statements = true;
    }

    if(!has_statements){
        finish_parser_error(2);
    }

}

// zpracování definice funkce
static ASTNode *parse_function(size_t function_indent, bool is_main)
{
    expect(TOKEN_KEYWORD, "def");
    
    if (current_is(TOKEN_ERROR)) {
        finish_lexer_error((int)current.value);
    }

    if (!current_is(TOKEN_IDENTIFIER)) {
        finish_parser_error(2);
    }

    ASTNode *params = ast_create_list(NODE_LIST);
    ASTNode *function = ast_create_func_def(
        current.context, params, TYPE_UNKNOWN, NULL, is_main
    );
    ast_list_append(program_root, function);

    advance();
    expect(TOKEN_SEPARATOR, "(");

    consume_continuation_lines(function_indent);

    if (!current_text_is(")")) {
        while (true) {
            parse_parameter(params);

            consume_continuation_lines(function_indent);

            if (!current_text_is(",")) {
                break;
            }

            expect(TOKEN_SEPARATOR, ",");

            bool newline_after_comma = consume_continuation_lines(function_indent);

            if (current_text_is(")")) {
                if (!newline_after_comma) {
                    finish_parser_error(2);
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

    // tělo funkce je odsazený blok nebo jeden výraz
    if (current_is(TOKEN_EOL)) {
        size_t body_indent = consume_line_ends();

        if (current_is(TOKEN_EOF) || body_indent <= function_indent) {
            finish_parser_error(2);
        }

        parse_block(body_indent, &function->as.func_def.body);
        body = function->as.func_def.body;
    } else {
        body = ast_create_list(NODE_BLOCK);
        function->as.func_def.body = body;
        ASTNode *value = parse_expression_until(NULL);
        ast_list_append(body, value);

        if (current_is(TOKEN_EOL)) {
            consume_line_ends();
        } else if (!current_is(TOKEN_EOF)) {
            finish_parser_error(2);
        }

    }

    function->as.func_def.return_type = return_type;
    function->as.func_def.body = body;
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
ASTNode *parse_program(symtable_t *functions, void (*register_function)(symtable_t *, ASTNode *))
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
    program_root = program;

    while(!current_is(TOKEN_EOF)){
        bool is_main = false;
        size_t function_indent = current_indent;

        if(current_is(TOKEN_ANNOTATION) && current_text_is("@main")){
            is_main = true;
            advance();

            if(current_is(TOKEN_EOL)){
                consume_line_ends();

                if(current_indent != function_indent){
                    finish_parser_error(2);
                }  
            }
        }

        if(!current_is_keyword("def")){
            finish_parser_error(2);
        }
        ASTNode *function = parse_function(function_indent, is_main);

        if (register_function != NULL) {
            register_function(functions, function);
        }
    }
    free(current.context);
    lexer_free();
    program_root = NULL;
    return program;
}    
