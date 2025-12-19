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
 * @param ucbConstant Constant multiplied to the UCB upper confidence bound
 * @param pucbConstantInit Initial value applied to the PUCB upper confidence bound
 * @param pucbConstantBase Base value applied to the PUCB upper confidence bound
 */
NodeParameter::NodeParameter(
    Processor* processor, int32_t width, int32_t height,
    float komi, int32_t rule, bool superko,
    float ucbConstant, float pucbConstantInit, float pucbConstantBase)
    : _processor(processor),
      _width(width),
      _height(height),
      _komi(komi),
      _rule(rule),
      _superko(superko),
      _ucbConstant(ucbConstant),
      _pucbConstantInit(pucbConstantInit),
      _pucbConstantBase(pucbConstantBase) {
}

/**
 * Return the object to execute inference.
 * @return Object to execute inference
 */
Processor* NodeParameter::getProcessor() const {
  return _processor;
}

/**
 * Return the board width.
 * @return Board width
 */
int32_t NodeParameter::getWidth() const {
  return _width;
}

/**
 * Return the board height.
 * @return Board height
 */
int32_t NodeParameter::getHeight() const {
  return _height;
}

/**
 * Return the komi points.
 * @return Komi points
 */
float NodeParameter::getKomi() const {
  return _komi;
}

/**
 * Return the rule for determining the winner.
 * @return Rule for determining the winner
 */
int32_t NodeParameter::getRule() const {
  return _rule;
}

/**
 * Return true if applying the superko rule.
 * @return True if applying the superko rule
 */
bool NodeParameter::getSuperko() const {
  return _superko;
}

/**
 * Return the constant multiplied to the UCB upper confidence bound.
 * @return Constant multiplied to the UCB upper confidence bound
 */
float NodeParameter::getUcbConstant() const {
  return _ucbConstant;
}

/**
 * Return the initial value applied to the PUCB upper confidence bound.
 * @return Initial value applied to the PUCB upper confidence bound
 */
float NodeParameter::getPucbConstantInit() const {
  return _pucbConstantInit;
}

/**
 * Return the base value applied to the PUCB upper confidence bound.
 * @return Base value applied to the PUCB upper confidence bound
 */
float NodeParameter::getPucbConstantBase() const {
  return _pucbConstantBase;
}

}  // namespace deepgo
