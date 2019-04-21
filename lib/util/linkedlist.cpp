/**
 * @file linkedlist.c
 * @author     Sihao Yu (sihaoy)
 * @author     Apoorv Gupta (apoorvg1)
 *
 * @brief      Linked list implementation
 */
#include <stddef.h>
#include "linkedlist.h"

void initList(List* result){
    //check for NULL pointer
    if (result == NULL) return;
    result->front = NULL;
    result->back = NULL;
    result->length = 0;
    return;
}

ListNode* getDataNode(List* list, void* data, int(*cmp)(void*, void*)){
    ListNode* cur = list->front;
    while(cur != NULL){
        if (1 == (*cmp) (data, cur->data) ){
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

void removeNode(List* list, ListNode* node){
    if ((list->length) == 0 || node == NULL) return;
    if (list->length == 1){
        list->front = NULL;
        list->back = NULL;
        list->length --;
        return;
    }
    else{
        if (node == list->back){
            list->back = node->prev;
            list->back->next = NULL;
        }
        else if (node == list->front){
            list->front = node->next;
            node->next->prev = node->prev;
        }
        else if(node->prev != NULL){
            node->prev->next = node->next;
        }
        if (node->next != NULL){
            node->next->prev = node->prev;
        }
        list->length --;
        return;
    }
}

void insertNodeFront(List* list, ListNode* node){
    if (list->length == 0){
        node->prev = NULL;
        node->next = NULL;
        list->front = node;
        list->back = node;
    }
    else{
        node->prev = NULL;
        node->next = list->front;
        list->front->prev = node;
        list->front = node;
    }
    list->length = list->length + 1;
}

ListNode* removeNodeBack (List* list){
    //wrapper for remove from back
    //owner ship of data not in list
    ListNode* res = list->back;
    removeNode(list, res);
    return res;
}

ListNode* removeNodeFront(List* list){
    //wrapper for remove from front
    ListNode* res = list->front;
    removeNode(list, res);
    return res;
}
int  isListEmpty (List* list)
{
    if(list == NULL || list->front == NULL || list->back == NULL)
        return 1;
    return 0;
}
ListNode*  peekFront (List* list)
{
    if(list == NULL || list->front == NULL || list->back == NULL)
        return NULL;
    return list -> front;
}
ListNode*  peekBack (List* list)
{
    if(list == NULL || list->front == NULL || list->back == NULL)
        return NULL;
    return list -> back;
}
void printLinkedList(List *list, void(*printFunction)(void*))
{
    ListNode *node=list->front;
    while(node != NULL)
    {
        //printFunction(node);
        node=node->next;
    }
}