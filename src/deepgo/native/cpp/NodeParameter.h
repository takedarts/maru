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
   * @param ucbConstant Constant multiplied to the UCB upper confidence bound
   * @param pucbConstantInit Initial value applied to the PUCB upper confidence bound
   * @param pucbConstantBase Base value applied to the PUCB upper confidence bound
   */
  NodeParameter(
      Processor* processor, int32_t width, int32_t height,
      float komi, int32_t rule, bool superko,
      float ucbConstant, float pucbConstantInit, float pucbConstantBase);

  /**
   * Destroy the parameter object.
   */
  virtual ~NodeParameter() = default;

  /**
   * Return the object to execute inference.
   * @return Object to execute inference
   */
  Processor* getProcessor() const;

  /**
   * Return the board width.
   * @return Board width
   */
  int32_t getWidth() const;

  /**
   * Return the board height.
   * @return Board height
   */
  int32_t getHeight() const;

  /**
   * Return the komi points.
   * @return Komi points
   */
  float getKomi() const;

  /**
   * Return the rule for determining the winner.
   * @return Rule for determining the winner
   */
  int32_t getRule() const;

  /**
   * Return true if applying the superko rule.
   * @return True if applying the superko rule
   */
  bool getSuperko() const;

  /**
   * Return the constant multiplied to the UCB upper confidence bound.
   * @return Constant multiplied to the UCB upper confidence bound
   */
  float getUcbConstant() const;

  /**
   * Return the initial value applied to the PUCB upper confidence bound.
   * @return Initial value applied to the PUCB upper confidence bound
   */
  float getPucbConstantInit() const;

  /**
   * Return the base value applied to the PUCB upper confidence bound.
   * @return Base value applied to the PUCB upper confidence bound
   */
  float getPucbConstantBase() const;

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

  /**
   * Constant multiplied to the UCB upper confidence bound.
   */
  float _ucbConstant;

  /**
   * Initial value applied to the PUCB upper confidence bound.
   */
  float _pucbConstantInit;

  /**
   * Base value applied to the PUCB upper confidence bound.
   */
  float _pucbConstantBase;
};

}  // namespace deepgo
