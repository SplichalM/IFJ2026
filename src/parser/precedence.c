#include "precedence.h"
#include "stack.h"
#include "lexer.h"
#include <stdlib.h>
#include <string.h>

#define S ACTION_SHIFT
#define Q ACTION_EQUAL
#define R ACTION_REDUCE
#define X ACTION_ERROR
#define A ACTION_ACCEPT

static const PrecedenceAction precedence_table[PREC_END + 1][PREC_END + 1] = {
    /* vstup:    opnd  add   mul   rel   eq   unary   (     )     $ */

    /* opnd  */ { X,    R,    R,    R,    R,    X,    X,    R,    R },
    /* add   */ { S,    R,    S,    R,    R,    S,    S,    R,    R },
    /* mul   */ { S,    R,    R,    R,    R,    S,    S,    R,    R },
    /* rel   */ { S,    S,    S,    X,    R,    S,    S,    R,    R },
    /* eq    */ { S,    S,    S,    S,    X,    S,    S,    R,    R },
    /* unary */ { S,    R,    R,    R,    R,    S,    S,    R,    R },
    /* (     */ { S,    S,    S,    S,    S,    S,    S,    Q,    X },
    /* )     */ { X,    R,    R,    R,    R,    X,    X,    R,    R },
    /* $     */ { S,    S,    S,    S,    S,    S,    S,    X,    A }
};

#undef S
#undef Q
#undef R
#undef X
#undef A

// kontrola typu tokenu
static bool token_is(const Token *token, TokenType type)
{
    return token->type == type;
}

// porovnání textu tokenu
static bool token_text_is(const Token *token, const char *text)
{
    size_t length = strlen(text);
    return token->context != NULL
        && token->length == length
        && memcmp(token->context, text, length) == 0;
}

// kontrola zda token odpovídá klíčovému slovu
static bool token_is_keyword(const Token *token, const char *keyword)
{
    return token_is(token, TOKEN_KEYWORD) && token_text_is(token, keyword);
}

// určení symbolu výrazu
static PrecedenceSymbol expression_symbol(const Token *token, bool expect_operand)
{
    if (token_is(token, TOKEN_ERROR)) {
        exit(1);
    }

    if(token_is(token, TOKEN_IDENTIFIER) || token_is(token, TOKEN_INT) || token_is(token, TOKEN_DOUBLE) || token_is(token, TOKEN_STRING)){
        return PREC_OPERAND;
    } 

    if(token_is_keyword(token, "then")|| token_is_keyword(token, "do")){
        return PREC_END;
    }
    if (token_is(token, TOKEN_EOL) || token_is(token, TOKEN_EOF)) {
        return PREC_END;
    }

    if (token_is(token, TOKEN_SEPARATOR) &&
        token_text_is(token, ";")) {
        return PREC_END;
    }

    if(token_is(token, TOKEN_SEPARATOR)){
        if(token_text_is(token, "(")){
            return PREC_LEFT_PAREN;
        }  
        
        if(token_text_is(token, ")")){
            return PREC_RIGHT_PAREN;
        }
    }

    if(token_is(token, TOKEN_OPERATOR)){
        if(token_text_is(token, "-")){
            return expect_operand ? PREC_UNARY_MINUS : PREC_ADD;
        }
        if(token_text_is(token, "+")){
            return PREC_ADD;
        }
        if(token_text_is(token, "*") || token_text_is(token, "/")){
            return PREC_MULTIPLY;
        }
        if(token_text_is(token, "<") || token_text_is(token, ">") || token_text_is(token, "<=") || token_text_is(token, ">=")){
            return PREC_RELATION;
        }
        if(token_text_is(token, "==") || token_text_is(token, "!=")){
            return PREC_EQUALITY;
        }
    }

    return PREC_INVALID;


}

// vytvoření uzlu pro operand
static ASTNode *create_operand_node(const Token *token)
{
    switch (token->type)
    {
        case TOKEN_IDENTIFIER:
        {
            return ast_create_ident(token->context);
        }
        case TOKEN_INT:
        {
            return ast_create_literal_int(token->value);
        }
        case TOKEN_DOUBLE:
        {
            return ast_create_literal_double(token->float_value);
        }
        case TOKEN_STRING:
        {
            return ast_create_literal_string(token->context);
        }
        default:
        {
            exit(2);
        }
    }
}

