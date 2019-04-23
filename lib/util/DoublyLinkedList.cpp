//
// Created by Apoorv Gupta on 4/23/19.
//

#include "DoublyLinkedList.h"
#include "../domp.h"

namespace domp {
    template <class T>
    DoublyLinkedList<T>::DoublyLinkedList() {
        front = back = NULL;
        length = 0;
    }

    template <typename T>
    void DoublyLinkedList<T>::InsertAfter(T* current, T* node) {
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

    template <typename T>
    void DoublyLinkedList<T>::InsertFront(T* node) {
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


    template <typename T>
    T* DoublyLinkedList<T>::begin() {
      return front;
    }

    template <typename T>
    T* DoublyLinkedList<T>::end() {
      return back;
    }

    template <typename T>
    bool DoublyLinkedList<T>::IsEmpty() {
      return (length == 0);
    }
}