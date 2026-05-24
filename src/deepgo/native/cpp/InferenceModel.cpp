#include "InferenceModel.h"

#ifdef USE_TORCH_TENSORRT
#include <torch_tensorrt/logging.h>
#endif

#include <fstream>

#include "Config.h"

namespace deepgo {

/**
 * 指定されたファイルがTensorRT形式のモデルファイルかどうかを判定する。
 * TensorRT形式のモデルファイルには特定のマーカー文字列が含まれているため、
 * それらのマーカー文字列がファイルに含まれているかどうかをチェックする。
 * TensorRTをサポートしていない場合は常にfalseを返す。
 * @param filename ファイル名
 * @return TensorRT形式のモデルファイルならTrue
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

  // ファイルをバイナリモードで開いて内容を文字列として読み込む
  std::ifstream ifs(filename, std::ios::binary);

  if (!ifs) {
    throw std::runtime_error("failed to open model file: " + filename);
  }

  std::string data(
      (std::istreambuf_iterator<char>(ifs)),
      std::istreambuf_iterator<char>());

  // マーカー文字列のいずれかがファイルに含まれているかどうかをチェックする
  for (const auto& marker : trt_markers) {
    if (data.find(marker) != std::string::npos) {
      return true;
    }
  }
#endif

  return false;
}

/**
 * 利用可能なGPU番号を取得する。
 * @return GPU番号の一覧
 */
std::vector<std::int32_t> InferenceModel::getAvailableGPUs() {
  if (torch::cuda::is_available()) {
    int32_t gpu_count = torch::cuda::device_count();
    std::vector<std::int32_t> gpus;

    for (int32_t i = 0; i < gpu_count; ++i) {
      gpus.push_back(i);
    }

    return gpus;
  }

  // MPSが使用できる場合はGPU番号0を返す
  if (torch::mps::is_available()) {
    return {0};
  }

  // それ以外の場合はGPUを使用しないことを示す-1を返す
  return {-1};
}

/**
 * 実行デバイスを取得する。
 * @param gpu GPU番号
 * @return 実行デバイス
 */
at::Device InferenceModel::getDevice(int32_t gpu) {
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

/**
 * 実行環境で使用できるデータ型を取得する。
 * @param gpu GPU番号
 * @param fp16 16bit精度で計算するならTrue
 * @return データ型
 */
at::ScalarType InferenceModel::getScalarType(int32_t gpu, bool fp16) {
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
 * 推論モデルを作成する。
 * @param filename モデルファイル
 * @param gpu GPU番号
 * @param fp16 半精度を使用するならtrue
 * @param deterministic 決定論的に実行するならtrue
 */
InferenceModel::InferenceModel(
    std::string filename, int32_t gpu, bool fp16, bool deterministic)
    : _ioMutex(),
      _computeMutex(),
      _device(InferenceModel::getDevice(gpu)),
      _dtype(InferenceModel::getScalarType(gpu, fp16)),
      _bitShift(),
      _cpu(gpu < 0) {
  // CUDAを使用する場合はCuDNNの設定を行う
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
  // TensorRTのログレベルをエラーに設定する
  torch_tensorrt::logging::set_reportable_log_level(
      torch_tensorrt::logging::Level::kERROR);
#endif

  // モデルをロードして実行デバイスとデータ型を設定する
  // TensorRT形式のモデルファイルとそれ以外のモデルファイルでロード方法が異なるため、
  // ファイルの内容をチェックしてロード方法を切り替える
  // TensorRTをサポートしていない場合は`isTensorRTModelFile`関数は常にfalseを返す
  if (isTensorRTModelFile(filename)) {
    _model = torch::jit::load(filename, _device);
    _model.eval();
  } else {
    _model = torch::jit::load(filename);
    _model.to(_device, _dtype);
    _model.eval();
  }

  // ビットシフト用テンソルを作成して実行デバイスに転送する
  _bitShift = torch::arange(0, 32, torch::TensorOptions().dtype(torch::kInt32));
  _bitShift = _bitShift.to(_device);
}

/**
 * 推論を実行する。
 * @param inputs 入力データ
 * @param outputs 出力データ
 * @param size 評価データの数
 */
void InferenceModel::forward(int32_t* inputs, float* outputs, int32_t size) {
  c10::InferenceMode guard;
  torch::Tensor in_data;
  torch::Tensor out_data;

  // 入力データをテンソルに変換する
  torch::Tensor in_values = torch::from_blob(
      inputs, size * MODEL_INPUT_PACK_SIZE,
      torch::TensorOptions().dtype(torch::kInt32));
  in_values = in_values.reshape({size, MODEL_INPUT_PACK_SIZE});

  // 入力データを実行デバイスに転送する
  {
    std::lock_guard<std::mutex> io_lock(_ioMutex);
    in_values = in_values.to(_device);
  }

  // デバイスでの計算を実行する
  {
    std::lock_guard<std::mutex> compute_lock(_computeMutex);

    // 入力データはそれぞれの入力値がビット表現で格納されている（最後の3つの値を除く）
    // 最後の3つの値以外はビットシフトして0か1の値に変換する
    in_data = torch::bitwise_right_shift(
        in_values.narrow(1, 0, MODEL_INPUT_PACK_SIZE - 1).unsqueeze(2), _bitShift);
    in_data = torch::bitwise_and(in_data, 1);
    in_data = in_data.reshape({size, -1});
    in_data = in_data.narrow(1, 0, MODEL_INPUT_SIZE);
    in_data = in_data.to(_dtype);

    // 最後から5つ目の値は0から1の範囲を0から0xfffffの範囲にスケーリングして格納されている
    // 最後から5つ目の値を0から1の範囲に正規化して入力データの最後の5つ目の値として格納する
    // 最後の5つ目の値：盤面のコミの目数を表す値
    in_values = in_values.narrow(1, MODEL_INPUT_PACK_SIZE - 1, 1);
    in_values = in_values.to(torch::kFloat32) / 0xfffff;
    in_values = in_values.to(_dtype);
    in_data.slice(1, MODEL_INPUT_SIZE - 5, MODEL_INPUT_SIZE - 4).copy_(in_values);

    // モデルを実行して出力データを取得する
    out_data = _model.forward({in_data}).toTensor();
  }

  // 出力データをCPUに転送する
  {
    std::lock_guard<std::mutex> io_lock(_ioMutex);
    out_data = out_data.to(torch::kCPU);
  }

  // 出力データを出力配列に書き込む
  torch::Tensor out_values = torch::from_blob(
      outputs, size * MODEL_OUTPUT_SIZE,
      torch::TensorOptions().dtype(torch::kFloat32));
  out_values = out_values.reshape({size, MODEL_OUTPUT_SIZE});
  out_values.copy_(out_data.to(torch::kFloat32));
}

}  // namespace deepgo
