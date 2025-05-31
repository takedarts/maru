#pragma once

#include <ATen/ATen.h>
#include <torch/script.h>
#include <torch/torch.h>

#include <cstdint>
#include <string>

namespace deepgo {

class Model {
 public:
  /**
   * コンストラクタ。
   * CPUを使って計算するオブジェクトを生成する。
   * @param filename モデルファル
   * @param gpu GPU番号
   * @param fp16 16bit精度で計算するならTrue
   * @param deterministic 計算結果を再現可能にするならTrue
   */
  Model(std::string filename, int32_t gpu, bool fp16, bool deterministic);

  /**
   * デストラクタ。
   */
  virtual ~Model() = default;

  /**
   * 推論を実行する。
   * @param inputs 入力データ
   * @param outputs 出力データ
   * @param size 評価データの数
   */
  virtual void forward(float* inputs, float* outputs, uint32_t size);

  /**
   * GPUを使うならTrueを返す。
   * @return GPUを使うなら1, 使わないなら0
   */
  virtual int32_t isCuda();

 private:
  /**
   * 推論モデル。
   */
  torch::jit::script::Module _model;

  /**
   * 実行デバイス。
   */
  at::Device _device;

  /**
   * 計算で使用するデータ型。
   */
  at::ScalarType _dtype;
};

}  // namespace deepgo
