#include "ServerState.h"
#include "Utils.h"

GlobalServerState_t::GlobalServerState_t() :
nServers_(0), nCurrentRequests_(0), initBiclusterSize_(0), nchunks_(0), nservices_(0), npendingBuffersUses_(0),
usePendingBuffers_(false)
{
  readyReq_ = MPI_REQUEST_NULL;
}

void GlobalServerState_t::initialize(int nservers, int initBiclusterSize, bool usePendingBuffers)
{
  nServers_ = nservers;
  initBiclusterSize_ = initBiclusterSize;
  usePendingBuffers_ = usePendingBuffers;
  if(nservers) {
    sentBuffers_.resize(nservers);
    std::fill(sentBuffers_.begin(), sentBuffers_.end(), nullptr);
  }
}

void GlobalServerState_t::push_back_local_buffer(uint32_t * buffer)
{
  localBuffers_.push_back(buffer);
}

void GlobalServerState_t::sendBuffer(uint32_t *cur_vector, int numBiclusters, int posServer)
{ MPI_Request send_req;

  MPI_Isend(cur_vector, numBiclusters * initBiclusterSize_, MPI_UINT32_T, posServer + 1, NewWorkTag, MPI_COMM_WORLD, &send_req);
  assert(sentBuffers_[posServer] == nullptr);
  sentBuffers_[posServer] = cur_vector;
  if (!nCurrentRequests_) {
    MPI_Irecv(&dummy_buf_, 0, MPI_INT, MPI_ANY_SOURCE, FinishTag, MPI_COMM_WORLD, &readyReq_);
  }
  nCurrentRequests_++;
  nservices_++;
  MPI_Request_free(&send_req);
}

void GlobalServerState_t::emptyPendingBuffers(int initialPos)
{

  for (int i = initialPos; !pendingBuffers_.empty() && (i < nServers_); ++i) {
    if(sentBuffers_[i] == nullptr) { // Means it is free
      const Buffer_t& tmp = pendingBuffers_.back();
      sendBuffer(tmp.buffer_, tmp.numBiclusters_, i);
      pendingBuffers_.pop_back();
      npendingBuffersUses_++;
    }
  }

}

void GlobalServerState_t::checkCompletions()
{ MPI_Status status;
  int flag;
  
  while (nCurrentRequests_) {
    MPI_Test(&readyReq_, &flag, &status);
    if (flag) {
      assert(sentBuffers_[status.MPI_SOURCE - 1] != nullptr);
      delete [] sentBuffers_[status.MPI_SOURCE - 1];
      sentBuffers_[status.MPI_SOURCE - 1] = nullptr;
      nCurrentRequests_--;
      if (nCurrentRequests_) {
        MPI_Irecv(&dummy_buf_, 0, MPI_INT, MPI_ANY_SOURCE, FinishTag, MPI_COMM_WORLD, &readyReq_);
      }
    } else {
      break;
    }
  }

}

bool GlobalServerState_t::findServer(uint32_t *cur_vector, int numBiclusters)
{ int i;

  mutex_.lock();
 
  nchunks_++;
 
  checkCompletions();

  for (i = 0; i < nServers_; i++) {
    if(sentBuffers_[i] == nullptr) { // Means it is free
      sendBuffer(cur_vector, numBiclusters, i);
      break;
    }
  }
  
  if(i == nServers_) {
    if (!usePendingBuffers_ || (pendingBuffers_.size() == nServers_)) {
      localBuffers_.push_back(cur_vector);
    } else {
      pendingBuffers_.push_back({cur_vector, numBiclusters});
      i = 0;
    }
  } else {
    if (!pendingBuffers_.empty()) { // can only be if usePendingBuffers_ is true
      emptyPendingBuffers(i + 1);
    }
  }
  
  mutex_.unlock();
  
  return (i < nServers_);
}

void GlobalServerState_t::finish()
{
  if(nServers_) {
    MPI_Request * const reqs = new MPI_Request[nServers_];
    uint32_t tmp, i;
    
    while (!pendingBuffers_.empty()) {
      checkCompletions();
      emptyPendingBuffers(0);
    }

    for (i = 0; i < nServers_; i++) {
      MPI_Isend(&tmp, 0, MPI_UINT32_T, i + 1, FinishTag, MPI_COMM_WORLD, reqs + i);
    }
    
    while(nCurrentRequests_) {
      checkCompletions();
    }
    
    MPI_Waitall(nServers_, reqs, MPI_STATUSES_IGNORE);
    
    delete [] reqs;

#ifdef BENCHMARKING
    Utils::log("Servers processed %d out of %d chunks (%.2f%%) (%d from pending buffers %.2f%%)\n", nservices_, nchunks_, nservices_ / (float)nchunks_ * 100.f, npendingBuffersUses_, npendingBuffersUses_ / (float)nchunks_ * 100.f);
#endif
  }
}

void GlobalServerState_t::clear()
{
  nchunks_ = 0;
  nservices_ = 0;
  for(auto ptr : localBuffers_) {
    delete [] ptr;
  }
  localBuffers_.clear();
}