// vytvoření položky zásobníku
static StackItem create_stack_item(const Token *token, PrecedenceSymbol symbol)
{
    StackItem item = {0};
    item.symbol = symbol;
    
    if (symbol == PREC_OPERAND)
    {
        item.node = create_operand_node(token);
    }else if (symbol == PREC_ADD || symbol == PREC_MULTIPLY || symbol == PREC_RELATION || symbol == PREC_EQUALITY || symbol == PREC_UNARY_MINUS)
    {
         if (token->context == NULL ||
            token->length >= sizeof(item.operator_text)) {
            exit(99);
        }

        memcpy(item.operator_text, token->context, token->length);
        item.operator_text[token->length] = '\0';
    } 
    return item;
}

// označení na zásobníku jako htoový 
static void reduce_operand(PrecedenceStack *stack)
{
    if(stack->count < 2){
        exit(2);
    }

    StackItem operand_item = stack->items[stack->count - 1];
    StackItem mark_item = stack->items[stack->count - 2];

    if(mark_item.symbol != PREC_MARKER || operand_item.symbol != PREC_OPERAND){
        exit(2);
    }

    stack_pop(stack);
    stack_pop(stack);

    StackItem new_item = {0};
    new_item.symbol = PREC_EXPRESSION;
    new_item.node = operand_item.node;

    stack_push(stack, new_item);
}

//složení levého a praveho výrazu do jednoho AST
static void reduce_binary(PrecedenceStack *stack)
{
    if(stack->count < 4){
        exit(2);
    }

    StackItem right_item = stack->items[stack->count - 1];
    StackItem operator = stack->items[stack->count - 2];
    StackItem left_item = stack->items[stack->count - 3];
    StackItem mark = stack->items[stack->count - 4];

    if(mark.symbol != PREC_MARKER || left_item.symbol != PREC_EXPRESSION ||right_item.symbol != PREC_EXPRESSION){
        exit(2);
    }

    if(operator.symbol != PREC_ADD && operator.symbol != PREC_MULTIPLY && operator.symbol != PREC_RELATION && operator.symbol != PREC_EQUALITY){
        exit(2);
    }

    ASTNode *new_node = ast_create_binary_op(operator.operator_text, left_item.node, right_item.node);

    stack_pop(stack); /*pravý item*/
    stack_pop(stack); /*op*/
    stack_pop(stack); /*levý item*/
    stack_pop(stack); /*značka*/

    StackItem new_item = {0};
    new_item.symbol = PREC_EXPRESSION;
    new_item.node = new_node;

    stack_push(stack, new_item);
}

// odstranení závorek a ponechání výrazu
static void reduce_parentheses(PrecedenceStack *stack)
{
    if(stack->count < 4){
        exit(2);
    }

    StackItem right_paren = stack->items[stack->count - 1];
    StackItem expr_item = stack->items[stack->count - 2];
    StackItem left_paren = stack->items[stack->count - 3];
    StackItem mark = stack->items[stack->count - 4];

    if(mark.symbol != PREC_MARKER || left_paren.symbol != PREC_LEFT_PAREN || right_paren.symbol != PREC_RIGHT_PAREN || expr_item.symbol != PREC_EXPRESSION){
        exit(2);
    }

    stack_pop(stack); /*)*/
    stack_pop(stack); /*výraz*/
    stack_pop(stack); /*(*/
    stack_pop(stack); /*značka*/

    stack_push(stack, expr_item);
}

// vytviření unárního mínus
static void reduce_unary(PrecedenceStack *stack)
{
    if(stack->count < 3){
        exit(2);
    }

    StackItem expression = stack->items[stack->count - 1];
    StackItem operator = stack->items[stack->count - 2];
    StackItem mark = stack->items[stack->count - 3];

    if(mark.symbol != PREC_MARKER || operator.symbol != PREC_UNARY_MINUS || expression.symbol != PREC_EXPRESSION){
        exit(2);
    }

    ASTNode *new_node = ast_create_unary_op(operator.operator_text, expression.node);

    stack_pop(stack); /*výrazek*/
    stack_pop(stack); /* - */
    stack_pop(stack); /*značka*/

    StackItem new_item = {0};
    new_item.symbol = PREC_EXPRESSION;
    new_item.node = new_node;

    stack_push(stack, new_item);
}

// nahrazení prázdných závorek Unitem 
static void reduce_unit(PrecedenceStack *stack)
{
    if (stack->count < 3) {
        exit(2);
    }

    StackItem right_paren = stack->items[stack->count - 1];
    StackItem left_paren = stack->items[stack->count - 2];
    StackItem marker = stack->items[stack->count - 3];

    if (marker.symbol != PREC_MARKER ||
        left_paren.symbol != PREC_LEFT_PAREN ||
        right_paren.symbol != PREC_RIGHT_PAREN) {
        exit(2);
    }

    stack_pop(stack);  /* ) */
    stack_pop(stack);  /* ( */
    stack_pop(stack);  /* značka */

    StackItem result = {0};
    result.symbol = PREC_EXPRESSION;
    result.node = ast_create_literal_unit();

    stack_push(stack, result);
}

