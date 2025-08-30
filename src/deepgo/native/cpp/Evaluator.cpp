#include "Evaluator.h"

namespace deepgo {

/**
 * Creates an evaluation result object.
 * @param processor Object to execute inference
 * @param komi Komi points
 * @param rule Rule for determining the winner
 * @param superko True to apply the superko rule
 */
Evaluator::Evaluator(
    Processor* processor, float komi, int32_t rule, bool superko)
    : _processor(processor),
      _komi(komi),
      _rule(rule),
      _superko(superko),
      _policies(),
      _value(0.0),
      _evaluated(false) {
}

/**
 * Clears the evaluation results by the model.
 */
void Evaluator::clear() {
  _policies.clear();
  _value = 0.0;
  _evaluated = false;
}

/**
 * Executes evaluation by the model.
 * @param board Board to be evaluated
 * @param color Color of the stone to be evaluated
 */
void Evaluator::evaluate(Board* board, int32_t color) {
  // Do nothing if already evaluated.
  if (_evaluated) {
    return;
  }

  // Get the width and height of the board.
  int32_t width = board->getWidth();
  int32_t height = board->getHeight();

  // Execute evaluation of the current board.
  float inputs[MODEL_INPUT_SIZE];
  float outputs[MODEL_OUTPUT_SIZE];

  board->getInputs(inputs, color, _komi, _rule, _superko);
  _processor->execute(inputs, outputs, 1);

  // Create a list of candidate moves
  std::unique_ptr<int32_t[]> enableds(new int32_t[width * height]);
  std::unique_ptr<int32_t[]> territories(new int32_t[width * height]);
  int32_t offset_x = (MODEL_SIZE - width) / 2;
  int32_t offset_y = (MODEL_SIZE - height) / 2;

  board->getEnableds(enableds.get(), color, true);
  board->getTerritories(territories.get(), color);

  for (int32_t y = 0; y < height; y++) {
    for (int32_t x = 0; x < width; x++) {
      int32_t board_index = y * width + x;
      int32_t model_index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      if (enableds[board_index] == 1 && territories[board_index] == EMPTY) {
        _policies.emplace_back(x, y, outputs[model_index], 0);
      }
    }
  }

  // Get the evaluation value.
  _value = outputs[MODEL_PREDICTIONS * MODEL_SIZE * MODEL_SIZE + 0] * 2 - 1;

  // If it is White's turn, invert the evaluation value for Black and White.
  if (color == WHITE) {
    _value = -_value;
  }

  // Set the evaluated flag.
  _evaluated = true;
}

/**
 * Returns true if the evaluation result by the model is set.
 * @return True if the evaluation result by the model is set
 */
bool Evaluator::isEvaluated() {
  return _evaluated;
}

/**
 * Gets the list of predicted candidate moves from the model's inference results.
 * @return List of predicted candidate moves
 */
std::vector<Policy> Evaluator::getPolicies() {
  return _policies;
}

/**
 * Gets the evaluation value from the model's inference results.
 * @return Evaluation value from the model's inference results
 */
float Evaluator::getValue() {
  return _value;
}

}  // namespace deepgo
