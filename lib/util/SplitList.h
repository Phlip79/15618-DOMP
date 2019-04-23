//
// Created by Apoorv Gupta on 4/22/19.
//

#ifndef DOMP_SPLITTEST_H
#define DOMP_SPLITTEST_H

#include <list>
#include "../domp.h"
#include "../MPIServer.h"

// One function works for all data types.  This would work
// even for user defined types if operator '>' is overloaded

using namespace std;

namespace domp {

  enum SplitListAccessType {EXCLUSIVE, SHARED};
  enum SplitListUseNode {USE_FIRST, USE_SECOND};

  #define SPLIT_ACCESS(accessType) (IS_EXCLUSIVE(accessType)?EXCLUSIVE:SHARED)

   class SplitList {
    private:
     Fragment* split(Fragment *current, int splitPoint, int nodeId, SplitListAccessType accessType, SplitListUseNode useNode);
     DOMPMapCommand_t* createCommand(Fragment *fragment, std::string varName);
     DoublyLinkedList<Fragment> fragments;
    public:
      SplitList();
      std::list<DOMPMapCommand_t*> ReadPhase(DOMPMapCommand_t command);
      void WritePhase(DOMPMapCommand_t command);
   };
}
#endif //DOMP_SPLITTEST_H
