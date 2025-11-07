#pragma once

#include "Processor.h"

namespace deepgo {

/**
 * Parameter class used when creating node objects.
 */
class NodeParameter {
 public:
  /**
   * Create a parameter object.
   * @param processor Object to execute inference
   * @param width Board width
   * @param height Board height
   * @param komi Komi points
   * @param rule Rule for determining the winner
   * @param superko True to apply superko rule
   */
  NodeParameter(
      Processor* processor, int32_t width, int32_t height,
      float komi, int32_t rule, bool superko);

  /**
   * Destroy the parameter object.
   */
  virtual ~NodeParameter() = default;

  /**
   * Return the object to execute inference.
   * @return Object to execute inference
   */
  Processor* getProcessor();

  /**
   * Return the board width.
   * @return Board width
   */
  int32_t getWidth();

  /**
   * Return the board height.
   * @return Board height
   */
  int32_t getHeight();

  /**
   * Return the komi points.
   * @return Komi points
   */
  float getKomi();

  /**
   * Return the rule for determining the winner.
   * @return Rule for determining the winner
   */
  int32_t getRule();

  /**
   * Return true if applying the superko rule.
   * @return True if applying the superko rule
   */
  bool getSuperko();

 private:
  /**
   * Object to execute inference.
   */
  Processor* _processor;

  /**
   * Board width.
   */
  int32_t _width;

  /**
   * Board height.
   */
  int32_t _height;

  /**
   * Komi points.
   */
  float _komi;

  /**
   * Rule for determining the winner.
   */
  int32_t _rule;

  /**
   * True if applying the superko rule.
   */
  bool _superko;
};

}  // namespace deepgo
