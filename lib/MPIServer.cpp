//
// Created by Apoorv Gupta on 4/21/19.

// Majority of the code hasn been taken from https://www.mpi-forum.org/docs/mpi-3.1/mpi31-report/node248.htm
//

#include "MPIServer.h"
#include <mpi.h>

namespace domp {

  MPIServer::~MPIServer() {
  }

  void MPIServer::accept() {
    MPI_Open_port(MPI_INFO_NULL, port_name);
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
        case MPI_MAP_RESP:break;
        case MPI_DATA_CMD:break;
        default:
          // I am receiving data
          break;
      }
    }
    MPI_Comm_disconnect(client);
    delete (client);
  }

  bool MPIServer::startServer() {
    return true;
  }

  bool MPIServer::stopServer() {
    MPI_Close_port(port_name);
    return true;
  }

  void MPIServer::requestData(std::string varName, int start, int size, MPIAccessType accessType) {
    DOMPMapCommand_t command;
    strncpy(command.varName, varName.c_str(), 50);
    command.accessType = accessType;
    command.size = size;
    command.start = start;
    mapRequest->commands.push_back(command);
  }

  void MPIServer::triggerMap() {
    ssize_t  size = mapRequest->commands.size() * sizeof(DOMPMapCommand_t);
    char buffer[size];

    std::list<DOMPMapCommand_t>::iterator it;
    int i = 0;
    for (it = mapRequest->commands.begin(); it != mapRequest->commands.end(); ++it){
      memcpy(&buffer[i * sizeof(DOMPMapCommand_t)], &(*it)  , sizeof(DOMPMapCommand_t));
    }

    // Send the MAP request to Master node
    MPI_Send(buffer, size, MPI_BYTE, 0, MPI_MAP_REQ, MPI_COMM_WORLD);
  }


  void MPIMasterServer::handleMapRequest(MPI_Status status, MPI_Comm *client) {
    if (status.MPI_ERROR == MPI_SUCCESS) {
      int count;
      MPI_Get_count(&status, MPI_BYTE, &count);
      char *buffer = new char[count];
      MPI_Recv(buffer, count, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, *client, NULL);
      commands_received.push_back(std::make_pair(status.MPI_SOURCE,(DOMPMapCommand_t*)buffer));

      mtx.lock();
      if (mapReceived == (size - 1)) {
        // Respond to all slaves
        mtx.unlock();


      } else {
        mapReceived ++;
        mtx.unlock();
      }
    }
  }
}
