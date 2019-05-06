//
// Created by Apoorv Gupta on 4/22/19.
//

#ifndef DOMP_SPLITTEST_H
#define DOMP_SPLITTEST_H

#include <list>
#include <string>
#include <set>
#include "DoublyLinkedList.h"
#include "../CommandManager.h"

// One function works for all data types.  This would work
// even for user defined types if operator '>' is overloaded

using namespace std;

namespace domp {

  enum SplitListAccessType {EXCLUSIVE, SHARED};
  enum SplitListUseNode {USE_FIRST, USE_SECOND};

#define IS_EXCLUSIVE(e) ((e == MPI_EXCLUSIVE_FETCH) ||(e == MPI_EXCLUSIVE_FIRST))
#define IS_FETCH(e) ((e == MPI_SHARED_FETCH) || (e == MPI_EXCLUSIVE_FETCH))

  class Fragment;
  template <typename T> class DoublyLinkedList;

  #define SPLIT_ACCESS(accessType) (IS_EXCLUSIVE(accessType)?EXCLUSIVE:SHARED)

   class SplitList {
    private:
     Fragment* Split(Fragment *current,
                     int splitPoint,
                     int nodeId,
                     SplitListAccessType accessType,
                     SplitListUseNode useNode);
     void CreateCommand(CommandManager *commandManager, int destination, Fragment *fragment, char* varName);
     DoublyLinkedList<Fragment> fragments;
    public:
      SplitList(int start, int size, int nodeId);
      ~SplitList();
      void ReadPhase(DOMPMapCommand_t *command, CommandManager *commandManager);
      void WritePhase(DOMPMapCommand_t *command);
   };
}

class domp::Fragment {
  int start;
  int size;
  int end;
  std::set <int> nodes;
  friend class SplitList;
  friend class DoublyLinkedList<Fragment>;
  Fragment *next;
  Fragment *prev;
 public:
  Fragment(int start, int size, int nodeId) {
    this->start = start;
    this->size = size;
    this->end = start + size - 1; // Notice -1
    next = prev = NULL;
    this->nodes.insert(nodeId);
  }

  Fragment(Fragment *from) {
    this->start = from->start;
    this->size = from->size;
    this->end = start + size -  1; // Notice -1
    next = prev = NULL;
    this->nodes.insert(from->nodes.begin(), from->nodes.end());
  }

  void addNode(int nodeId) {
    this->nodes.insert(nodeId);
  }

  void update(int start, int end) {
    this->start = start;
    this->end = end;
    this->size = end - start + 1;
  }
};

#endif //DOMP_SPLITTEST_H
