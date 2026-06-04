#pragma once

#include <ATen/ATen.h>
#include <torch/script.h>
#include <torch/torch.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace deepgo {

/**
 * A class representing an inference model.
 */
class InferenceModel {
 public:
  /**
   * Gets the available GPU numbers.
   * @return List of GPU numbers
   */
  static std::vector<std::int32_t> getAvailableGPUs();

  /**
   * Gets the execution device.
   * @param gpu GPU number
   * @return Execution device
   */
  static at::Device getDevice(int32_t gpu);

  /**
   * Gets the execution data type.
   * @param gpu GPU number
   * @param fp16 true to use half precision
   * @return Execution data type
   */
  static at::ScalarType getScalarType(int32_t gpu, bool fp16);

  /**
   * Creates an inference model.
   * @param filename Model file
   * @param gpu GPU number
   * @param fp16 true to use half precision
   * @param deterministic true to run deterministically
   */
  InferenceModel(std::string filename, int32_t gpu, bool fp16, bool deterministic);

  /**
   * Executes inference.
   * @param inputs Input data
   * @param outputs Output data
   * @param size Number of data samples to evaluate
   */
  void forward(int32_t* inputs, float* outputs, int32_t size);

  /**
   * Returns true if using CUDA.
   * @return true if using CUDA
   */
  inline bool isCuda() const {
    return _device.is_cuda();
  }

  /**
   * Returns true if using CPU.
   * @return true if using CPU
   */
  inline bool isCpu() const {
    return _cpu;
  }

 private:
  /**
   * Mutex for synchronizing input/output transfers.
   */
  std::mutex _ioMutex;

  /**
   * Mutex for synchronizing inference execution.
   */
  std::mutex _computeMutex;

  /**
   * Model.
   */
  torch::jit::script::Module _model;

  /**
   * Execution device.
   */
  at::Device _device;

  /**
   * Execution data type.
   */
  at::ScalarType _dtype;

  /**
   * Tensor for bit shifting.
   */
  torch::Tensor _bitShift;

  /**
   * true if executing on CPU.
   */
  bool _cpu;
};

}  // namespace deepgo
