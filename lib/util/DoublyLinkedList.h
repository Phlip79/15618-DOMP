//
// Created by Apoorv Gupta on 4/20/19.
//

#ifndef DOMP_LINKEDLIST_H
#define DOMP_LINKEDLIST_H

#include <ctype.h>
#include <stdint.h>
#include <stddef.h>

namespace domp {

template <typename T>
class DoublyLinkedList {
  uint32_t length; // <! size of list
  T* front; // <! pointer first node
  T* back; // <! pointer last node
 public:
  DoublyLinkedList();
  void InsertAfter(T* current, T* node);
  void InsertFront(T* node);
  T* begin();
  T* end();
  bool IsEmpty();
};
}

#endif //DOMP_LINKEDLIST_H
