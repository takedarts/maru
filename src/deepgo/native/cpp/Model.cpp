#include "Model.h"

#include "Config.h"

namespace deepgo {

/**
 * Get the device available in the execution environment.
 * @param gpu GPU number
 */
static at::Device getDevice(int32_t gpu) {
  // Use CPU if GPU number is negative
  if (gpu < 0) {
    return at::Device(at::kCPU);
  }

  // Use CUDA if available
  if (torch::cuda::is_available() && gpu < torch::cuda::device_count()) {
    return at::Device(at::kCUDA, gpu);
  }

  // Use MPS if available
  if (torch::mps::is_available() && gpu == 0) {
    return at::Device(at::kMPS);
  }

  // Otherwise, throw an exception
  throw std::runtime_error("Specified GPU device is not available.");
}

/**
 * Get the scalar type used in the execution environment.
 * @param gpu GPU number
 * @param fp16 True to compute with 16-bit precision
 * @return Scalar type
 */
static at::ScalarType getScalarType(int32_t gpu, bool fp16) {
  // Use Half if calculating with 16-bit precision using CUDA
  if (torch::cuda::is_available() && gpu >= 0 && fp16) {
    return at::kHalf;
  }
  // Use Half if calculating with 16-bit precision using MPS
  else if (torch::mps::is_available() && gpu == 0 && fp16) {
    return at::kHalf;
  }
  // Otherwise, use Float32
  else {
    return at::kFloat;
  }
}

/**
 * Constructor.
 * If GPU number is -1, creates an object that computes on CPU.
 * @param filename Model file
 * @param gpu GPU number
 * @param fp16 True to compute with 16-bit precision
 * @param deterministic True to make computation results reproducible
 */
Model::Model(std::string filename, int32_t gpu, bool fp16, bool deterministic)
    : _device(getDevice(gpu)),
      _dtype(getScalarType(gpu, fp16)) {
  if (torch::cuda::is_available()) {
    torch::globalContext().setUserEnabledCuDNN(true);

    if (deterministic) {
      torch::globalContext().setBenchmarkCuDNN(false);
      torch::globalContext().setDeterministicCuDNN(true);
    } else {
      torch::globalContext().setBenchmarkCuDNN(true);
      torch::globalContext().setDeterministicCuDNN(false);
    }
  }

  _model = torch::jit::load(filename);
  _model.to(_device, _dtype);
  _model.eval();

  _bitShift = torch::arange(0, 32, torch::TensorOptions().dtype(torch::kInt32));
  _bitShift = _bitShift.to(_device);
}

/**
 * Execute inference.
 * @param inputs Input data
 * @param outputs Output data
 * @param size Number of evaluation data
 */
void Model::forward(int32_t* inputs, float* outputs, uint32_t size) {
  torch::NoGradGuard no_grad;

  torch::Tensor in_values = torch::from_blob(
      inputs, size * MODEL_INPUT_PACK_SIZE,
      torch::TensorOptions().dtype(torch::kInt32));
  in_values = in_values.reshape({size, MODEL_INPUT_PACK_SIZE});
  in_values = in_values.to(_device);

  torch::Tensor in_data = torch::bitwise_right_shift(
      in_values.narrow(1, 0, MODEL_INPUT_PACK_SIZE - 1).unsqueeze(2), _bitShift);
  in_data = torch::bitwise_and(in_data, 1);
  in_data = in_data.reshape({size, -1});
  in_data = in_data.narrow(1, 0, MODEL_INPUT_SIZE);
  in_data = in_data.to(_dtype);

  in_values = in_values.narrow(1, MODEL_INPUT_PACK_SIZE - 1, 1);
  in_values = in_values.to(torch::kFloat32) / 0xfffff;
  in_values = in_values.to(_dtype);
  in_data.slice(1, MODEL_INPUT_SIZE - 5, MODEL_INPUT_SIZE - 4).copy_(in_values);

  torch::Tensor out_data = _model.forward({in_data}).toTensor();
  out_data = out_data.reshape({-1});
  out_data = out_data.to(torch::kCPU, torch::kFloat32).contiguous();

  memcpy(outputs, out_data.data_ptr(), sizeof(float) * out_data.numel());
}

/**
 * Returns True if using GPU.
 * @return 1 if using GPU, 0 otherwise
 */
int32_t Model::isCuda() {
  return _device.is_cuda();
}

}  // namespace deepgo
