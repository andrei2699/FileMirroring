#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>

typedef struct _list_node
{
    char *value;
    struct _list_node *next;
} ListNode_t;

void InitList(ListNode_t *head);
void ListAdd(ListNode_t *head, char *value);
void ListRemove(ListNode_t *head, char *value);
int ListSearch(ListNode_t *head, char *value);
void ListPrint(ListNode_t *head);
void FreeList(ListNode_t *head);

#endif // LIST_H