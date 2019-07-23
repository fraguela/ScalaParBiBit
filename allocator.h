#ifndef BBUTILS_ALLOCATOR_H
#define BBUTILS_ALLOCATOR_H

#include <cassert>
#include <cstring>
#include <atomic>
#include <type_traits>

void *allocator_mmap(size_t bytesNeeded);

struct MemoryChunk {

  char* entries;
  char* nextentry;
  char* end;
  MemoryChunk* next;
  
  MemoryChunk(MemoryChunk* next, size_t bytesNeeded);
  
  ~MemoryChunk();
  
  template<typename T>
  __attribute__((always_inline))
  T* alloc() {
    return (T*)alloc<std::alignment_of<T>::value>(sizeof(T));
  }
  
  __attribute__((always_inline))
  char* alloc(size_t size) {
    auto mem = nextentry;
    nextentry += size;
    return mem;
  }
  
  template<size_t alignPowerTwo>
  __attribute__((always_inline))
  char* alloc(size_t size) {
    static_assert(!(alignPowerTwo & (alignPowerTwo - 1)), "Requires power of 2");
    auto mem = nextentry;
    mem += alignPowerTwo - 1;
    mem = (char*)(((uintptr_t)mem) & ~(alignPowerTwo - 1));
    nextentry = mem + size;
    return mem;
  }
  
  template<typename T>
  __attribute__((always_inline))
  void free(T* mem) {
    return free((void*)mem, sizeof(T));
  }
  
  __attribute__((always_inline))
  void free(void* mem, size_t length) {
    (void)mem;
    nextentry -= length;
    assert((char*)mem == nextentry);
  }
};

class MemoryChunkManager {

  static thread_local MemoryChunk* MemoryChunk_;
  std::atomic<MemoryChunk*> allMemoryChunks_;
  
public:
  
  static constexpr size_t MemoryChunkSize = 1ULL << 22; // 4MB

  MemoryChunkManager();
  
  MemoryChunk* linkNewMemoryChunk();
  
  void thread_init();

  template<typename T>
  __attribute__((always_inline))
  T* alloc() {
    MemoryChunk* myMemoryChunk = MemoryChunk_;
    if(myMemoryChunk->nextentry + sizeof(T) > myMemoryChunk->end) {
      myMemoryChunk = linkNewMemoryChunk();
    }
    auto r = myMemoryChunk->alloc<T>();
    assert(r);
    return r;
  }
  
  template<typename T>
  __attribute__((always_inline))
  void free(T* mem) {
    return MemoryChunk_->free(mem);
  }
  
  template<int alignPowerTwo>
  __attribute__((always_inline))
  char* alloc(size_t size) {
    MemoryChunk* myMemoryChunk = MemoryChunk_;
    if(myMemoryChunk->nextentry + size > myMemoryChunk->end) {
      myMemoryChunk = linkNewMemoryChunk();
    }
    auto r = myMemoryChunk->alloc<alignPowerTwo>(size);
    assert(r);
    return r;
  }
  
  __attribute__((always_inline))
  char* realloc(void* mem, size_t size) {
    char *ret;
    MemoryChunk* const myMemoryChunk = MemoryChunk_;
    if(((char*)mem + size) > myMemoryChunk->end) {
      MemoryChunk* const myNewMemoryChunk = linkNewMemoryChunk();
      ret = myNewMemoryChunk->alloc(size);
      memcpy(ret, mem, myMemoryChunk->nextentry - (char*)mem);
      return ret;
    } else {
      myMemoryChunk->nextentry = ((char*)mem + size);
      return (char*)mem;
    }
  }
  
  __attribute__((always_inline))
  void free(void* mem, size_t length) {
    return MemoryChunk_->free(mem, length);
  }
  
  ~MemoryChunkManager();

};

#endif
