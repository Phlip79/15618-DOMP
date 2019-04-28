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
#include "CommandManager.h"
#include "util/SplitList.h"

using namespace std;

namespace domp {

#define DOMP_MIN_DATA_TAG (100)
#define DOMP_INVALID_NODE (-1)

  class DataManager;
  class MasterDataManager;
  class Fragment;
  class MasterVariable;


  enum MPIServerTag {MPI_MAP_REQ= 0, MPI_MAP_RESP, MPI_DATA_CMD, MPI_EXIT_CMD, MPI_EXIT_ACK};
  enum MPICommandType {MPI_DATA_FETCH = 0, MPI_DATA_SEND};

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
  virtual void registerVariable(std::string varName, Variable *variable);

  virtual void triggerMap();
};

class domp::MasterDataManager : public domp::DataManager {
  std::list<DOMPMapCommand_t> commands_received;
  std::map<std::string, MasterVariable*> varList;
  CommandManager *commandManager;

 public:
  MasterDataManager(DOMP *dompObject, int clusterSize, int rank) :DataManager(dompObject, clusterSize, rank){
    commandManager =  new CommandManager(clusterSize);
  };

  ~MasterDataManager();

  void handleMapRequest(MPI_Status *status);
  void triggerMap();
  void registerVariable(std::string varName, Variable *variable);
};



class domp::MasterVariable {
  void *ptr;
  SplitList *dataList;
 public:
  MasterVariable(void * ptr, int size) {
    this->ptr = ptr;
    dataList = new SplitList(0, size, DOMP_INVALID_NODE);
  }

  void applyCommand(CommandManager *commandManager, DOMPMapCommand_t *command) {
    dataList->ReadPhase(command, commandManager);
    dataList->WritePhase(command);
  }
};

#endif //DOMP_MPISERVER_H
