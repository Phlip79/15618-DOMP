//
// Created by Apoorv Gupta on 4/28/19.
//

#include "DataManager.h"
#include "CommandManager.h"

#include <utility>
namespace domp {
  CommandManager::CommandManager(int clusterSize) {
    this->clusterSize = clusterSize;
    for(int i = 0; i < clusterSize; i++) {
      tagvalues[i] = DOMP_MIN_DATA_TAG;
      commandMap[i] = new list<DOMPDataCommand_t*>();
    }
  }
  CommandManager::~CommandManager() {
    // TO ensure the memory is freed
    ReInitialize();
    for(int i = 0; i < clusterSize; i++) {
      delete(commandMap[i]);
    }
  }
  std::pair<char *, int> CommandManager::GetCommands(int rank) {
    list<DOMPDataCommand_t *> *commandList = commandMap[rank];
    int size = commandList->size() * sizeof(DOMPDataCommand_t);
    auto *buffer = new char[size];
    int i = 0;
    for(std::list<DOMPDataCommand_t*>::iterator it = commandList->begin(); it != commandList->end(); ++it) {
     memcpy(buffer + i * sizeof(DOMPDataCommand_t), *it, sizeof(DOMPDataCommand_t));
     i++;
    }
    return std::make_pair(buffer, size);
  };
  void CommandManager::InsertCommand(char* varName, int start, int size, int source, int destination) {
    auto destinationCommand = new DOMPDataCommand_t();
    auto sourceCommand = new DOMPDataCommand_t();

    strncpy(destinationCommand->varName, varName, DOMP_MAX_VAR_NAME);
    strncpy(sourceCommand->varName, varName, DOMP_MAX_VAR_NAME);
    destinationCommand->size = sourceCommand->size = size;
    destinationCommand->start = sourceCommand->start = start;
    destinationCommand->nodeId = source;
    sourceCommand->nodeId = destination;

    destinationCommand->commandType = MPI_DATA_FETCH;
    sourceCommand->commandType = MPI_DATA_SEND;

    int myTag = tagvalues[destination]++;
    sourceCommand->tagValue = destinationCommand->tagValue = myTag;


    commandMap[source]->push_back(sourceCommand);
    commandMap[destination]->push_back(destinationCommand);

    if (source ==0 || destination == 0) {
      log("Inserted commmand for rank 0 and current size is %d");
    }

  }

  void CommandManager::ReInitialize() {
    // Reinitialize the datastructure now
    for(int i = 0; i < clusterSize; i++) {
      tagvalues[i] = DOMP_MIN_DATA_TAG;
      list<DOMPDataCommand_t *> *commandList = commandMap[i];
      for(std::list<DOMPDataCommand_t*>::iterator it = commandList->begin(); it != commandList->end(); it++) {
        delete(*it);
      }
      commandMap[i]->clear();
    }
  }
}