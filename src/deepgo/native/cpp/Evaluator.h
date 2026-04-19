#pragma once

#include <cstdint>
#include <map>
#include <queue>
#include <shared_mutex>

#include "Board.h"
#include "Config.h"
#include "Evaluation.h"
#include "Policy.h"
#include "Processor.h"

namespace deepgo {

/**
 * Class to store evaluation results.
 */
class Evaluator {
 public:
  /**
   * Creates an evaluation result object.
   * @param processor Object to execute inference
   * @param cacheSize Cache size for evaluation results
   * @param komi Komi points
   * @param rule Rule for determining the winner
   * @param superko True to apply the superko rule
   */
  Evaluator(
      Processor* processor, int32_t cacheSize, float komi, int32_t rule, bool superko);

  /**
   * Executes evaluation by the model.
   * If there is an evaluation result in the cache, it returns that.
   * Otherwise, it executes the evaluation, saves it in the cache, and then returns it.
   * @param board Board to be evaluated
   * @param color Color of the stone to be evaluated
   * @param enableShicho True to target atari moves
   * @return Evaluation result
   */
  Evaluation evaluate(Board* board, int32_t color);

 private:
  /**
   * Mutex for synchronization.
   */
  std::shared_mutex _mutex;

  /**
   * Object to execute inference.
   */
  Processor* _processor;

  /**
   * Cache size for evaluation results.
   */
  int32_t _cacheSize;

  /**
   * Komi points.
   */
  float _komi;

  /**
   * Rule for determining the winner.
   */
  int32_t _rule;

  /**
   * True to apply the superko rule.
   */
  bool _superko;

  /**
   * Queue of cache keys.
   * When the cache size exceeds the configured limit,
   * keys are dequeued and used to remove entries from the cache.
   */
  std::queue<uint64_t> _cacheKeys;

  /**
   * Cache of evaluation results.
   * The key is the hash value of the board, and the value is the evaluation result.
   */
  std::map<uint64_t, Evaluation> _cache;

  /**
   * Executes evaluation by the model.
   * @param board Board to be evaluated
   * @param color Color of the stone to be evaluated
   * @param enableShicho True to target atari moves
   * @return Evaluation result
   */
  Evaluation _evaluate(Board* board, int32_t color);
};

}  // namespace deepgo
