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


  void MPIServer::accept() {
    while (true) {
      log("Node %d waiting for a request\b", rank);
      MPI_Status *status = new MPI_Status();
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, mpi_comm, status);
      log("Node %d received a request\b", rank);
      if (status->MPI_TAG == MPI_EXIT_CMD) {
        free(status);
        log("Node %d MPI Server exiting\n", rank);
        break;
      }
      // Handle in a new thread
      // TODO: Consider using a threadpool instead of spawning thread every time
      std::thread(&MPIServer::handleRequest, this, status);
    }
    serverRunning = false;
    exitCondv.notify_all();
  }


  void MPIServer::handleRequest(MPI_Status *status) {
    if (status->MPI_ERROR == MPI_SUCCESS) {
      switch (status->MPI_TAG) {
        case MPI_MAP_REQ:
          if (rank != 0) {
            std::cout<<"ERROR::Node "<<rank<<" received map request"<<std::endl;
            // TODO: Please see if we should respond with an error message
          }
          else {
            handleMapRequest(status);
          }
          break;
        case MPI_MAP_RESP:
          handleMapResponse(status);
          break;
        case MPI_DATA_CMD:
          transferData(status);
          break;
        default:
          // Handle data receive and synchronization here
          receiveData(status);
          break;
      }
    }
    delete (status);
  }

  void MPIServer::startServer() {
    serverRunning = true;
    // First start your own server thread
     serverThread = std::thread(&MPIServer::accept, this);
  }

  void MPIServer::stopServer() {
    // Inform the PROBE to finish using EXIT CMD
    MPI_Send(NULL, 0, MPI_BYTE, rank, MPI_EXIT_CMD, mpi_comm);
    std::unique_lock<std::mutex> lck(dataMtx);
    log("Node %d MPI sent exit request\n", rank);
    // Wait for server to notify that it exited. It is required otherwise MPI_Finalize will cause errors
    exitCondv.wait(lck, [this]{return !serverRunning;});
    log("Node %d MPI received exit ack\n", rank);
    serverThread.detach();
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
      int requests = count / sizeof(DOMPMapCommand_t);
      int tag = DOMP_MIN_DATA_TAG;
      for(int i = 0; i < requests; i++) {
        auto *command = reinterpret_cast<DOMPMapCommand_t *>(buffer + i *sizeof(DOMPMapCommand_t));
        // Send the Data request to slave nodes. Use already created connection
        MPI_Isend(command, sizeof(DOMPMapCommand_t), MPI_BYTE, command->nodeId, tag, mpi_comm, NULL);
        // Keep track of all requests
        dataRequests[tag++] = command;
      }
      // If I had nothing to receive, notify the waiting master thread to continue
      std::unique_lock<std::mutex> lck(dataMtx);
      if (requests == 0) {
        dataReceived = true;
        dataCV.notify_all();
      }
    }
  }

  void MPIServer::transferData(MPI_Status *status) {
    if (status->MPI_ERROR == MPI_SUCCESS) {
      DOMPMapCommand_t command;
      MPI_Recv(&command, sizeof(DOMPMapCommand_t), MPI_BYTE, status->MPI_SOURCE, status->MPI_TAG, mpi_comm, NULL);
      std::pair<void*, int> ret = dompObject->mapDataRequest(command.varName, command.start, command.size);
      auto buffer = ret.first;
      // Someone requested data from me. Read and send the data to the specified node
      MPI_Send(buffer, ret.second, MPI_BYTE, status->MPI_SOURCE, status->MPI_TAG, mpi_comm);
    }
  }


  void MPIServer::receiveData(MPI_Status *status) {
    if (status->MPI_ERROR == MPI_SUCCESS) {
      bool isLastReceived = false;
      DOMPMapCommand_t *command = NULL;
      // Get the corresponding command
      {
        std::unique_lock<std::mutex> lck(dataMtx);
        if (dataRequests.count(status->MPI_TAG) != 0) {
          command = dataRequests[status->MPI_TAG];
          dataRequests.erase(status->MPI_TAG);
          if (dataRequests.size() == 0) {
            isLastReceived = true;
          }
        } else {
          std::cout<<"Tag is not found"<< std::endl;
        }
      }
      // Wait for the data transfer to finish
      if (command != NULL) {
        std::pair<void *, int> ret = dompObject->mapDataRequest(command->varName, command->start, command->size);
        MPI_Recv(ret.first, ret.second, MPI_BYTE, status->MPI_SOURCE, status->MPI_TAG, mpi_comm, NULL);
        // Now let the main thread know that we have received all data
        if (isLastReceived) dataReceived = true;
        dataCV.notify_all();
      }
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
    // Wait for the data transfer to finish
    std::unique_lock<std::mutex> lck(dataMtx);
    while (!dataReceived) dataCV.wait(lck);
    // Synchronization is must here
    MPI_Barrier(MPI_COMM_WORLD);
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
