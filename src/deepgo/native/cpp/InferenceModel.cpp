#include "InferenceModel.h"

#ifdef USE_TORCH_TENSORRT
#include <torch_tensorrt/logging.h>
#endif

#include <fstream>

#include "Config.h"

namespace deepgo {

/**
 * Determines whether the specified file is a TensorRT model file.
 * Because TensorRT model files contain specific marker strings,
 * this function checks whether any of those marker strings are present in the file.
 * Always returns false if TensorRT is not supported.
 * @param filename File name
 * @return True if the file is a TensorRT model file
 */
static bool isTensorRTModelFile(const std::string& filename) {
#ifdef USE_TORCH_TENSORRT
  const std::vector<std::string> trt_markers = {
      "__torch__.torch.classes.tensorrt.Engine",
      "torch.classes.tensorrt.Engine",
      "ops.tensorrt.execute_engine",
      "tensorrt::execute_engine",
      "torch_tensorrt",
      "_trt_engine",
  };

  // Opens the file in binary mode and reads the contents as a string
  std::ifstream ifs(filename, std::ios::binary);

  if (!ifs) {
    throw std::runtime_error("failed to open model file: " + filename);
  }

  std::string data(
      (std::istreambuf_iterator<char>(ifs)),
      std::istreambuf_iterator<char>());

  // Checks whether any of the marker strings are present in the file
  for (const auto& marker : trt_markers) {
    if (data.find(marker) != std::string::npos) {
      return true;
    }
  }
#endif

  return false;
}

/**
 * Returns the list of available GPU indices.
 * @return List of GPU indices
 */
std::vector<std::int32_t> InferenceModel::getAvailableGPUs() {
  // List of available GPU indices
  // By default, includes -1 which indicates CPU computation
  std::vector<std::int32_t> device_ids{-1};

  // Add all GPU indices if CUDA is available
  if (torch::cuda::is_available()) {
    int32_t gpu_count = torch::cuda::device_count();

    for (int32_t i = 0; i < gpu_count; ++i) {
      device_ids.push_back(i);
    }
  }
  // Add GPU index 0 if MPS is available
  else if (torch::mps::is_available()) {
    device_ids.push_back(0);
  }

  return device_ids;
}

/**
 * Gets the execution device.
 * @param gpu GPU number
 * @return Execution device
 */
at::Device InferenceModel::getDevice(int32_t gpu) {
  // Uses CPU if the GPU number is negative
  if (gpu < 0) {
    return at::Device(at::kCPU);
  }

  // Uses CUDA if CUDA is available
  if (torch::cuda::is_available() && gpu < torch::cuda::device_count()) {
    return at::Device(at::kCUDA, gpu);
  }

  // Uses MPS if MPS is available
  if (torch::mps::is_available() && gpu == 0) {
    return at::Device(at::kMPS);
  }

  // Throws an exception otherwise
  throw std::runtime_error("Specified GPU device is not available.");
}

/**
 * Gets the data type available for the execution environment.
 * @param gpu GPU number
 * @param fp16 True to compute with 16-bit precision
 * @return Data type
 */
at::ScalarType InferenceModel::getScalarType(int32_t gpu, bool fp16) {
  // Uses Half when computing with 16-bit precision on CUDA
  if (torch::cuda::is_available() && gpu >= 0 && fp16) {
    return at::kHalf;
  }
  // Uses Half when computing with 16-bit precision on MPS
  else if (torch::mps::is_available() && gpu == 0 && fp16) {
    return at::kHalf;
  }
  // Uses Float32 otherwise
  else {
    return at::kFloat;
  }
}

/**
 * Creates an inference model.
 * @param filename Model file
 * @param gpu GPU number
 * @param fp16 true to use half precision
 * @param deterministic true to run deterministically
 */
InferenceModel::InferenceModel(
    std::string filename, int32_t gpu, bool fp16, bool deterministic)
    : _ioMutex(),
      _computeMutex(),
      _device(InferenceModel::getDevice(gpu)),
      _dtype(InferenceModel::getScalarType(gpu, fp16)),
      _bitShift(),
      _cpu(gpu < 0) {
  // Configures CuDNN settings when using CUDA
  if (_device.is_cuda()) {
    torch::globalContext().setUserEnabledCuDNN(true);

    if (deterministic) {
      torch::globalContext().setBenchmarkCuDNN(false);
      torch::globalContext().setDeterministicCuDNN(true);
    } else {
      torch::globalContext().setBenchmarkCuDNN(true);
      torch::globalContext().setDeterministicCuDNN(false);
    }
  }

#ifdef USE_TORCH_TENSORRT
  // Sets the TensorRT log level to error
  torch_tensorrt::logging::set_reportable_log_level(
      torch_tensorrt::logging::Level::kERROR);
#endif

  // Loads the model and sets the execution device and data type
  // Because the loading method differs between TensorRT model files and other model files,
  // the file contents are checked to select the appropriate loading method
  // The `isTensorRTModelFile` function always returns false if TensorRT is not supported
  if (isTensorRTModelFile(filename)) {
    _model = torch::jit::load(filename, _device);
    _model.eval();
  } else {
    _model = torch::jit::load(filename);
    _model.to(_device, _dtype);
    _model.eval();
  }

  // Creates the bit-shift tensor and transfers it to the execution device
  _bitShift = torch::arange(0, 32, torch::TensorOptions().dtype(torch::kInt32));
  _bitShift = _bitShift.to(_device);
}

/**
 * Executes inference.
 * @param inputs Input data
 * @param outputs Output data
 * @param size Number of data samples to evaluate
 */
void InferenceModel::forward(int32_t* inputs, float* outputs, int32_t size) {
  c10::InferenceMode guard;
  torch::Tensor in_data;
  torch::Tensor out_data;

  // Converts input data to a tensor
  torch::Tensor in_values = torch::from_blob(
      inputs, size * MODEL_INPUT_PACK_SIZE,
      torch::TensorOptions().dtype(torch::kInt32));
  in_values = in_values.reshape({size, MODEL_INPUT_PACK_SIZE});

  // Transfers input data to the execution device
  {
    std::lock_guard<std::mutex> io_lock(_ioMutex);
    in_values = in_values.to(_device);
  }

  // Executes computation on the device
  {
    std::lock_guard<std::mutex> compute_lock(_computeMutex);

    // Each input value in the input data is stored as a bit representation (except the last 3 values)
    // Bit-shifts all values except the last 3 to convert them to 0 or 1
    in_data = torch::bitwise_right_shift(
        in_values.narrow(1, 0, MODEL_INPUT_PACK_SIZE - 1).unsqueeze(2), _bitShift);
    in_data = torch::bitwise_and(in_data, 1);
    in_data = in_data.reshape({size, -1});
    in_data = in_data.narrow(1, 0, MODEL_INPUT_SIZE);
    in_data = in_data.to(_dtype);

    // The 5th-to-last value is stored scaled from the range 0 to 1 into the range 0 to 0xfffff
    // Normalizes the 5th-to-last value to the range 0 to 1 and stores it as the 5th-to-last value of the input data
    // 5th-to-last value: a value representing the komi in board points
    in_values = in_values.narrow(1, MODEL_INPUT_PACK_SIZE - 1, 1);
    in_values = in_values.to(torch::kFloat32) / 0xfffff;
    in_values = in_values.to(_dtype);
    in_data.slice(1, MODEL_INPUT_SIZE - 5, MODEL_INPUT_SIZE - 4).copy_(in_values);

    // Executes the model and obtains the output data
    out_data = _model.forward({in_data}).toTensor();
  }

  // Transfers output data to CPU
  {
    std::lock_guard<std::mutex> io_lock(_ioMutex);
    out_data = out_data.to(torch::kCPU);
  }

  // Writes output data to the output array
  torch::Tensor out_values = torch::from_blob(
      outputs, size * MODEL_OUTPUT_SIZE,
      torch::TensorOptions().dtype(torch::kFloat32));
  out_values = out_values.reshape({size, MODEL_OUTPUT_SIZE});
  out_values.copy_(out_data.to(torch::kFloat32));
}

}  // namespace deepgo
