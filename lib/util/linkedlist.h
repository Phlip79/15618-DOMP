//
// Created by Apoorv Gupta on 4/20/19.
//

#ifndef DOMP_LINKEDLIST_H
#define DOMP_LINKEDLIST_H

#include <ctype.h>
#include <stdint.h>
/**
 * @brief      List node of double linked list
 */
typedef struct list_node ListNode;
struct list_node{
  void* data; // <! pointer representing data
  ListNode* next; // <! pointer to next node
  ListNode* prev; // <! pointer to previous node
};
/**
 * @brief      Header of double linked list
 */
typedef struct list_header List;
struct list_header{
  uint32_t length; // <! size of list
  ListNode* front; // <! pointer first node
  ListNode* back; // <! pointer last node
};

/**
 * @brief      Initialized allocated list header
 *
 * @param      head  stack / heap allocated list head
 */
void initList(List* head);

/**
 * @brief      Look up node with data where comparator returns 1
 *
 * @param      list  to be searched
 * @param      data  Data targert to be searched
 * @param[in]  cmp   Comparator function, return 1 on equals 0 otherwise
 *                    arg1 : target data   arg2 : data in list
 *
 * @return     pointer to list node if found, NULL if not found (NULL check is
 *             required)
 */
ListNode* getDataNode(List* list, void* data, int(*cmp)(void*, void*));

/**
 * @brief      Remove a node from middle of list
 *
 * @param      list  List header for removal
 * @param      node  Target node to be removed, will do nothing if given node
 *                   not in header list
 */
void removeNode(List* list, ListNode* node);

/*
 * @brief      Insert new node at beginning of list
 *
 * @param      list  List header for insertion
 * @param      node  List node for insertion
 */
void insertNodeFront(List* list, ListNode* node);

/**
 * @brief      Removes a node back.
 *
 * @param      list  The list header for removal
 *
 * @return     Node removed from list, NULL if list empty
 */
ListNode*  removeNodeBack (List* list);

/**
 * @brief      Removes a node front.
 *
 * @param      list  The list header for removal
 *
 * @return     Node removed from list, NULL if list empty
 */
ListNode*  removeNodeFront (List* list);

/**
 * @brief      Determines if linkedlist is empty.
 *
 * @param      list  The list header for test
 *
 * @return     1 if empty, otherwise 0
 */
int  isListEmpty (List* list);

/**
 * @brief      A function for freeing internal data structure of a node
 * @param      node  to be freed
 */
typedef void (*elem_free_fn)(ListNode* node);

/**
 * @brief      Helper function to print content of linkedlist
 *
 * @param      list  The list header for removal
 */
void printLinkedList(List *list, void(*printFunction)(void*));

/**
 * @brief      Returns the element from front of the queue without removing it
 *             from queue. If list is NULL or list is empty, it returns NULL
 *
 * @param      list  The list header for removal
 *
 * @return     If list is NULL or list is empty, it returns NULL. Otherwise it
 *             returns the front node from list.
 */
ListNode*  peekFront (List* list);
/**
 * @brief      Returns the element from back of the queue without removing it
 *             from queue. If list is NULL or list is empty, it returns NULL
 *
 * @param      list  The list header for removal
 *
 * @return     If list is NULL or list is empty, it returns NULL. Otherwise it
 *             returns the back node from list.
 */
ListNode*  peekBack (List* list);

#endif //DOMP_LINKEDLIST_H
