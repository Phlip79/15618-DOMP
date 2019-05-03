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

#define DOMP_MIN_DATA_TAG (10)
#define DOMP_INVALID_NODE (-1)

  class DataManager;
  class MasterDataManager;
  class Fragment;
  class MasterVariable;


  enum MPIServerTag {MPI_MAP_REQ= 0, MPI_MAP_RESP, MPI_DATA_CMD, MPI_EXIT_CMD, MPI_EXIT_ACK};
  enum MPICommandType {MPI_DATA_FETCH = 0, MPI_DATA_SEND};
  enum MPIDataPhaseType {DATA_PHASE_READ, DATA_PHASE_UPDATE};

  typedef struct DOMPDataCommand {
    char varName[DOMP_MAX_VAR_NAME];
    int start;
    int size;
    int nodeId;
    int tagValue;
    MPICommandType commandType;
  } DOMPDataCommand_t;
}

class domp::DataManager {
 protected:
  int clusterSize;
  int rank;
  std::list<DOMPMapCommand_t*> mapRequest;
  DOMP *dompObject;
  MPI_Comm mpi_comm;

 public:
  DataManager(DOMP *dompObject, int clusterSize, int rank);
  virtual ~DataManager();
  void requestData(std::string varName, int start, int size, MPIAccessType accessType);
  void handleMapResponse(char* buffer, int count);
  virtual void registerVariable(std::string varName, Variable *variable);

  virtual void triggerMap();
};

class domp::MasterDataManager : public domp::DataManager {
  std::list<DOMPMapCommand_t*> commands_received;
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

  ~MasterVariable() {
    delete(dataList);
  }

  void applyCommand(CommandManager *commandManager, DOMPMapCommand_t *command, MPIDataPhaseType phase) {
    if (phase == DATA_PHASE_READ)
      dataList->ReadPhase(command, commandManager);
    else dataList->WritePhase(command);
  }
};

#endif //DOMP_MPISERVER_H
