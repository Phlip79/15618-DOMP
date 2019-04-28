//
// Created by Apoorv Gupta on 4/21/19.

// Majority of the code hasn been taken from https://www.mpi-forum.org/docs/mpi-3.1/mpi31-report/node248.htm
//

#include "MPIServer.h"
#include <mpi.h>
#include <thread>

using namespace domp;
namespace domp {

  MPIServer::MPIServer(DOMP *dompObject, char* clusterName, int clusterSize, int rank) {
    this->dompObject = dompObject;
    strncpy(this->clusterName, clusterName, DOMP_MAX_CLUSTER_NAME);
    this->clusterSize = clusterSize;
    this->rank = rank;
    this->mapRequest = new DOMPMapRequest_t();
    nodeConnections = new MPI_Comm[this->rank];
    MPI_Comm_dup(MPI_COMM_WORLD, &mpi_comm);
  }

  MPIServer::~MPIServer() {
    MPI_Comm_free(&mpi_comm);
  }


  void MPIServer::requestData(std::string varName, int start, int size, MPIAccessType accessType) {
    // Keep accumulating all data requests. Send it at once in triggerMap function() called when synchronize is called
    // Thread-safety not required. Assuming that caller is calling this function sequentially
    DOMPMapCommand_t command;
    command.varName = varName;
    command.accessType = accessType;
    command.size = size;
    command.start = start;
    mapRequest->commands.push_back(command);
  }

  void MPIServer::handleMapResponse(MPI_Status *status) {
    if (status->MPI_ERROR == MPI_SUCCESS && status->MPI_SOURCE == 0) {
      int count;
      MPI_Get_count(status, MPI_BYTE, &count);
      auto *buffer = new char[count];
      MPI_Recv(buffer, count, MPI_BYTE, status->MPI_SOURCE, status->MPI_TAG, mpi_comm, NULL);
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
    }
  }

  void MPIServer::triggerMap() {
    dataReceived = false;
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

  void MPIMasterServer::triggerMap() {
//    dataReceived = false;
//    ssize_t  size = mapRequest->commands.size() * sizeof(DOMPMapCommand_t);
//    char buffer[size];
//
//    std::list<DOMPMapCommand_t>::iterator it;
//    int i = 0;
//    for (it = mapRequest->commands.begin(); it != mapRequest->commands.end(); ++it){
//      memcpy(&buffer[i * sizeof(DOMPMapCommand_t)], &(*it)  , sizeof(DOMPMapCommand_t));
//    }
//
//    // Send the MAP request to Master node always. Use the already created connection
//    MPI_Send(buffer, size, MPI_BYTE, 0, MPI_MAP_REQ, mpi_comm);
//    // Clear the commands now
//    mapRequest->commands.clear();
//
//    MPI_Status status;
//    MPI_Probe(0, MPI_MAP_RESP, mpi_comm, &status);
//
//    handleMapResponse(&status);
//
//    // Synchronization is must here as all nodes should receive and send the shared data
//    MPI_Barrier(MPI_COMM_WORLD);
  }

  void MPIServer::handleMapRequest(MPI_Status* status) {
    // Do nothing
  }

  void MPIMasterServer::handleMapRequest(MPI_Status* status) {
    if (status->MPI_ERROR == MPI_SUCCESS) {
      int count;
      MPI_Get_count(status, MPI_BYTE, &count);
      char *buffer = new char[count];
      MPI_Recv(buffer, count, MPI_BYTE, status->MPI_SOURCE, status->MPI_TAG, mpi_comm, NULL);
      commands_received.push_back(std::make_pair(status->MPI_SOURCE,(DOMPMapCommand_t*)buffer));

      mappingMtx.lock();
      if (mapReceived == (clusterSize - 1)) {
        // Respond to all slaves
        mappingMtx.unlock();


      } else {
        mapReceived ++;
        mappingMtx.unlock();
      }
    }
  }
}
