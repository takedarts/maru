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
 * 推論モデルを表すクラス。
 */
class InferenceModel {
 public:
  /**
   * 利用可能なGPU番号を取得する。
   * @return GPU番号の一覧
   */
  static std::vector<std::int32_t> getAvailableGPUs();

  /**
   * 実行デバイスを取得する。
   * @param gpu GPU番号
   * @return 実行デバイス
   */
  static at::Device getDevice(int32_t gpu);

  /**
   * 実行データ型を取得する。
   * @param gpu GPU番号
   * @param fp16 半精度を使用するならtrue
   * @return 実行データ型
   */
  static at::ScalarType getScalarType(int32_t gpu, bool fp16);

  /**
   * 推論モデルを作成する。
   * @param filename モデルファイル
   * @param gpu GPU番号
   * @param fp16 半精度を使用するならtrue
   * @param deterministic 決定論的に実行するならtrue
   */
  InferenceModel(std::string filename, int32_t gpu, bool fp16, bool deterministic);

  /**
   * 推論を実行する。
   * @param inputs 入力データ
   * @param outputs 出力データ
   * @param size 評価データの数
   */
  void forward(int32_t* inputs, float* outputs, int32_t size);

  /**
   * CUDAを使用しているならtrueを返す。
   * @return CUDAを使用しているならtrue
   */
  inline bool isCuda() const {
    return _device.is_cuda();
  }

  /**
   * CPUを使用しているならtrueを返す。
   * @return CPUを使用しているならtrue
   */
  inline bool isCpu() const {
    return _cpu;
  }

 private:
  /**
   * 入出力転送の同期用ミューテックス。
   */
  std::mutex _ioMutex;

  /**
   * 推論実行の同期用ミューテックス。
   */
  std::mutex _computeMutex;

  /**
   * モデル。
   */
  torch::jit::script::Module _model;

  /**
   * 実行デバイス。
   */
  at::Device _device;

  /**
   * 実行データ型。
   */
  at::ScalarType _dtype;

  /**
   * ビットシフト用テンソル。
   */
  torch::Tensor _bitShift;

  /**
   * CPUで実行するならtrue。
   */
  bool _cpu;
};

}  // namespace deepgo
