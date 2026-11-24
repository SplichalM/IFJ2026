#include "stack.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// inicializace zásobníku
void stack_init(PrecedenceStack *stack) {
    stack->items = NULL;
    stack->count = 0;
    stack->capacity = 0;
}

// přidání položky na zásobník
void stack_push(PrecedenceStack *stack, StackItem item) {
    if (stack->count >= stack->capacity) {
        if (stack->capacity > SIZE_MAX / 2) {
            exit(99);
        }

        size_t new_capacity = (stack->capacity == 0)
            ? 64
            : stack->capacity * 2;

        if (new_capacity > SIZE_MAX / sizeof(StackItem)) {
            exit(99);
        }

        StackItem *new_items = realloc(stack->items, new_capacity * sizeof(StackItem));
        if (new_items == NULL) {
            exit(99); 
        }
        stack->items = new_items;
        stack->capacity = new_capacity;
    }
    stack->items[stack->count++] = item;
}

// zjištění jestli je zásobník prázdný
bool stack_is_empty(const PrecedenceStack *stack) {
    return stack->count == 0;
}

// vrácení vrchní položky zásobníku
StackItem stack_top(const PrecedenceStack *stack)
{
    if (stack_is_empty(stack)) {
        exit(99);
    }

    return stack->items[stack->count - 1];
}

// odstranění vrchní položky zásobníku
void stack_pop(PrecedenceStack *stack)
{
    if (stack_is_empty(stack)) {
        exit(99);
    }

    stack->count--;
}

// odstranění všech položek zásobníku
void stack_free(PrecedenceStack *stack) {
    free(stack->items);
    stack_init(stack);
}

// zjištění indexu vrchní terminální položky zásobníku
size_t stack_top_terminal_index(const PrecedenceStack *stack) {
    size_t index = stack->count;

    while (index > 0) {
        index--;
        PrecedenceSymbol symbol = stack->items[index].symbol;
        if (symbol >= PREC_OPERAND && symbol <= PREC_END) {
            return index;
        }

        if(symbol != PREC_EXPRESSION && symbol != PREC_MARKER){
            exit(99);
        }
    }
    exit(99);
}

// vložení značky na zásobník
void stack_insert_marker(PrecedenceStack *stack)
{
    size_t position = stack_top_terminal_index(stack) + 1;

    StackItem marker = {0};
    marker.symbol = PREC_MARKER;

    stack_push(stack, marker);

    for (size_t i = stack->count - 1; i > position; i--) {
        stack->items[i] = stack->items[i - 1];
    }

    stack->items[position] = marker;
}