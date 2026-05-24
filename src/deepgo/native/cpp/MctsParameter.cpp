#include "MctsParameter.h"

namespace deepgo {

/**
 * Creates a parameter object.
 * @param width Board width
 * @param height Board height
 * @param komi Komi value
 * @param rule Win/loss determination rule
 * @param superko True if the superko rule is applied
 * @param pucbConstantInit Initial value of the constant multiplied by the PUCB confidence bound
 * @param pucbConstantBase Incremental value of the constant multiplied by the PUCB confidence bound
 */
MctsParameter::MctsParameter(
    int32_t width, int32_t height, float komi, int32_t rule, bool superko,
    float pucbConstantInit, float pucbConstantBase)
    : _width(width),
      _height(height),
      _komi(komi),
      _rule(rule),
      _superko(superko),
      _pucbConstantInit(pucbConstantInit),
      _pucbConstantBase(pucbConstantBase) {
}

/**
 * Returns the board width.
 * @return Board width
 */
int32_t MctsParameter::getWidth() const {
  return _width;
}

/**
 * Returns the board height.
 * @return Board height
 */
int32_t MctsParameter::getHeight() const {
  return _height;
}

/**
 * Returns the komi value.
 * @return Komi value
 */
float MctsParameter::getKomi() const {
  return _komi;
}

/**
 * Returns the win/loss determination rule.
 * @return Win/loss determination rule
 */
int32_t MctsParameter::getRule() const {
  return _rule;
}

/**
 * Returns true if the superko rule is applied.
 * @return True if the superko rule is applied
 */
bool MctsParameter::getSuperko() const {
  return _superko;
}

/**
 * Returns the initial value of the constant multiplied by the PUCB confidence bound.
 * @return Initial value of the constant multiplied by the PUCB confidence bound
 */
float MctsParameter::getPucbConstantInit() const {
  return _pucbConstantInit;
}

/**
 * Returns the incremental value of the constant multiplied by the PUCB confidence bound.
 * @return Incremental value of the constant multiplied by the PUCB confidence bound
 */
float MctsParameter::getPucbConstantBase() const {
  return _pucbConstantBase;
}

}  // namespace deepgo
