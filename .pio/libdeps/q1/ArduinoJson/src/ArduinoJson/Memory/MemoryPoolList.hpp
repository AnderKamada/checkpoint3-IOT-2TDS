// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Memory/MemoryPool.hpp>
#include <ArduinoJson/Polyfills/assert.hpp>
#include <ArduinoJson/Polyfills/utility.hpp>

#include <string.h>  // memcpy

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

using PoolCount = SlotId;

template <typename T>
class MemoryPoolList {
  struct FreeSlot {
    SlotId next;
  };

  static_assert(sizeof(FreeSlot) <= sizeof(T), "T is too small");

 public:
  using Pool = MemoryPool<T>;

  MemoryPoolList() = default;

  ~MemoryPoolList() {
    ARDUINOJSON_ASSERT(count_ == 0);
  }

  friend void swap(MemoryPoolList& a, MemoryPoolList& b) {
    bool aUsedPreallocated = a.pools_ == a.preallocatedPools_;
    bool bUsedPreallocated = b.pools_ == b.preallocatedPools_;

    // Who is using preallocated pools?
    if (aUsedPreallocated && bUsedPreallocated) {
      // both of us => swap preallocated pools
      for (PoolCount i = 0; i < ARDUINOJSON_INITIAL_POOL_COUNT; i++)
        swap_(a.preallocatedPools_[i], b.preallocatedPools_[i]);
    } else if (bUsedPreallocated) {
      // only b => copy b's preallocated pools and give him a's pointer
      for (PoolCount i = 0; i < b.count_; i++)
        a.preallocatedPools_[i] = b.preallocatedPools_[i];
      b.pools_ = a.pools_;
      a.pools_ = a.preallocatedPools_;
    } else if (aUsedPreallocated) {
      // only a => copy a's preallocated pools and give him b's pointer
      for (PoolCount i = 0; i < a.count_; i++)
        b.preallocatedPools_[i] = a.preallocatedPools_[i];
      a.pools_ = b.pools_;
      b.pools_ = b.preallocatedPools_;
    } else {
      // neither => swap pointers
      swap_(a.pools_, b.pools_);
    }

    swap_(a.count_, b.count_);
    swap_(a.capacity_, b.capacity_);
    swap_(a.freeList_, b.freeList_);
  }

  MemoryPoolList& operator=(MemoryPoolList&& src) {
    ARDUINOJSON_ASSERT(count_ == 0);
    if (src.pools_ == src.preallocatedPools_) {
      memcpy(preallocatedPools_, src.preallocatedPools_,
             sizeof(preallocatedPools_));
      pools_ = preallocatedPools_;
    } else {
      pools_ = src.pools_;
      src.pools_ = nullptr;
    }
    count_ = src.count_;
    capacity_ = src.capacity_;
    src.count_ = 0;
    src.capacity_ = 0;
    return *this;
  }

  Slot<T> allocSlot(Allocator* allocator) {
    // try to allocate from free list
    if (freeList_ != NULL_SLOT) {
      return allocFromFreeList();
    }

    // try to allocate from last pool (other pools are full)
    if (count_) {
      auto slot = allocFromLastPool();
      if (slot)
        return slot;
    }

    // create a new pool and try again
    auto pool = addPool(allocator);
    if (!pool)
      return {};

    return allocFromLastPool();
  }

  void freeSlot(Slot<T> slot) {
    reinterpret_cast<FreeSlot*>(slot.ptr())->next = freeList_;
    freeList_ = slot.id();
  }

  T* getSlot(SlotId id) const {
    if (id == NULL_SLOT)
      return nullptr;
    auto poolIndex = SlotId(id / ARDUINOJSON_POOL_CAPACITY);
    auto indexInPool = SlotId(id % ARDUINOJSON_POOL_CAPACITY);
    ARDUINOJSON_ASSERT(poolIndex < count_);
    return pools_[poolIndex].getSlot(indexInPool);
  }

  void clear(Allocator* allocator) {
    for (PoolCount i = 0; i < count_; i++)
      pools_[i].destroy(allocator);
    count_ = 0;
    freeList_ = NULL_SLOT;
    if (pools_ != preallocatedPools_) {
      allocator->deallocate(pools_);
      pools_ = preallocatedPools_;
      capacity_ = ARDUINOJSON_INITIAL_POOL_COUNT;
    }
  }

  SlotCount usage() const {
    SlotCount total = 0;
    for (PoolCount i = 0; i < count_; i++)
      total = SlotCount(total + pools_[i].usage());
    return total;
  }

  size_t size() const {
    return Pool::slotsToBytes(usage());
  }

  void shrinkToFit(Allocator* allocator) {
    if (count_ > 0)
      pools_[count_ - 1].shrinkToFit(allocator);
    if (pools_ != preallocatedPools_ && count_ != capacity_) {
      pools_ = static_cast<Pool*>(
          allocator->reallocate(pools_, count_ * sizeof(Pool)));
      ARDUINOJSON_ASSERT(pools_ != nullptr);  // realloc to smaller can't fail
      capacity_ = count_;
    }
  }

 private:
  Slot<T> allocFromFreeList() {
    ARDUINOJSON_ASSERT(freeList_ != NULL_SLOT);
    auto id = freeList_;
    auto slot = getSlot(freeList_);
    freeList_ = reinterpret_cast<FreeSlot*>(slot)->next;
    return {slot, id};
  }

  Slot<T> allocFromLastPool() {
    ARDUINOJSON_ASSERT(count_ > 0);
    auto poolIndex = SlotId(count_ - 1);
    auto slot = pools_[poolIndex].allocSlot();
    if (!slot)
      return {};
    return {slot.ptr(),
            SlotId(poolIndex * ARDUINOJSON_POOL_CAPACITY + slot.id())};
  }

  Pool* addPool(Allocator* allocator) {
    if (count_ == capacity_ && !increaseCapacity(allocator))
      return nullptr;
    auto pool = &pools_[count_++];
    SlotCount poolCapacity = ARDUINOJSON_POOL_CAPACITY;
    if (count_ == maxPools)  // last pool is smaller because of NULL_SLOT
      poolCapacity--;
    pool->create(poolCapacity, allocator);
    return pool;
  }

  bool increaseCapacity(Allocator* allocator) {
    if (capacity_ == maxPools)
      return false;
    void* newPools;
    auto newCapacity = PoolCount(capacity_ * 2);

    if (pools_ == preallocatedPools_) {
      newPools = allocator->allocate(newCapacity * sizeof(Pool));
      if (!newPools)
        return false;
      memcpy(newPools, preallocatedPools_, sizeof(preallocatedPools_));
    } else {
      newPools = allocator->reallocate(pools_, newCapacity * sizeof(Pool));
      if (!newPools)
        return false;
    }

    pools_ = static_cast<Pool*>(newPools);
    capacity_ = newCapacity;
    return true;
  }

  Pool preallocatedPools_[ARDUINOJSON_INITIAL_POOL_COUNT];
  Pool* pools_ = preallocatedPools_;
  PoolCount count_ = 0;
  PoolCount capacity_ = ARDUINOJSON_INITIAL_POOL_COUNT;
  SlotId freeList_ = NULL_SLOT;

 public:
  static const PoolCount maxPools =
      PoolCount(NULL_SLOT / ARDUINOJSON_POOL_CAPACITY + 1);
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
