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
#include <map>
#include <thread>
#include <condition_variable>
#include "domp.h"

using namespace std;

namespace domp {
#define DOMP_MIN_DATA_TAG (100)

  class DataManager;
  class MasterDataManager;
  enum MPIServerTag {MPI_MAP_REQ= 0, MPI_MAP_RESP, MPI_DATA_CMD, MPI_EXIT_CMD, MPI_EXIT_ACK};
  enum MPICommandType {MPI_DATA_FETCH = 0, MPI_DATA_SEND};
  enum MPIAccessType {MPI_SHARED_FETCH= 0, MPI_EXCLUSIVE_FETCH, MPI_SHARED_FIRST, MPI_EXCLUSIVE_FIRST};

#define IS_EXCLUSIVE(e) ((e == MPI_EXCLUSIVE_FETCH) ||(e == MPI_EXCLUSIVE_FIRST))
#define IS_FETCH(e) ((e == MPI_SHARED_FETCH) || (e == MPI_EXCLUSIVE_FETCH))


  typedef struct DOMPMapCommand {
    std::string varName;
    int start;
    int size;
    MPIAccessType accessType;
    int nodeId;
  } DOMPMapCommand_t;


  typedef struct DOMPDataCommand {
    std::string varName;
    int start;
    int size;
    int nodeId;
    int tagValue;
    MPICommandType commandType;
  } DOMPDataCommand_t;

  typedef struct DOMPMapRequest {
    int count;
    std::list <DOMPMapCommand_t> commands;
  } DOMPMapRequest_t;

}



class domp::DataManager {
 protected:
  int clusterSize;
  int rank;
  DOMPMapRequest_t *mapRequest;
  DOMP *dompObject;
  MPI_Comm mpi_comm;

 public:
  DataManager(DOMP *dompObject, int clusterSize, int rank);
  virtual ~DataManager();
  void requestData(std::string varName, int start, int size, MPIAccessType accessType);
  void handleMapResponse(MPI_Status *status);

  virtual void triggerMap();
};


class domp::MasterDataManager : public domp::DataManager {
  std::list<std::pair<int,DOMPMapCommand_t>> commands_received;
 public:
  MasterDataManager(DOMP *dompObject, int size, int rank) :DataManager(dompObject, size, rank){

  };

  void handleMapRequest(MPI_Status *status);
  void triggerMap();
};
#endif //DOMP_MPISERVER_H
