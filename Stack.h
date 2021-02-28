#ifndef STACK_H
#define STACK_H

#include <stdio.h>
#include <stdlib.h>

typedef struct _stack_node
{
    void *value_ptr;
    struct _stack_node *next_ptr;
} StackNode_t;

typedef struct
{
    StackNode_t *top_ptr;
} Stack_t;

void InitStack(Stack_t *stack_ptr);
void StackPush(Stack_t *stack_ptr, void *value_ptr);
void *StackPop(Stack_t *stack_ptr);
void *StackPeek(Stack_t *stack_ptr);
void FreeStack(Stack_t *stack_ptr);

#endif // STACK_H