#pragma once

#include "Board.h"
#include "Config.h"
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
   * @param komi Komi points
   * @param rule Rule for determining the winner
   * @param superko True to apply the superko rule
   */
  Evaluator(Processor* processor, float komi, int32_t rule, bool superko);

  /**
   * Clears the evaluation results by the model.
   */
  void clear();

  /**
   * Executes evaluation by the model.
   * @param board Board to be evaluated
   * @param color Color of the stone to be evaluated
   * @param enableShicho True to target atari moves
   */
  void evaluate(Board* board, int32_t color);

  /**
   * Returns true if the evaluation result by the model is set.
   * @return True if the evaluation result by the model is set
   */
  bool isEvaluated();

  /**
   * Gets the list of predicted candidate moves from the model's inference results.
   * @return List of predicted candidate moves
   */
  std::vector<Policy> getPolicies();

  /**
   * Gets the evaluation value from the model's inference results.
   * @return Evaluation value from the model's inference results
   */
  float getValue();

 private:
  /**
   * Object to execute inference.
   */
  Processor* _processor;

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
   * List of candidate moves from the model's inference results.
   */
  std::vector<Policy> _policies;

  /**
   * Evaluation value from the model's inference results.
   */
  float _value;

  /**
   * True if the evaluation result by the model is set.
   */
  bool _evaluated;
};

}  // namespace deepgo
