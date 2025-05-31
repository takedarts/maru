#include "Executor.h"

#include <iostream>

#include "Config.h"

namespace deepgo {

/**
 * 推論実行オブジェクトを生成する。
 * @param model モデルファイル
 * @param gpu GPU番号
 * @param batchSize バッチサイズの最大値
 * @param fp16 16bit精度で計算するならTrue
 * @param deterministic 計算結果を再現可能にするならTrue
 */
Executor::Executor(
    std::string model, int32_t gpu, int32_t batchSize, bool fp16, bool deterministic)
    : _mutex(),
      _condition(),
      _model(new Model(model, gpu, fp16, deterministic)),
      _thread(),
      _terminated(false),
      _queue(),
      _waitingCount(0),
      _reservedCount(0),
      _batchSize(batchSize) {
  _thread.reset(new std::thread([this]() { this->_run(); }));
}

/**
 * デストラクタ。
 */
Executor::~Executor() {
  // 終了フラグを立てる。
  {  // synchronize
    std::unique_lock<std::mutex> lock(_mutex);
    _terminated = true;
    _condition.notify_all();
  }

  // スレッドの停止を待機する。
  _thread->join();
}

/**
 * 推論を実行する。
 * @param inputs 入力データ
 * @param outputs 出力データ
 * @param size 入出力データの数
 */
void Executor::execute(float* inputs, float* outputs, int32_t size) {
  // 計算オブジェクトを生成する。
  std::unique_ptr<ExecutorJob> job(new ExecutorJob(inputs, outputs, size));

  // キューに追加する。
  {  // synchronize
    std::unique_lock<std::mutex> lock(_mutex);
    _queue.push(job.get());
    _waitingCount += job->getSize();
    _reservedCount = std::max(0, _reservedCount - job->getSize());
    _condition.notify_all();
  }

  // 計算が完了するまで待機する。
  job->wait();
}

/**
 * 待機中の計算処理の数を返す。
 * @return 待機中の計算処理の数
 */
int32_t Executor::getWaitingCount() {
  std::unique_lock<std::mutex> lock(_mutex);
  return _waitingCount + _reservedCount;
}

/**
 * 計算処理の予約数を増やす。
 * @param reservedCount 予約数
 */
void Executor::addReservedCount(int32_t reservedCount) {
  std::unique_lock<std::mutex> lock(_mutex);
  _reservedCount += reservedCount;
}

/**
 * スレッドで実行されるメソッド。
 */
void Executor::_run() {
  try {
    while (true) {
      std::vector<ExecutorJob*> jobs;

      // キューの状態と実行状態を確認する
      {  // synchronize
        // キューが空の場合は待機する。
        std::unique_lock<std::mutex> lock(_mutex);
        _condition.wait(lock, [this] { return !_queue.empty() || _terminated; });

        // 終了フラグが立っている場合は終了する。
        if (_terminated) {
          // キューに残っている計算オブジェクトを全て取り出して終了を通知する。
          while (!_queue.empty()) {
            ExecutorJob* job = _queue.front();
            _queue.pop();
            job->notify();
          }

          break;
        }

        // キューから計算オブジェクトを取り出す。
        int32_t jobs_size = 0;

        while (!_queue.empty() && jobs_size < _batchSize) {
          ExecutorJob* job = _queue.front();
          _queue.pop();
          _waitingCount -= job->getSize();
          jobs.push_back(job);
          jobs_size += job->getSize();
        }
      }

      // 計算を実行する。
      _forward(jobs);

      // 計算が完了したことを通知する。
      for (auto job : jobs) {
        job->notify();
      }
    }
  } catch (std::exception& e) {
    std::cerr << "Executor::_run : " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Executor::_run : unknown error" << std::endl;
  }
}

/**
 * 計算を実行する。
 * @param jobs 計算タスクのリスト
 */
void Executor::_forward(std::vector<ExecutorJob*>& jobs) {
  // 入力データの大きさを計算する
  int32_t jobs_size = 0;

  for (auto job : jobs) {
    jobs_size += job->getSize();
  }

  // 入力データと出力データの領域を確保する。
  std::unique_ptr<float[]> all_inputs(new float[jobs_size * MODEL_INPUT_SIZE]);
  std::unique_ptr<float[]> all_outputs(new float[jobs_size * MODEL_OUTPUT_SIZE]);

  // 入力データを作成する。
  int32_t input_offset = 0;

  for (auto job : jobs) {
    float* inputs = job->getInputs();
    int32_t size = job->getSize();

    memcpy(
        all_inputs.get() + input_offset * MODEL_INPUT_SIZE,
        inputs,
        size * MODEL_INPUT_SIZE * sizeof(float));

    input_offset += size;
  }

  // 計算を実行する。
  _model->forward(all_inputs.get(), all_outputs.get(), jobs_size);

  // 出力データを反映する。
  int32_t out_offset = 0;

  for (auto job : jobs) {
    float* outputs = job->getOutputs();
    int32_t size = job->getSize();

    memcpy(
        outputs,
        all_outputs.get() + out_offset * MODEL_OUTPUT_SIZE,
        size * MODEL_OUTPUT_SIZE * sizeof(float));

    out_offset += size;
  }
}

}  // namespace deepgo
