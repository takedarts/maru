#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "Executor.h"
#include "Model.h"

namespace deepgo {

class Processor {
 public:
  /**
   * Creates a computation management object.
   * @param model Model file
   * @param gpus List of GPU numbers
   * @param batchSize Maximum batch size
   * @param fp16 True to compute with 16-bit precision
   * @param deterministic True to make computation results reproducible
   * @param threadsParGpu Number of threads per GPU
   */
  Processor(
      std::string model, std::vector<int32_t> gpus, int32_t batchSize,
      bool fp16, bool deterministic, int32_t threadsParGpu);

  /**
   * Executes inference.
   * @param inputs Input data
   * @param outputs Output data
   * @param size Number of input/output data
   */
  void execute(float* inputs, float* outputs, int32_t size);

 private:
  /**
   * Mutex for synchronization.
   */
  std::mutex _mutex;

  /**
   * Computation executor objects.
   */
  std::vector<std::unique_ptr<Executor>> _executors;
};

}  // namespace deepgo
