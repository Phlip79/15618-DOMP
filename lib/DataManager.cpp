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
    this->mapRequest = new DOMPMapRequest_t();
    MPI_Comm_dup(MPI_COMM_WORLD, &mpi_comm);
  }

  DataManager::~DataManager() {
    MPI_Comm_free(&mpi_comm);
    delete(this->mapRequest);
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
    DOMPMapCommand_t command;
    command.varName = varName;
    command.accessType = accessType;
    command.size = size;
    command.start = start;
    command.nodeId = rank;
    mapRequest->commands.push_back(command);
  }

  void DataManager::handleMapResponse(MPI_Status *status) {
    if (status->MPI_ERROR == MPI_SUCCESS && status->MPI_SOURCE == 0) {
      int count;
      MPI_Get_count(status, MPI_BYTE, &count);
      auto *buffer = new char[count];
      MPI_Recv(buffer, count, MPI_BYTE, status->MPI_SOURCE, status->MPI_TAG, mpi_comm, NULL);
      log("SLAVE %d::Data request response received.", rank);
      int numRequests = count / sizeof(DOMPMapCommand_t);
      MPI_Request *requests = new MPI_Request[numRequests];
      for(int i = 0; i < numRequests; i++) {
        auto *command = reinterpret_cast<DOMPDataCommand_t *>(buffer + i *sizeof(DOMPDataCommand_t));
        std::pair<void*, int> ret = dompObject->mapDataRequest(command->varName, command->start, command->size);
        if (command->commandType == MPI_DATA_FETCH) {
          // Wait for the data to receive
          MPI_Irecv(ret.first, ret.second, MPI_BYTE, command->nodeId, command->tagValue, mpi_comm, &requests[i]);
        }
        else if(command->commandType == MPI_DATA_SEND) {
          // Send the Data request to slave nodes. Use already created connection
          MPI_Isend(ret.first, ret.second, MPI_BYTE, command->nodeId, command->tagValue, mpi_comm, &requests[i]);
        }
      }

      MPI_Waitall(numRequests , requests, NULL);
      delete(buffer);
      delete(requests);
    }
  }

  void DataManager::triggerMap() {

    ssize_t  size = mapRequest->commands.size() * sizeof(DOMPMapCommand_t);
    char buffer[size];

    std::list<DOMPMapCommand_t>::iterator it;
    int i = 0;
    for (it = mapRequest->commands.begin(); it != mapRequest->commands.end(); ++it){
      memcpy(&buffer[i * sizeof(DOMPMapCommand_t)], &(*it)  , sizeof(DOMPMapCommand_t));
    }

    // Send the MAP request to Master node always. Use the already created connection
    MPI_Send(buffer, size, MPI_BYTE, 0, MPI_MAP_REQ, mpi_comm);
    // Clear the commands now
    mapRequest->commands.clear();

    MPI_Status status;
    MPI_Probe(0, MPI_MAP_RESP, mpi_comm, &status);

    handleMapResponse(&status);

    // Synchronization is must here as all nodes should receive and send the shared data
    MPI_Barrier(MPI_COMM_WORLD);
  }

  void MasterDataManager::triggerMap() {
    // Master node directly pushes its own command to the list
    std::list<DOMPMapCommand_t>::iterator it;
    for (it = mapRequest->commands.begin(); it != mapRequest->commands.end(); ++it){
      commands_received.push_back(*it);
    }

    // Master doesn't send the request to itself. It waits for a message from all other nodes
    MPI_Status status;
    int requestReceived = 1;
    while (requestReceived != clusterSize) {
      MPI_Probe(MPI_ANY_SOURCE, MPI_MAP_REQ, mpi_comm, &status);
      handleMapRequest(&status);
      requestReceived++;
      log("MASTER::Data request received from %d", status.MPI_SOURCE);
    }
    // Perform the mapping here

    std::list<DOMPMapCommand_t>::iterator commandIterator;
    for(commandIterator = commands_received.begin(); commandIterator != commands_received.end(); commandIterator++) {
      DOMPMapCommand_t command = *commandIterator;
      MasterVariable *masterVariable = varList[command.varName];
      masterVariable->applyCommand(commandManager, &command);
    }

    int index = 1;
    MPI_Request *requests = new MPI_Request[clusterSize - 1];
    while (index != clusterSize) {
      std::pair<char*, int> data = commandManager->GetCommands(index);
      MPI_Isend(data.first, data.second, MPI_BYTE, index, MPI_MAP_RESP, mpi_comm, &requests[index-1]);
      index++;
    }
    MPI_Waitall(clusterSize - 1 , requests, NULL);
    // Process the commands for master node

    commandManager->ReInitialize();
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
      for (int i = 0; i < numRequests; i++) {
        auto *command = reinterpret_cast<DOMPMapCommand_t *>(buffer + i *sizeof(DOMPMapCommand_t));
        commands_received.push_back(*command);
      }
      delete(buffer);
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
