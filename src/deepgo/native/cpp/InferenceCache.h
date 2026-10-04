#pragma once

#include <atomic>
#include <cstdint>
#include <list>
#include <map>
#include <mutex>

#include "InferenceHash.h"
#include "InferenceResult.h"

namespace deepgo {

/**
 * Class holding a cached inference result.
 */
class InferenceCacheEntry {
 public:
  /**
   * Create an inference cache entry.
   * @param result Inference result
   * @param orderIterator Iterator into the cache usage order list
   */
  InferenceCacheEntry(
      const InferenceResult& result,
      std::list<InferenceHash>::iterator orderIterator);

  /**
   * Get the inference result.
   * @return Inference result
   */
  inline const InferenceResult& getResult() const {
    return _result;
  }

  /**
   * Get the iterator into the cache usage order list.
   * @return Iterator into the cache usage order list
   */
  inline std::list<InferenceHash>::iterator getOrderIterator() const {
    return _orderIterator;
  }

  /**
   * Set the iterator into the cache usage order list.
   * @param orderIterator Iterator into the cache usage order list
   */
  inline void setOrderIterator(
      std::list<InferenceHash>::iterator orderIterator) {
    _orderIterator = orderIterator;
  }

 private:
  /** Inference result. */
  InferenceResult _result;

  /** Iterator into the cache usage order list. */
  std::list<InferenceHash>::iterator _orderIterator;
};

/**
 * Class caching inference results with LRU eviction.
 */
class InferenceCache {
 public:
  /**
   * Create an inference cache.
   * @param cacheSize Maximum number of cached inference results
   */
  explicit InferenceCache(int32_t cacheSize);

  /**
   * Destroy the inference cache.
   */
  virtual ~InferenceCache() = default;

  /**
   * Get the inference result for the specified hash.
   * Move a matching entry to the most recently used position.
   * @param inferenceHash Hash of the inference result to find
   * @param result Object receiving the cached inference result
   * @return True if an inference result was found
   */
  bool get(const InferenceHash& inferenceHash, InferenceResult& result);

  /**
   * Register an inference result for the specified hash.
   * Do nothing if a result is already registered for the same hash.
   * @param inferenceHash Hash under which to register the inference result
   * @param result Inference result to register
   */
  void put(const InferenceHash& inferenceHash, const InferenceResult& result);

  /**
   * Get the inference cache hit rate.
   * @return Inference cache hit rate
   */
  inline float getHitRate() const {
    return _hitRate.load(std::memory_order_relaxed);
  }

 private:
  /**
   * Update the cache hit rate.
   * The caller must hold _mutex when calling this function.
   * @param cacheHit True if a cached result was found
   */
  void updateHitRate(bool cacheHit);

  /** Mutex synchronizing the cache. */
  std::mutex _mutex;

  /** Maximum number of cached inference results. */
  int32_t _cacheSize;

  /**
   * Cache usage order.
   * The front is the most recently used entry and the back is the least recently used.
   */
  std::list<InferenceHash> _order;

  /** Inference results and their usage-order positions. */
  std::map<InferenceHash, InferenceCacheEntry> _entries;

  /** Inference cache hit rate. */
  std::atomic<float> _hitRate;
};

}  // namespace deepgo
