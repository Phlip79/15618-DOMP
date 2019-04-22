//
// Created by Apoorv Gupta on 4/21/19.
//

#ifndef DOMP_MPISERVER_H
#define DOMP_MPISERVER_H

#include <iostream>
#include <list>
#include <mutex>
#include <utility>
#include <mpi.h>

using namespace std;

namespace domp {
#define DOMP_MAX_VAR_NAME (50)

  class MPIServer;
  class MPIMasterServer;
  enum MPIServerTag {MPI_MAP_REQ= 0, MPI_MAP_RESP, MPI_DATA_CMD};
  enum MPIAccessType {MPI_SHARED_FETCH= 0, MPI_EXCLUSIVE_FETCH, MPI_SHARED_FIRST, MPI_EXCLUSIVE_FIRST};

  typedef struct DOMPMapCommand {
    char varName[DOMP_MAX_VAR_NAME];
    int start;
    int size;
    MPIAccessType accessType;
  } DOMPMapCommand_t;

  typedef struct DOMPMapRequest {
    int count;
    std::list <DOMPMapCommand_t> commands;
  } DOMPMapRequest_t;

}



class domp::MPIServer {
 protected:
  int size;
  int rank;
  char name[20];
  char port_name[MPI_MAX_PORT_NAME];
  void accept();
  void handleRequest(MPI_Comm *client);
  DOMPMapRequest_t *mapRequest;
 public:
  MPIServer(std::string name, int size, int rank) {
    int len = name.copy(this->name, 19, 0);
    this->name[len] = '\0';
    this->size = size;
    this->rank = rank;
    this->mapRequest = new DOMPMapRequest_t();
  }
  ~MPIServer();


  bool startServer();
  bool stopServer();

  void requestData(std::string varName, int start, int size, MPIAccessType accessType);

  void triggerMap();
  void handleMapRequest(MPI_Status status, MPI_Comm *client){}

};


 class domp::MPIMasterServer : protected domp::MPIServer{
  int mapReceived;
  std::mutex mtx;
  std::list<std::pair<int,DOMPMapCommand_t*>> commands_received;
  MPIMasterServer(std::string name, int size, int rank) :MPIServer(std::move(name), size, rank){
    mapReceived = 0;
  };
  void handleMapRequest(MPI_Status status, MPI_Comm *client);
};
#endif //DOMP_MPISERVER_H
