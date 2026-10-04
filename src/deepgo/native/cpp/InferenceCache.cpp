#include "InferenceCache.h"

namespace deepgo {

/**
 * Create an inference cache entry.
 * @param result Inference result
 * @param orderIterator Iterator into the cache usage order list
 */
InferenceCacheEntry::InferenceCacheEntry(
    const InferenceResult& result,
    std::list<InferenceHash>::iterator orderIterator)
    : _result(result), _orderIterator(orderIterator) {}

/**
 * Create an inference cache.
 * @param cacheSize Maximum number of cached inference results
 */
InferenceCache::InferenceCache(int32_t cacheSize)
    : _mutex(),
      _cacheSize(cacheSize),
      _order(),
      _entries(),
      _hitRate(0.0f) {}

/**
 * Get the inference result for the specified hash.
 * Move a matching entry to the most recently used position.
 * @param inferenceHash Hash of the inference result to find
 * @param result Object receiving the cached inference result
 * @return True if an inference result was found
 */
bool InferenceCache::get(
    const InferenceHash& inferenceHash, InferenceResult& result) {
  // Synchronize cache lookup and usage-order updates
  std::lock_guard<std::mutex> lock(_mutex);

  // Look up the inference result for the specified hash
  auto it = _entries.find(inferenceHash);

  // Handle a missing result as a cache miss
  if (it == _entries.end()) {
    updateHitRate(false);
    return false;
  }

  // Copy the cached inference result to the caller
  result = it->second.getResult();

  // Move the accessed entry to the most recently used position
  _order.splice(_order.begin(), _order, it->second.getOrderIterator());
  it->second.setOrderIterator(_order.begin());

  // Update the hit rate for a cache hit
  updateHitRate(true);
  return true;
}

/**
 * Register an inference result for the specified hash.
 * Do nothing if a result is already registered for the same hash.
 * @param inferenceHash Hash under which to register the inference result
 * @param result Inference result to register
 */
void InferenceCache::put(
    const InferenceHash& inferenceHash, const InferenceResult& result) {
  // Disable caching when the cache size is zero or negative
  if (_cacheSize <= 0) {
    return;
  }

  // Synchronize cache insertion and eviction
  std::lock_guard<std::mutex> lock(_mutex);

  // Do nothing if another inference operation has already inserted the result
  if (_entries.find(inferenceHash) != _entries.end()) {
    return;
  }

  // Add the new cache key at the most recently used position
  _order.push_front(inferenceHash);

  // Register the inference result and its position in the usage order
  _entries.emplace(
      inferenceHash, InferenceCacheEntry(result, _order.begin()));

  // Evict the least recently used entry if the cache exceeds its capacity
  if (_entries.size() > static_cast<std::size_t>(_cacheSize)) {
    const InferenceHash& oldest_hash = _order.back();

    _entries.erase(oldest_hash);
    _order.pop_back();
  }
}

/**
 * Update the cache hit rate.
 * The caller must hold _mutex when calling this function.
 * @param cacheHit True if a cached result was found
 */
void InferenceCache::updateHitRate(bool cacheHit) {
  // Convert the lookup result to a numeric value
  float hit_rate_increment = cacheHit ? 1.0f : 0.0f;

  // Update the cache hit rate using an exponential moving average
  float old_hit_rate = _hitRate.load(std::memory_order_relaxed);
  float new_hit_rate = old_hit_rate * 0.99f + hit_rate_increment * 0.01f;

  _hitRate.store(new_hit_rate, std::memory_order_relaxed);
}

}  // namespace deepgo
