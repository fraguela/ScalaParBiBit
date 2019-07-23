#ifndef BBUTILS_UNORDEREDVARSET_H
#define BBUTILS_UNORDEREDVARSET_H

#include <sys/mman.h>
#include "allocator.h"

template<typename ACCESSOR_T, bool HASH_OPT = true>
class UnorderedVarSet {
  
  struct Entry {
    std::atomic<Entry*> next_;
    char data_[0];

    void setNext(Entry* next) {
      next_.store(next, std::memory_order_relaxed);
    }

    Entry* getNext() const noexcept {
      return next_.load(std::memory_order_relaxed);
    }
    
  };
  
  size_t nbuckets_;
  MemoryChunkManager mmg_;
  std::atomic<Entry*>* mapping_;
  
  Entry *createEntry(const ACCESSOR_T& acc)
  {
    Entry *ret = reinterpret_cast<Entry *>(mmg_.alloc<4>(sizeof(Entry) + acc.size()));
    if (HASH_OPT) {
      assert( (((intptr_t)ret) & 0xFFFF000000000003ULL) == 0 ); //No top 16 bits and lower 2 bits
    } else {
      assert( (((intptr_t)ret) & 0x3) == 0 ); //No lower 2 bits
    }
    ret->setNext(nullptr);
    acc.fill(ret->data_);
    return ret;
  }
  
  // In pluton under g++ 3.6.1 it has been sometimes observed that "current" only gets successfully the top 32 bits!!
  //The bug only appeared when HASH_OPT is true.
  //The bug appeared quite often in the Sandy Bridge, but also sometimes in the Broadwell ones.
  //Attempts to solve it by changing to std::memory_order_seq_cst the atomic operations did not succeed
  //False fixes are also possible, i.e., sometimes the lower 32 bits can be actually 0
  __attribute__((always_inline)) void fix_bug(std::atomic<Entry*> * const parentLink, Entry *& current) const noexcept
  {
    if ( (((intptr_t)(current) & 0xFFFFFFFFULL) == 0) && current ) {
      current = parentLink->load(std::memory_order_relaxed);
//#ifn NDEBUG
//      if ( ((intptr_t)(current) & 0xFFFFFFFFULL) != 0 ) {
//        std::cerr << "Fixed " << current << '\n';
//      } else std::cerr << "False Fixed\n";
//#endif
    }
  }
  
public:

  UnorderedVarSet(size_t nbuckets = 256)
  {
    if (!nbuckets) {
      nbuckets = 1;
    }
    
    for(nbuckets_ = 1; nbuckets_ < nbuckets; nbuckets_ = nbuckets_ << 1);
    
    size_t sz = nbuckets_ * sizeof(std::atomic<Entry*>);
    mapping_ = static_cast<std::atomic<Entry*>*>(allocator_mmap(sz));
    posix_madvise(mapping_, sz, POSIX_MADV_RANDOM);
    for (size_t i = 0; i < nbuckets_; i++) {
      mapping_[i].store(nullptr, std::memory_order_relaxed);
    }
  }
  
  void thread_init()
  {
    mmg_.thread_init();
  }

  bool insert(const ACCESSOR_T& acc, const bool only_check = false)
  {
    const size_t h = acc.hash();
    const size_t nentry = h & (nbuckets_ - 1);
    const size_t h16l = h & 0xFFFF000000000000ULL;
    
    Entry *current = mapping_[nentry].load(std::memory_order_relaxed);
    std::atomic<Entry*> *parentLink = &mapping_[nentry];
    
    while(current) {

      if (HASH_OPT) {
        const size_t currentHash = ((intptr_t)current & 0xFFFF000000000000ULL);
        current = (Entry*)((intptr_t)current & 0x0000FFFFFFFFFFFFULL);
        if( (currentHash == h16l) && acc.matches(current->data_) ) {
          return false;
        }
      } else {
        if(acc.matches(current->data_) ) {
          return false;
        }
      }
      
      parentLink = &(current->next_);
      current = current->getNext();
      fix_bug(parentLink, current);
    }
    
    if (!only_check) {
      Entry * const new_entry = createEntry(acc);
      Entry * const new_entryWithHash = HASH_OPT ? (Entry *)(((intptr_t)new_entry)|h16l) : new_entry;
      while(!parentLink->compare_exchange_weak(current, new_entryWithHash, std::memory_order_release, std::memory_order_relaxed)) {
        while(current) {
          
          if (HASH_OPT) {
            const size_t currentHash = ((intptr_t)current & 0xFFFF000000000000ULL);
            current = (Entry*)((intptr_t)current & 0x0000FFFFFFFFFFFFULL);
            if( (currentHash == h16l) && acc.matches(current->data_) ) {
              mmg_.free(new_entry, sizeof(Entry) + acc.size());
              return false;
            }
          } else {
            if( acc.matches(current->data_) ) {
              mmg_.free(new_entry, sizeof(Entry) + acc.size());
              return false;
            }
          }

          parentLink = &current->next_;
          current = current->getNext();
          fix_bug(parentLink, current);
        }
      }
    }

    return true;
  }
  
  bool contains(const ACCESSOR_T& acc)
  {
    return !insert(acc, true);
  }

  size_t nBuckets() const noexcept { return nbuckets_; }
  
  struct Stats {
    size_t size;
    size_t usedBuckets;
    size_t collisions;
    size_t longestChain;
    double avgChainLength;
  };
  
  void getStats(Stats& s) const {
    s.size = 0;
    s.usedBuckets = 0;
    s.collisions = 0;
    s.longestChain = 0;
    s.avgChainLength = 0.0;
    
    for(size_t idx = 0; idx < nbuckets_; ++idx) {
      Entry* bucket = mapping_[idx].load(std::memory_order_relaxed);
      if(bucket) {
        Entry* entry = bucket;
        size_t chainSize = 0;
        while(entry) {
          chainSize++;
          if (HASH_OPT) {
            entry = (Entry*)((intptr_t)entry & 0x0000FFFFFFFFFFFFULL);
          }
          entry = entry->getNext();
        }
        s.usedBuckets++;
        s.size += chainSize;
        s.collisions += chainSize - 1;
        if(chainSize > s.longestChain) s.longestChain = chainSize;
      }
    }
    if(nbuckets_ > 0) {
      s.avgChainLength = (double)s.size / (double)nbuckets_;
    }
  }

  size_t size() const {
    Stats s;
    getStats(s);
    return s.size;
  }
  
  template<typename CONTAINER>
  void getDensityStats(size_t bars, CONTAINER& elements) const {
    
    size_t bucketPerBar = nbuckets_ / bars;
    bucketPerBar += bucketPerBar == 0;
    
    for(size_t idx = 0; idx < nbuckets_; ) {
      size_t elementsInThisBar = 0;
      const size_t max = std::min(nbuckets_, idx + bucketPerBar);
      for(; idx < max; ++idx) {
        
        Entry* bucket = mapping_[idx].load(std::memory_order_relaxed);
        if(bucket) {
          Entry* entry = bucket;
          size_t chainSize = 0;
          while(entry) {
            chainSize++;
            if (HASH_OPT) {
              entry = (Entry*)((intptr_t)entry & 0x0000FFFFFFFFFFFFULL);
            }
            entry = entry->getNext();
          }
          elementsInThisBar += chainSize;
        }
      }
      elements.push_back(elementsInThisBar);
    }
    
  }

  ~UnorderedVarSet() {
    munmap(mapping_, nbuckets_ * sizeof(std::atomic<Entry*>));
  }
  
};

#endif
