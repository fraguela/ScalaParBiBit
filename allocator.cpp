#if defined(linux) || defined(__linux) || defined(__linux__)
#include <linux/mman.h>
#endif

#include <sys/mman.h>
#include <iostream>
#include "allocator.h"

thread_local MemoryChunk* MemoryChunkManager::MemoryChunk_;

void *allocator_mmap(size_t bytesNeeded)
{
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  
#if defined(MAP_NORESERVE) && defined(MAP_HUGE_SHIFT)
  flags |= MAP_NORESERVE | (18 << MAP_HUGE_SHIFT);
#endif
  
  return mmap(nullptr, bytesNeeded, PROT_READ|PROT_WRITE, flags, -1, 0);
}

MemoryChunk::MemoryChunk(MemoryChunk* next, size_t bytesNeeded) :
next(next) 
{

  entries = (char*)allocator_mmap(bytesNeeded);
  
  if((intptr_t)entries & 0xFFFF000000000000ULL) {
    std::cerr << "Warning: allocated memory memoryChunk using upper 16 bits" << std::endl;
  }
  assert(entries);
  nextentry = entries;
  end = entries + bytesNeeded;
}

MemoryChunk::~MemoryChunk() 
{
  munmap(entries, (end-entries));
}

MemoryChunkManager::MemoryChunkManager()
: allMemoryChunks_(nullptr)
{ }

void MemoryChunkManager::thread_init()
{
  linkNewMemoryChunk();
}

MemoryChunk* MemoryChunkManager::linkNewMemoryChunk()
{
  auto myMemoryChunk = new MemoryChunk(allMemoryChunks_.load(std::memory_order_relaxed), MemoryChunkSize);
  MemoryChunk_ = myMemoryChunk;
  while(!allMemoryChunks_.compare_exchange_weak(myMemoryChunk->next, myMemoryChunk, std::memory_order_release, std::memory_order_relaxed)) {
  }
  return myMemoryChunk;
}

MemoryChunkManager::~MemoryChunkManager()
{
  auto current = allMemoryChunks_.load(std::memory_order_relaxed);
  while(current) {
    const auto next = current->next;
    delete current;
    current = next;
  }
}
