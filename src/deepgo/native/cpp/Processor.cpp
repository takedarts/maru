#include "Processor.h"

#include <algorithm>
#include <iostream>

namespace deepgo {

/**
 * Creates a computation management object.
 * @param model Model file
 * @param gpus List of GPU numbers
 * @param batchSize Maximum batch size
 * @param fp16 True to compute with 16-bit precision
 * @param deterministic True to make computation results reproducible
 * @param threadsParGpu Number of threads per GPU
 */
Processor::Processor(
    std::string model, std::vector<int32_t> gpus, int32_t batchSize,
    bool fp16, bool deterministic, int32_t threadsParGpu)
    : _mutex(),
      _executors() {
  for (auto gpu : gpus) {
    for (int32_t i = 0; i < threadsParGpu; i++) {
      _executors.push_back(
          std::make_unique<Executor>(model, gpu, batchSize, fp16, deterministic));
    }
  }
}

/**
 * Executes inference.
 * @param inputs Input data
 * @param outputs Output data
 * @param size Number of input/output data
 */
void Processor::execute(float* inputs, float* outputs, int32_t size) {
  // Select the computation executor to run next
  int32_t min_index = 0;

  {  // synchronize
    // Find the computation executor with the fewest waiting tasks
    std::unique_lock<std::mutex> lock(_mutex);
    int32_t min_count = _executors[0]->getWaitingCount();

    for (int32_t i = 1; i < _executors.size(); i++) {
      int32_t count = _executors[i]->getWaitingCount();

      if (count < min_count) {
        min_index = i;
        min_count = count;
      }
    }

    // Increase the reserved count
    _executors[min_index]->addReservedCount(size);
  }

  // Execute the computation
  _executors[min_index]->execute(inputs, outputs, size);
}

}  // namespace deepgo
