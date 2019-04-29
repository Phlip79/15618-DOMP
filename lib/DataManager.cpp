//
// Created by Apoorv Gupta on 4/21/19.

// Majority of the code hasn been taken from https://www.mpi-forum.org/docs/mpi-3.1/mpi31-report/node248.htm
//

#include "DataManager.h"
#include "CommandManager.h"
#include <mpi.h>
#include <thread>
using namespace domp;
namespace domp {

  DataManager::DataManager(DOMP *dompObject, int clusterSize, int rank) {
    this->dompObject = dompObject;
    this->clusterSize = clusterSize;
    this->rank = rank;
    MPI_Comm_dup(MPI_COMM_WORLD, &mpi_comm);
  }

  DataManager::~DataManager() {
    MPI_Comm_free(&mpi_comm);
  }

  MasterDataManager::~MasterDataManager() {
    // Free the memory for variables
    for (std::map<std::string,MasterVariable*>::iterator it=varList.begin(); it!=varList.end(); ++it)
      delete(it->second);
    delete(commandManager);
  }

  void DataManager::requestData(std::string varName, int start, int size, MPIAccessType accessType) {
    // Keep accumulating all data requests. Send it at once in triggerMap function() called when synchronize is called
    // Thread-safety not required. Assuming that caller is calling this function sequentially
    auto command = new DOMPMapCommand_t();
    strncpy(command->varName, varName.c_str(), varName.size());
    command->accessType = accessType;
    command->size = size;
    command->start = start;
    command->nodeId = rank;
    mapRequest.push_back(command);
    log("Node %d:: Added request var[%s], start=%d, size=%d", rank, command->varName, start, size);
  }

  void DataManager::handleMapResponse(char* buffer, int count) {
      int numRequests = count / sizeof(DOMPDataCommand_t);
      log("Node %d::Data request response received with %d requests.", rank, numRequests);
      MPI_Request *requests = new MPI_Request[numRequests];
      MPI_Status *status = new MPI_Status[numRequests];
      for(int i = 0; i < numRequests; i++) {
        auto *command = reinterpret_cast<DOMPDataCommand_t *>(buffer + i *sizeof(DOMPDataCommand_t));
        std::pair<char*, int> ret = dompObject->mapDataRequest(command->varName, command->start, command->size);
        if (command->commandType == MPI_DATA_FETCH) {
          log("Node %d::[%d] DATAFETCH Var[%s], start[%d], size[%d], bytes[%d], tag[%d] Address[%p] Node[%d]", rank, i,
              command->varName, command->start, command->size, ret.second, command->tagValue, ret.first, command->nodeId);
          // Wait for the data to receive
          MPI_Irecv(ret.first, ret.second, MPI_BYTE, command->nodeId, command->tagValue, mpi_comm, &requests[i]);
        }
        else {
          // Send the Data request to slave nodes. Use already created connection
          log("Node %d::[%d] DATASEND Var[%s], start[%d], size[%d], bytes[%d], tag[%d] Address[%p] Node[%d]", rank, i,
              command->varName, command->start, command->size, ret.second, command->tagValue, ret.first, command->nodeId);
          MPI_Isend(ret.first, ret.second, MPI_BYTE, command->nodeId, command->tagValue, mpi_comm, &requests[i]);
        }
      }

      if(MPI_Waitall(numRequests , requests, status) == MPI_ERR_IN_STATUS) {
        log("ERROR::Waitall failed");
      }

      for(int i = 0; i < numRequests; i++) {
        if (status[i].MPI_ERROR != MPI_SUCCESS) {
          log("Command with %d failed with error code %ld", i, status[i].MPI_ERROR);
        }
      }

      delete(requests);
  }

  void DataManager::triggerMap() {

    ssize_t  size = mapRequest.size() * sizeof(DOMPMapCommand_t);
    char buffer[size];

    std::list<DOMPMapCommand_t*>::iterator it;
    int i = 0;
    for (it = mapRequest.begin(); it != mapRequest.end(); ++it){
      DOMPMapCommand_t *cmd = *it;
      memcpy(&buffer[i * sizeof(DOMPMapCommand_t)], cmd, sizeof(DOMPMapCommand_t));
      i++;
      delete(cmd);
    }

    // Send the MAP request to Master node always. Use the already created connection
    MPI_Send(buffer, size, MPI_BYTE, 0, MPI_MAP_REQ, mpi_comm);
    // Clear the commands now
    mapRequest.clear();

    MPI_Status status;
    MPI_Probe(0, MPI_MAP_RESP, mpi_comm, &status);

    if (status.MPI_ERROR == MPI_SUCCESS && status.MPI_SOURCE == 0) {
      int count;
      MPI_Get_count(&status, MPI_BYTE, &count);
      auto *buffer = new char[count];
      MPI_Recv(buffer, count, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, mpi_comm, NULL);
      handleMapResponse(buffer, count);
      delete(buffer);
    }


    // Synchronization is must here as all nodes should receive and send the shared data
    MPI_Barrier(MPI_COMM_WORLD);
  }

