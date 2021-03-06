//
// Created by Apoorv Gupta on 4/22/19.
//

#include <iostream>
#include "SplitList.h"
#include "../domp.h"

using namespace std;

namespace domp {
  SplitList::SplitList(int start, int size, int nodeId) {
    fragments.InsertFront(new Fragment(start, size, nodeId));
  }

  SplitList::~SplitList() {
    Fragment *current = fragments.begin();
    while(current != NULL) {
      Fragment *next = current->next;
      delete(current);
      current = next;
    }
  }


  // This is the read phase. This is where we read where data existed in previous phase, so that we can read it from
  // that node. In this function, we also perform the splitting.
  void SplitList::ReadPhase(DOMPMapCommand_t *command, CommandManager *commandManager) {
    int start = command->start;
    int end = command->start + command->size - 1;
    int nodeId = command->nodeId;
    MPIAccessType  accessType = command->accessType;
    char* varName = command->varName;

    log("READPHASE::Start[%d], End[%d], NodeId[%d], VarName[%s]", start, end, nodeId, varName);

    Fragment *current = fragments.begin();
    while(current != NULL && start <= end) {
      // Case 1: ||
      if (start > current->end) {
        log("Found Case 1:: Required[%d, %d] Current[%d, %d]", start, end, current->start, current->end);
        current = current->next;
        continue;
      }
      // Case 2: |X||
      if (start == current->start && end < current->end) {
        log("Found Case 2:: Required[%d, %d] Current[%d, %d]", start, end, current->start, current->end);
        Fragment* nextNode = Split(current, end, nodeId, SPLIT_ACCESS(accessType), USE_FIRST);
        if (nextNode != NULL && IS_FETCH(accessType)) {
          CreateCommand(commandManager, nodeId, current, varName);
        }
        break;
      }
      // Case 3: ||X|
      else if (start > current->start && end >= current->end) {
        // Same case for both Exclusive fetch and shared fetch
        log("Found Case 3:: Required[%d, %d] Current[%d, %d]", start, end, current->start, current->end);
        Fragment* nextNode = Split(current, start - 1, nodeId, SPLIT_ACCESS(accessType), USE_SECOND);
        if (nextNode != NULL && IS_FETCH(accessType)) {
          CreateCommand(commandManager, nodeId, nextNode, varName);
          start = nextNode->end + 1;
          current = nextNode;
        } else {
          start = current->end + 1;
        }
      }
      // Case 4: ||X||
      else if (start > current->start && end < current->end) {
        // Same case for both Exclusive fetch and shared fetch
        // We need to Split twice
        // First Split using second
        log("Found Case 4:: Required[%d, %d] Current[%d, %d]", start, end, current->start, current->end);
        Fragment* nextNode = Split(current, start - 1, nodeId, SPLIT_ACCESS(accessType), USE_SECOND);
        if (nextNode != NULL) {
          // Second Split using first. See last argument as true here.
          Fragment* nextNextNode = Split(nextNode, end, nodeId, SPLIT_ACCESS(accessType), USE_FIRST);
          if (nextNextNode != NULL && IS_FETCH(accessType)) {
            CreateCommand(commandManager, nodeId, nextNode, varName);
          }
        }
        break;
      }
      // Case 5: |X|
      else if (start == current->start && end >= current->end) {
        log("Found Case 5:: Required[%d, %d] Current[%d, %d]", start, end, current->start, current->end);
        if (current->nodes.count(nodeId) == 0) {
          if (IS_FETCH(accessType)) {
            CreateCommand(commandManager, nodeId, current, varName);
          }
        }
        // Update start
        start = current->end + 1;
      }
      current = current->next;
    }

    log("READPHASE finished::Start[%d], End[%d], NodeId[%d], VarName[%s]", start, end, nodeId, varName);
  }


  Fragment* SplitList::Split(Fragment *current,
                             int splitPoint,
                             int nodeId,
                             SplitListAccessType accessType,
                             SplitListUseNode useNode) {
    if (accessType != EXCLUSIVE) {
      // If not exclusive and already have it, don't Split
      if (current->nodes.count(nodeId) != 0) return NULL;
    }
    // Create a copy of the fragment
    Fragment* fragment = new Fragment(current);

    fragment->update(splitPoint+1, current->end);
    current->update(current->start, splitPoint);
    fragments.InsertAfter(current, fragment);
    return fragment;
  }

  void SplitList::CreateCommand(CommandManager *commandManager, int destination, Fragment *fragment, char* varName) {
    // Logic from which node to fetch the data. We can distribute it to multiple nodes, if multiple nodes have the data
    int source = *fragment->nodes.begin();
    log("MASTER:: Created fetch command Var[%s], From[%d] TO[%d], Start[%d], Size[%d]", varName, source, destination,
        fragment->start, fragment->size);
    commandManager->InsertCommand(varName, fragment->start, fragment->size, source, destination);
  }

  // This is the write phase. This is when the new nodeIds will be added and previous nodeIds will be deleted for
  // exclusive nodes
  void SplitList::WritePhase(DOMPMapCommand_t *command) {
    // For write phase, Split should have happened already
    int start = command->start;
    int end = command->start + command->size - 1;
    int nodeId = command->nodeId;
    MPIAccessType  accessType = command->accessType;
    char* varName = command->varName;
    std::list<DOMPMapCommand_t*> commands;

    log("WritePhase::Start[%d], End[%d], NodeId[%d], VarName[%s]", start, end, nodeId, varName);

    Fragment *current = fragments.begin();
    while(current != NULL && start <= end) {
      // Greater than as the requested interval could span multiple intervals
      if (start == current->start && end >= current->end) {
        // TODO: We should do coalesce here
        if (IS_EXCLUSIVE(accessType)) {
          current->nodes.clear();
        }
        if (current->nodes.count(nodeId) == 0) {
          current->nodes.insert(nodeId);
          log("WritePhase::Inserted New Node for Start[%d], End[%d], NodeId[%d], VarName[%s]", start, end, nodeId, varName);
        }
        start = current->end + 1;
      }
      else if (start > current->end) {
        // Nothing to do
      } else {
        std::cout<<"ERROR: This shouldn't have happened. Writephase, no interval found"<<std::endl;
        break;
      }
      current = current->next;
    }

    log("WritePhase finished::Start[%d], End[%d], NodeId[%d], VarName[%s]", start, end, nodeId, varName);
  }
};
