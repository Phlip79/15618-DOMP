//
// Created by Apoorv Gupta on 4/22/19.
//

#include "SplitList.h"

using namespace std;

namespace domp {
  SplitList::SplitList() {
  }

  // This is the read phase. This is where we read where data existed in previous phase, so that we can read it from
  // that node. In this function, we also perform the splitting.
  std::list<DOMPMapCommand_t*> SplitList::ReadPhase(DOMPMapCommand_t command) {
    int start = command.start;
    int end = command.start + command.size;
    int nodeId = command.nodeId;
    MPIAccessType  accessType = command.accessType;
    std::string varName = command.varName;
    std::list<DOMPMapCommand_t*> commands;

    if(fragments.IsEmpty()) {
      auto fragment = new Fragment(start, command.size, nodeId);
      fragments.InsertFront(fragment);
      return commands;
    }

    Fragment *current = fragments.begin();
    while(current != fragments.end() || start > end) {
      // Case 1: ||
      if (start > current->end) {
        current = current->next;
        continue;
      }
      // Case 2: |X||
      if (start == current->start && end < current->end) {
        auto nextNode = split(current, end, nodeId, SPLIT_ACCESS(accessType),USE_FIRST);
        if (nextNode != NULL && IS_FETCH(accessType)) {
          commands.push_back(createCommand(current, varName));
        }
        break;
      }
      // Case 3: ||X|
      else if (start > current->start && end == current->end) {
        // Same case for both Exclusive fetch and shared fetch
        auto nextNode = split(current, end, nodeId, SPLIT_ACCESS(accessType),USE_SECOND);
        if (nextNode != NULL && IS_FETCH(accessType)) {
          commands.push_back(createCommand(nextNode, varName));
        }
        break;
      }
      // Case 4: ||X||
      else if (start > current->start && end < current->end) {
        // Same case for both Exclusive fetch and shared fetch
        // We need to split twice
        // First split using second
        auto nextNode = split(current, start, nodeId, SPLIT_ACCESS(accessType),USE_SECOND);
        if (nextNode != NULL) {
          // Second split using first. See last argument as true here.
          auto nextNextNode = split(nextNode, end, nodeId, SPLIT_ACCESS(accessType),USE_FIRST);
          if (nextNextNode != NULL) {
            commands.push_back(createCommand(nextNode, varName));
          }
        }
        break;
      }
      // Case 5: |X|
      else if (start == current->start && end >= current->end) {
        if (current->nodes.count(nodeId) == 0) {
          if (IS_FETCH(accessType)) {
            commands.push_back(createCommand(current, varName));
          }
        }
        // Update start
        start = current->end + 1;
      }
      current = current->next;
    }
    return commands;

  }


  Fragment* SplitList::split(Fragment *current, int splitPoint, int nodeId, SplitListAccessType accessType, SplitListUseNode useNode) {
    if (accessType != EXCLUSIVE) {
      // If not exclusive and already have it, don't split
      if (current->nodes.count(nodeId) != 0) return NULL;
    }
    // Create a copy of the fragment
    auto fragment = new Fragment(current);

    fragment->update(splitPoint+1, current->end);
    current->update(current->start, splitPoint);
    fragments.InsertAfter(current, fragment);
    return fragment;
  }

  DOMPMapCommand_t* SplitList::createCommand(Fragment *fragment, std::string varName) {
    DOMPMapCommand_t *command = new DOMPMapCommand_t();
    command->start = fragment->start;
    command->size = fragment->size;
    command->varName = varName;
    // Logic from which node to fetch the data. We can distribute it to multiple nodes, if multiple nodes have the data
    command->nodeId = *fragment->nodes.begin();
    return command;

  }

  // This is the write phase. This is when the new nodeIds will be added and previous nodeIds will be deleted for
  // exclusive nodes
  void SplitList::WritePhase(DOMPMapCommand_t command) {
    // For write phase, split should have happened already
    int start = command.start;
    int end = command.start + command.size;
    int nodeId = command.nodeId;
    MPIAccessType  accessType = command.accessType;
    std::string varName = command.varName;
    std::list<DOMPMapCommand_t*> commands;

    Fragment *current = fragments.begin();
    while(current != fragments.end() || start > end) {
      if (start == current->start && end == current->end) {
        if (IS_EXCLUSIVE(accessType)) {
          current->nodes.clear();
        }
        current->nodes.insert(nodeId);
        break;
      } else if (start > current->end) {
        // Nothing to do
      } else {
        std::cout<<"ERROR: This shouldn't have happened. Writephase, no interval found"<<std::endl;
        break;
      }
      current = current->next;
    }
  }
};
