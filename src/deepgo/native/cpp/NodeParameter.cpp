#include "NodeParameter.h"

namespace deepgo {

/**
 * Create a parameter object.
 * @param processor Object to execute inference
 * @param width Board width
 * @param height Board height
 * @param komi Komi points
 * @param rule Rule for determining the winner
 * @param superko True to apply superko rule
 */
NodeParameter::NodeParameter(
    Processor* processor, int32_t width, int32_t height,
    float komi, int32_t rule, bool superko)
    : _processor(processor),
      _width(width),
      _height(height),
      _komi(komi),
      _rule(rule),
      _superko(superko) {
}

/**
 * Return the object to execute inference.
 * @return Object to execute inference
 */
Processor* NodeParameter::getProcessor() {
  return _processor;
}

/**
 * Return the board width.
 * @return Board width
 */
int32_t NodeParameter::getWidth() {
  return _width;
}

/**
 * Return the board height.
 * @return Board height
 */
int32_t NodeParameter::getHeight() {
  return _height;
}

/**
 * Return the komi points.
 * @return Komi points
 */
float NodeParameter::getKomi() {
  return _komi;
}

/**
 * Return the rule for determining the winner.
 * @return Rule for determining the winner
 */
int32_t NodeParameter::getRule() {
  return _rule;
}

/**
 * Return true if applying the superko rule.
 * @return True if applying the superko rule
 */
bool NodeParameter::getSuperko() {
  return _superko;
}

}  // namespace deepgo
