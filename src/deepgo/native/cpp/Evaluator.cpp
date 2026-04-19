#include "Evaluator.h"

namespace deepgo {

/**
 * Creates an evaluation result object.
 * @param processor Object to execute inference
 * @param cacheSize Cache size for evaluation results
 * @param komi Komi points
 * @param rule Rule for determining the winner
 * @param superko True to apply the superko rule
 */
Evaluator::Evaluator(
    Processor* processor, int32_t cacheSize, float komi, int32_t rule, bool superko)
    : _mutex(),
      _processor(processor),
      _cacheSize(cacheSize),
      _komi(komi),
      _rule(rule),
      _superko(superko),
      _cacheKeys(),
      _cache() {
}

/**
 * Executes evaluation by the model.
 * @param board Board to be evaluated
 * @param color Color of the stone to be evaluated
 * @return Evaluation result
 */
Evaluation Evaluator::evaluate(Board* board, int32_t color) {
  // Create a hash value.
  uint64_t hash = board->getHash();

  if (color == WHITE) {
    hash ^= 0xffffffffffffffffULL;
  }

  // If there is an evaluation result in the cache, return it.
  {
    std::shared_lock<std::shared_mutex> lock(_mutex);

    auto it = _cache.find(hash);

    if (it != _cache.end()) {
      return it->second;
    }
  }

  // If there is no evaluation result in the cache,
  // execute the evaluation and save it in the cache.
  Evaluation evaluation = _evaluate(board, color);

  {
    std::unique_lock<std::shared_mutex> lock(_mutex);

    while (!_cacheKeys.empty() && _cacheKeys.size() >= static_cast<size_t>(_cacheSize)) {
      _cache.erase(_cacheKeys.front());
      _cacheKeys.pop();
    }

    if (_cache.find(hash) == _cache.end()) {
      _cacheKeys.push(hash);
      _cache.emplace(hash, evaluation);
    }
  }

  return evaluation;
}

/**
 * Executes evaluation by the model.
 * If there is an evaluation result in the cache, it returns that.
 * Otherwise, it executes the evaluation, saves it in the cache, and then returns it.
 * @param board Board to be evaluated
 * @param color Color of the stone to be evaluated
 * @return Evaluation result
 */
Evaluation Evaluator::_evaluate(Board* board, int32_t color) {
  // Get the width and height of the board.
  int32_t width = board->getWidth();
  int32_t height = board->getHeight();

  // Execute evaluation of the current board.
  int32_t inputs[MODEL_INPUT_PACK_SIZE];
  float outputs[MODEL_OUTPUT_SIZE];

  board->getInputs(inputs, color, _komi, _rule, _superko);
  _processor->execute(inputs, outputs, 1);

  // Create a list of candidate moves
  std::unique_ptr<int32_t[]> enableds(new int32_t[width * height]);
  std::unique_ptr<int32_t[]> territories(new int32_t[width * height]);
  int32_t offset_x = (MODEL_SIZE - width) / 2;
  int32_t offset_y = (MODEL_SIZE - height) / 2;
  std::vector<Policy> policies;

  board->getEnableds(enableds.get(), color, true);
  board->getTerritories(territories.get(), color);

  for (int32_t y = 0; y < height; y++) {
    for (int32_t x = 0; x < width; x++) {
      int32_t board_index = y * width + x;
      int32_t model_index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      if (enableds[board_index] == 1 && territories[board_index] == EMPTY) {
        policies.emplace_back(x, y, outputs[model_index], 0);
      }
    }
  }

  // Get the evaluation value.
  float value = outputs[MODEL_PREDICTIONS * MODEL_SIZE * MODEL_SIZE + 0] * 2 - 1;

  // If it is White's turn, invert the evaluation value for Black and White.
  if (color == WHITE) {
    value = -value;
  }

  // Return the evaluation result
  return Evaluation(value, policies);
}

}  // namespace deepgo
