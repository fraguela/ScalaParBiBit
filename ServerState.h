#ifndef SERVERSTATE_H_
#define SERVERSTATE_H_

#include "Macros.h"

class GlobalServerState_t {
  
  struct Buffer_t {
    uint32_t *buffer_;
    int numBiclusters_;
  };

  vector<uint32_t *> sentBuffers_;
  vector<uint32_t *> localBuffers_;
  vector<Buffer_t> pendingBuffers_;
  mutex mutex_;
  MPI_Request readyReq_;
  int nServers_, nCurrentRequests_;
  int initBiclusterSize_;
  int nchunks_, nservices_, npendingBuffersUses_;
  int dummy_buf_;
  bool usePendingBuffers_;
  
  void sendBuffer(uint32_t *cur_vector, int numBiclusters, int posServer);
  void emptyPendingBuffers(int initialPos);
  void checkCompletions();

public:

  static constexpr int NewWorkTag = 0;
  static constexpr int FinishTag = 1;

  GlobalServerState_t();

  void initialize(int nservers, int initBiclusterSize, bool usePendingBuffers);

  void push_back_local_buffer(uint32_t * buffer);

  bool findServer(uint32_t *cur_vector, int numBiclusters);

  void finish();

  void clear();

 };

#endif