// zpracování redukce na zásobníku
static void reduce(PrecedenceStack *stack)
{
    size_t marker_index = stack->count;

    while (marker_index > 0) {
        marker_index--;
        if (stack->items[marker_index].symbol == PREC_MARKER) {
            break;
        }
    }

    if(stack->count == 0 || stack->items[marker_index].symbol != PREC_MARKER){
        exit(2);
    }

    size_t lenght = stack->count - marker_index - 1;

    if(lenght == 1){
        reduce_operand(stack);
    } else if(lenght == 2){
        if (stack->items[marker_index + 1].symbol == PREC_LEFT_PAREN) {
            reduce_unit(stack);
        } else {
            reduce_unary(stack);
        }
    } else if(lenght == 3){
        if (stack->items[marker_index + 1].symbol == PREC_LEFT_PAREN) {
            reduce_parentheses(stack);
        } else {
            reduce_binary(stack);
        }
    } else {
        exit(2);
    }
}

// určení akce precedence tabulky
static PrecedenceAction precedence_action(PrecedenceSymbol stack_symbol, PrecedenceSymbol input_symbol)
{
    if((unsigned)stack_symbol > PREC_END || (unsigned)input_symbol > PREC_END){
        return ACTION_ERROR;
    }

    return precedence_table[stack_symbol][input_symbol];
}

// zpracování výrazu s precedence tabulkou
ASTNode *parse_precedence_expression(Token *current, int (*advance)(void), size_t line_indent)
{
    PrecedenceStack stack;
    stack_init(&stack);

    StackItem end = {0};
    end.symbol = PREC_END;
    stack_push(&stack, end);

    bool expect_operand = true;
    size_t parenthesis_count = 0;

    while(true){
        if (current->type == TOKEN_EOL && parenthesis_count > 0) {
            size_t next_line_indent;
            do{
                next_line_indent = (size_t)current->value;
                advance();
            }while(current->type == TOKEN_EOL);

            if(current->type == TOKEN_EOF || next_line_indent <= line_indent){
                exit(2);
            }
        }

        PrecedenceSymbol input_symbol = expression_symbol(current, expect_operand);

        if (expect_operand) {
            bool empty_parentheses = input_symbol == PREC_RIGHT_PAREN && stack.count > 0 && stack.items[stack.count - 1].symbol == PREC_LEFT_PAREN;
            if (input_symbol != PREC_OPERAND && input_symbol != PREC_UNARY_MINUS && input_symbol != PREC_LEFT_PAREN && !empty_parentheses) {
                exit(2);
            }
        } else {
            if (input_symbol == PREC_OPERAND || input_symbol == PREC_UNARY_MINUS || input_symbol == PREC_LEFT_PAREN || input_symbol == PREC_INVALID) {
                exit(2);
            }
        }

        size_t top_index = stack_top_terminal_index(&stack);
        PrecedenceSymbol stack_symbol = stack.items[top_index].symbol;

        PrecedenceAction action = precedence_action(stack_symbol, input_symbol);

        if(action == ACTION_REDUCE){
            reduce(&stack);
        }else if(action == ACTION_SHIFT || action == ACTION_EQUAL){
            if(action == ACTION_SHIFT){
                stack_insert_marker(&stack);
            }

            StackItem item = create_stack_item(current, input_symbol);
            stack_push(&stack, item);

            if(input_symbol == PREC_LEFT_PAREN){
                parenthesis_count++;
            }else if(input_symbol == PREC_RIGHT_PAREN){
                if(parenthesis_count == 0){
                    exit(2);
                }
                parenthesis_count--;
            }

             expect_operand = input_symbol != PREC_OPERAND && input_symbol != PREC_RIGHT_PAREN;

             advance();
        }else if(action == ACTION_ACCEPT){
            if(expect_operand || parenthesis_count != 0 || stack.count != 2 || stack.items[0].symbol != PREC_END || stack.items[1].symbol != PREC_EXPRESSION){
                exit(2);
            }

            ASTNode *result = stack.items[1].node;
            stack_free(&stack);
            return result;
        }else{
            stack_free(&stack);
            exit(2);
        }
    }
}