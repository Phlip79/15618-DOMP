//
// Created by Apoorv Gupta on 4/20/19.
//

#ifndef DOMP_LINKEDLIST_H
#define DOMP_LINKEDLIST_H

#include <ctype.h>
#include <stdint.h>
#include <stddef.h>

namespace domp {
template <typename T> class DoublyLinkedList;
}

template <class T>
class domp::DoublyLinkedList {
  uint32_t length; // <! clusterSize of list
  T* front; // <! pointer first node
  T* back; // <! pointer last node
 public:
  DoublyLinkedList() {
    front = back = NULL;
    length = 0;
  }

  void InsertAfter(T* current, T* node) {
    T* next = current->next;
    node->prev = current;
    node->next = next;
    current->next = node;
    if (next != NULL)
      next->prev = node;
    // Update back if it was last node
    if (current == back) {
      back = node;
    }
    length +=1;
  }

  void InsertFront(T* node) {
    if (length == 0){
      node->prev = NULL;
      node->next = NULL;
      front = node;
      back = node;
    }
    else{
      node->prev = NULL;
      node->next = front;
      front->prev = node;
      front = node;
    }
    length = length + 1;
  }


  T* begin() {
    return front;
  }

  T* end() {
    return back;
  }

  bool IsEmpty() {
    return (length == 0);
  }
};



#endif //DOMP_LINKEDLIST_H
