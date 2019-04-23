//
// Created by Apoorv Gupta on 4/21/19.

// Majority of the code hasn been taken from https://www.mpi-forum.org/docs/mpi-3.1/mpi31-report/node248.htm
//

#include "MPIServer.h"
#include <mpi.h>
#include <thread>

namespace domp {

  MPIServer::~MPIServer() {
  }

  void MPIServer::accept() {
    MPI_Open_port(MPI_INFO_NULL, port_name);
    char name[DOMP_MAX_CLIENT_NAME];
    snprintf(name, DOMP_MAX_CLIENT_NAME, "%s-%d", clusterName, rank);
    MPI_Publish_name(name, MPI_INFO_NULL, port_name);
    printf("Server for node %d available at %s\n", rank, port_name);
    while (1) {
      MPI_Comm *client = new MPI_Comm();
      MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD, client);
      handleRequest(client);
    }
  }


  void MPIServer::handleRequest(MPI_Comm *client) {
    MPI_Status status;
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, *client, &status);
    if (status.MPI_ERROR == MPI_SUCCESS) {
      switch (status.MPI_TAG) {
        case MPI_MAP_REQ:
          if (rank != 0) {
            std::cout<<"ERROR::Node "<<rank<<" received map request"<<std::endl;
            // TODO: Please see if we should respond with an error message
          }
          else {
            handleMapRequest(status, client);
          }
          break;
        case MPI_MAP_RESP:
          handleMapResponse(status, client);
          break;
        case MPI_DATA_CMD:
          transferData(status, client);
          break;
        default:
          // Handle data receive and synchronization here
          receiveData(status, client);
          break;
      }
    }
    MPI_Comm_disconnect(client);
    delete (client);
  }

  void MPIServer::startServer() {
    // First start your own server thread
    serverThread = std::thread(&MPIServer::accept, this);
    // Now create connection to all other threads
    char port_name[MPI_MAX_PORT_NAME];
    char name[DOMP_MAX_CLIENT_NAME];
    for (int i = 0; i < size; i++) {
      snprintf(name, DOMP_MAX_CLIENT_NAME, "%s-%d", clusterName, i);
      MPI_Lookup_name(name, MPI_INFO_NULL, port_name);
      MPI_Comm_connect( port_name, MPI_INFO_NULL, i, MPI_COMM_WORLD, &nodeConnections[i]);
    }
  }

  void MPIServer::stopServer() {
    MPI_Close_port(port_name);
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

  void MPIServer::handleMapResponse(MPI_Status status, MPI_Comm *client) {
    if (status.MPI_ERROR == MPI_SUCCESS && status.MPI_SOURCE == 0) {
      int count;
      MPI_Get_count(&status, MPI_BYTE, &count);
      auto *buffer = new char[count];
      MPI_Recv(buffer, count, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, *client, NULL);
      int requests = count / sizeof(DOMPMapCommand_t);
      int tag = DOMP_MIN_DATA_TAG;
      for(int i = 0; i < requests; i++) {
        auto *command = reinterpret_cast<DOMPMapCommand_t *>(buffer + i *sizeof(DOMPMapCommand_t));
        // Send the Data request to slave nodes. Use already created connection
        MPI_Isend(command, sizeof(DOMPMapCommand_t), MPI_BYTE, 0, tag, nodeConnections[command->nodeId], NULL);
        // Keep track of all requests
        dataRequests[tag++] = command;
      }
    }
  }

  void MPIServer::transferData(MPI_Status status, MPI_Comm *client) {
    if (status.MPI_ERROR == MPI_SUCCESS) {
      DOMPMapCommand_t command;
      MPI_Recv(&command, sizeof(DOMPMapCommand_t), MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, *client, NULL);
      std::pair<void*, int> ret = dompObject->mapDataRequest(command.varName, command.start, command.size);
      auto buffer = ret.first;
      // Someone requested data from me. Read and send the data to the specified node
      MPI_Send(buffer, ret.second, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, *client);
    }
  }


  void MPIServer::receiveData(MPI_Status status, MPI_Comm *client) {
    if (status.MPI_ERROR == MPI_SUCCESS) {
      bool isLastReceived = false;
      DOMPMapCommand_t *command = NULL;
      // Get the corresponding command
      {
        std::unique_lock<std::mutex> lck(dataMtx);
        if (dataRequests.count(status.MPI_TAG) != 0) {
          command = dataRequests[status.MPI_TAG];
          dataRequests.erase(status.MPI_TAG);
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
        MPI_Recv(ret.first, ret.second, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, *client, NULL);
        // Now let the main thread know that we receieved all data
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
    MPI_Send(buffer, size, MPI_BYTE, 0, MPI_MAP_REQ, nodeConnections[0]);
    // Clear the commands now
    mapRequest->commands.clear();
    // Wait for the data transfer to finish
    std::unique_lock<std::mutex> lck(dataMtx);
    // If I had nothing to receive
    if (mapRequest->commands.size() == 0) {
      dataReceived = true;
    }
    while (!dataReceived) dataCV.wait(lck);
    // Synchronization is must here
    MPI_Barrier(MPI_COMM_WORLD);
  }


  void MPIMasterServer::handleMapRequest(MPI_Status status, MPI_Comm *client) {
    if (status.MPI_ERROR == MPI_SUCCESS) {
      int count;
      MPI_Get_count(&status, MPI_BYTE, &count);
      char *buffer = new char[count];
      MPI_Recv(buffer, count, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, *client, NULL);
      commands_received.push_back(std::make_pair(status.MPI_SOURCE,(DOMPMapCommand_t*)buffer));

      mappingMtx.lock();
      if (mapReceived == (size - 1)) {
        // Respond to all slaves
        mappingMtx.unlock();


      } else {
        mapReceived ++;
        mappingMtx.unlock();
      }
    }
  }
}
