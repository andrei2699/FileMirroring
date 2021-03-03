#include "List.h"
#include <string.h>

void InitList(ListNode_t *head, char *value)
{
    if(strcmp(value, "") == 0) return;
    head->value = (char *) malloc(sizeof(char) * strlen(value));
    strcpy(head->value, value);
    head->next = NULL;
}

void ListAdd(ListNode_t *head, char *value)
{
    if(strcmp(value, "") == 0) return;
    
    ListNode_t * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = (ListNode_t *) malloc(sizeof(ListNode_t));
    current->next->value = (char *) malloc(sizeof(char) * strlen(value));
    strcpy(current->next->value, value);
    current->next->next = NULL;
}

void ListPrint(ListNode_t *head) 
{
    ListNode_t *current = head;

    while (current != NULL) {
        printf("%s\n", current->value);
        current = current->next;
    }
}

void ListRemove(ListNode_t *head, char *value);
void ListSearch(ListNode_t *head);
void FreeList(ListNode_t *head);