  void MasterDataManager::triggerMap() {
    // Master node directly pushes its own command to the list
    std::list<DOMPMapCommand_t*>::iterator it;
    for (it = mapRequest.begin(); it != mapRequest.end(); ++it){
      DOMPMapCommand_t *command = *it;
      log("MASTER::Received request Node[%d], varName[%s], start[%d], size[%d]",command->nodeId, command->varName,
          command->start, command->size);
      commands_received.push_back(command);
    }

    mapRequest.clear();

    log("MASTER::Starting receiving requests ");
    // Master doesn't send the request to itself. It waits for a message from all other nodes
    MPI_Status status;
    int requestReceived = 1;
    while (requestReceived != clusterSize) {
      MPI_Probe(MPI_ANY_SOURCE, MPI_MAP_REQ, mpi_comm, &status);
      handleMapRequest(&status);
      requestReceived++;
    }
    // Perform the mapping here

    log("MASTER::Starting applying requests");
    std::list<DOMPMapCommand_t*>::iterator commandIterator;
    for(commandIterator = commands_received.begin(); commandIterator != commands_received.end(); ++commandIterator) {
      auto command = *commandIterator;
      if (0 == varList.count(std::string(command->varName))){
        log("MASTER::Variable %s not found", command->varName);
        MPI_Abort(MPI_COMM_WORLD, DOMP_VAR_NOT_FOUND_ON_MASTER);
      }
      log("MASTER::Applying command for nodeId %d", command->nodeId);
      MasterVariable *masterVariable = varList[command->varName];
      masterVariable->applyCommand(commandManager, command);
      delete(command);
    }

    log("MASTER::Starting sending commands");

    int index = 1;
    MPI_Request *requests = new MPI_Request[clusterSize - 1];
    while (index != clusterSize) {
      std::pair<char*, int> data = commandManager->GetCommands(index);
      log("MASTER sending commands for size %d to nodeId %d", data.second/sizeof(DOMPDataCommand_t), index);
      MPI_Isend(data.first, data.second, MPI_BYTE, index, MPI_MAP_RESP, mpi_comm, &requests[index-1]);
      index++;
      delete(data.first);
    }
    MPI_Waitall(clusterSize - 1 , requests, MPI_STATUSES_IGNORE);
    index = 1;
    while (index != clusterSize) {
      std::pair<char*, int> data = commandManager->GetCommands(index);
      // Release the buffer
      delete(data.first);
      index++;
    }
    delete (requests);
    commands_received.clear();

    log("MASTER::Starting applying its own commands");

    // Process the commands for master node
    std::pair<char*, int> data = commandManager->GetCommands(rank);
    char* buffer = data.first;
    handleMapResponse(buffer, data.second);
    delete(buffer);

    commandManager->ReInitialize();

    log("MASTER::Completed applying its own commands");

    // Synchronization is must here as all nodes should receive and send the shared data
    MPI_Barrier(MPI_COMM_WORLD);
  }

  void MasterDataManager::handleMapRequest(MPI_Status* status) {
    if (status->MPI_ERROR == MPI_SUCCESS) {
      int count;
      MPI_Get_count(status, MPI_BYTE, &count);
      char *buffer = new char[count];
      MPI_Recv(buffer, count, MPI_BYTE, status->MPI_SOURCE, status->MPI_TAG, mpi_comm, NULL);
      // Now process all requests one at a time
      int numRequests = count / sizeof(DOMPMapCommand_t);
      log("MASTER::Received %d requests from node %d", numRequests, status->MPI_SOURCE);
      for (int i = 0; i < numRequests; i++) {
        auto cmd = new DOMPMapCommand_t();
        memcpy(cmd, buffer + i *sizeof(DOMPMapCommand_t), sizeof(DOMPMapCommand_t));
        log("MASTER::Received request Node[%d], varName[%s], start[%d], size[%d]",status->MPI_SOURCE, cmd->varName,
            cmd->start, cmd->size);
        commands_received.push_back(cmd);
      }
      delete(buffer);
      log("MASTER::Added %d requests from node %d", numRequests, status->MPI_SOURCE);
    }
  }

  void DataManager::registerVariable(std::string varName, Variable *variable) {
    // Do nothing on regular ndoes
  }

  void MasterDataManager::registerVariable(std::string varName, Variable *variable) {
    // Register to your own mapping
    if (varList.count(varName) != 0) {
      delete(varList[varName]);
    }
    varList[varName] = new MasterVariable(variable->getPtr(), variable->getSize());
  }

}
