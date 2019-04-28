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
    }
  }
  CommandManager::~CommandManager() {
    // TO ensure the memory is freed
    ReInitialize();
  }
  std::pair<char *, int> CommandManager::GetCommands(int rank) {
    if (commandMap.count(rank) != 0) {
      list<DOMPDataCommand_t *> commandList = commandMap[rank];
      int size = commandList.size() * sizeof(DOMPDataCommand_t);
      auto *buffer = new char[size];
      int i = 0;
      for(std::list<DOMPDataCommand_t*>::iterator it = commandList.begin(); it != commandList.end(); it++) {
       memcpy(buffer + i * sizeof(DOMPDataCommand_t), *it, sizeof(DOMPDataCommand_t));
       i++;
      }
      return std::make_pair(buffer, size);
    }
    return std::make_pair(nullptr, 0);
  };
  void CommandManager::InsertCommand(std::string varName, int start, int size, int source, int destination) {
    auto destinationCommand = new DOMPDataCommand_t();
    auto sourceCommand = new DOMPDataCommand_t();

    destinationCommand->varName = sourceCommand->varName = std::move(varName);
    destinationCommand->size = sourceCommand->size = size;
    destinationCommand->start = sourceCommand->start = start;
    destinationCommand->nodeId = source;
    sourceCommand->nodeId = destination;

    destinationCommand->commandType = MPI_DATA_FETCH;
    sourceCommand->commandType = MPI_DATA_SEND;

    int myTag = tagvalues[destination]++;
    sourceCommand->tagValue = destinationCommand->tagValue = myTag;

    commandMap[source].push_back(sourceCommand);
    commandMap[destination].push_back(destinationCommand);
  }

  void CommandManager::ReInitialize() {
    // Reinitialize the datastructure now
    for(int i = 0; i < clusterSize; i++) {
      tagvalues[i] = DOMP_MIN_DATA_TAG;
      if (commandMap.count(i) != 0) {
        list<DOMPDataCommand_t *> commandList = commandMap[i];
        for(std::list<DOMPDataCommand_t*>::iterator it = commandList.begin(); it != commandList.end(); it++) {
          delete(*it);
        }
        commandMap[i].clear();
      }
    }
  }
}