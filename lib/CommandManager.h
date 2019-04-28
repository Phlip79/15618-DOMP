//
// Created by Apoorv Gupta on 4/28/19.
//

#ifndef DOMP_COMMANDMANAGER_H
#define DOMP_COMMANDMANAGER_H

#include <list>
#include <map>
#include <string>

#include "domp.h"

using namespace std;

namespace domp {
  class CommandManager;
  class DOMPDataCommand;

  enum MPIAccessType {MPI_SHARED_FETCH= 0, MPI_EXCLUSIVE_FETCH, MPI_SHARED_FIRST, MPI_EXCLUSIVE_FIRST};
  typedef struct DOMPMapCommand {
      char varName[DOMP_MAX_VAR_NAME];
      int start;
      int size;
      MPIAccessType accessType;
      int nodeId;
    } DOMPMapCommand_t;
}

class domp::CommandManager {
  std::map<int, std::list<DOMPDataCommand*>*> commandMap;
  std::map<int, int> tagvalues;
  int clusterSize;

 public:
  CommandManager(int clusterSize);
  ~CommandManager();
  std::pair<char *, int> GetCommands(int rank);
  void InsertCommand(char* varName, int start, int size, int source, int destination);
  void ReInitialize();
};

#endif //DOMP_COMMANDMANAGER_H
