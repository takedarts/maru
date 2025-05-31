#include "Model.h"

#include <iostream>

#include "Config.h"

namespace deepgo {

/**
 * 実行環境で使用できるデバイスを取得する。
 * @param gpu GPU番号
 */
static at::Device getDevice(int32_t gpu) {
  // GPU番号が負の値ならCPUを使う
  if (gpu < 0) {
    return at::Device(at::kCPU);
  }

  // CUDAが使用できる場合はCUDAを使う
  if (torch::cuda::is_available() && gpu < torch::cuda::device_count()) {
    return at::Device(at::kCUDA, gpu);
  }

  // MPSが使用できる場合はMPSを使う
  if (torch::mps::is_available() && gpu == 0) {
    return at::Device(at::kMPS);
  }

  // それ以外の場合は例外を投げる
  throw std::runtime_error("Specified GPU device is not available.");
}

static at::ScalarType getScalarType(int32_t gpu, bool fp16) {
  // CUDAを使用して16bit精度で計算する場合はHalfを使う
  if (torch::cuda::is_available() && gpu >= 0 && fp16) {
    return at::kHalf;
  }
  // MPSを使用して16bit精度で計算する場合はHalfを使う
  else if (torch::mps::is_available() && gpu == 0 && fp16) {
    return at::kHalf;
  }
  // それ以外はFloat32を使う
  else {
    return at::kFloat;
  }
}

/**
 * コンストラクタ。
 * GPU番号に-1を指定するとCPUで計算するオブジェクトを生成する。
 * @param filename モデルファル
 * @param gpu GPU番号
 * @param fp16 16bit精度で計算するならTrue
 * @param deterministic 計算結果を再現可能にするならTrue
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
}

/**
 * 推論を実行する。
 * @param inputs 入力データ
 * @param outputs 出力データ
 * @param size 評価データの数
 */
void Model::forward(float* inputs, float* outputs, uint32_t size) {
  torch::NoGradGuard no_grad;
  torch::Tensor in_data = torch::from_blob(
      inputs, size * MODEL_INPUT_SIZE,
      torch::TensorOptions().dtype(torch::kFloat32));
  in_data = in_data.reshape({size, MODEL_INPUT_SIZE});
  in_data = in_data.to(_device, _dtype);

  torch::Tensor out_data = _model.forward({in_data}).toTensor();
  out_data = out_data.reshape({-1});
  out_data = out_data.to(torch::kCPU, torch::kFloat32).contiguous();
  memcpy(outputs, out_data.data_ptr(), sizeof(float) * out_data.numel());
}

/**
 * GPUを使うならTrueを返す。
 * @return GPUを使うなら1, 使わないなら0
 */
int32_t Model::isCuda() {
  return _device.is_cuda();
}

}  // namespace deepgo
