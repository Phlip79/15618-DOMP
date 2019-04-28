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
#include "util/DoublyLinkedList.h"

using namespace std;

namespace domp {

#define DOMP_MIN_DATA_TAG (100)

  class DataManager;
  class MasterDataManager;
  class Fragment;

  template class DoublyLinkedList<Fragment>;

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
  virtual void registerVariable(std::string varName, Variable *variable);

  virtual void triggerMap();
};

class domp::MasterDataManager : public domp::DataManager {
  std::list<std::pair<int,DOMPMapCommand_t>> commands_received;
  std::map<std::string, MasterVariable*> varList;

 public:
  MasterDataManager(DOMP *dompObject, int size, int rank) :DataManager(dompObject, size, rank){

  };

  ~MasterDataManager();

  void handleMapRequest(MPI_Status *status);
  void triggerMap();
  void registerVariable(std::string varName, Variable *variable);
};

class domp::Fragment {
  int start;
  int size;
  int end;
  std::set <int> nodes;
  friend SplitList;
  friend DoublyLinkedList<Fragment>;
  Fragment *next;
  Fragment *prev;
 public:
  Fragment(int start, int size, int nodeId) {
    this->start = start;
    this->size = size;
    this->end = start + size - 1; // Notice -1
    next = prev = NULL;
    this->nodes.insert(nodeId);
  }

  Fragment(Fragment *from) {
    this->start = from->start;
    this->size = from->size;
    this->end = start + size -  1; // Notice -1
    next = prev = NULL;
    this->nodes.insert(from->nodes.begin(), from->nodes.end());
  }

  void addNode(int nodeId) {
    this->nodes.insert(nodeId);
  }

  void update(int start, int end) {
    this->start = start;
    this->end = end;
    this->size = end - start + 1;
  }
};



class domp::MasterVariable {
  void *ptr;
  DoublyLinkedList<Fragment> fragments;
 public:
  MasterVariable(void * ptr, int size) {
    this->ptr = ptr;
    auto *fragment = new Fragment(0, size, -1);
    fragments.InsertFront(fragment);
  }
};

#endif //DOMP_MPISERVER_H
