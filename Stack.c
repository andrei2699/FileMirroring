#include "Stack.h"

void InitStack(Stack_t *stack_ptr)
{
    stack_ptr->top_ptr = NULL;
}

void StackPush(Stack_t *stack_ptr, void *value_ptr)
{
    StackNode_t *node = (StackNode_t *)malloc(sizeof(StackNode_t));
    node->next_ptr = NULL;
    node->value_ptr = value_ptr;

    if (stack_ptr->top_ptr == NULL)
    {
        stack_ptr->top_ptr = node;
    }
    else
    {
        node->next_ptr = stack_ptr->top_ptr;
        stack_ptr->top_ptr = node;
    }
}

void *StackPop(Stack_t *stack_ptr)
{
    if (stack_ptr->top_ptr == NULL)
    {
        return NULL;
    }

    StackNode_t *lastTop_ptr = stack_ptr->top_ptr;

    void *value_ptr = lastTop_ptr->value_ptr;

    stack_ptr->top_ptr = lastTop_ptr->next_ptr;

    free(lastTop_ptr);

    return value_ptr;
}

void *StackPeek(Stack_t *stack_ptr)
{
    if (stack_ptr->top_ptr == NULL)
    {
        return NULL;
    }
    return stack_ptr->top_ptr->value_ptr;
}

void FreeStack(Stack_t *stack_ptr)
{
    while (stack_ptr->top_ptr != NULL)
    {
        StackPop(stack_ptr);
    }
}
