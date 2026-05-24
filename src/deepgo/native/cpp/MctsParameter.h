#pragma once

#include <cstdint>

namespace deepgo {

/**
 * Parameter class used when creating search nodes.
 */
class MctsParameter {
 public:
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
  MctsParameter(
      int32_t width, int32_t height, float komi, int32_t rule, bool superko,
      float pucbConstantInit, float pucbConstantBase);

  /**
   * Returns the board width.
   * @return Board width
   */
  int32_t getWidth() const;

  /**
   * Returns the board height.
   * @return Board height
   */
  int32_t getHeight() const;

  /**
   * Returns the komi value.
   * @return Komi value
   */
  float getKomi() const;

  /**
   * Returns the win/loss determination rule.
   * @return Win/loss determination rule
   */
  int32_t getRule() const;

  /**
   * Returns true if the superko rule is applied.
   * @return True if the superko rule is applied
   */
  bool getSuperko() const;

  /**
   * Returns the initial value of the constant multiplied by the PUCB confidence bound.
   * @return Initial value of the constant multiplied by the PUCB confidence bound
   */
  float getPucbConstantInit() const;

  /**
   * Returns the incremental value of the constant multiplied by the PUCB confidence bound.
   * @return Incremental value of the constant multiplied by the PUCB confidence bound
   */
  float getPucbConstantBase() const;

 private:
  /**
   * Board width.
   */
  int32_t _width;

  /**
   * Board height.
   */
  int32_t _height;

  /**
   * Komi value.
   */
  float _komi;

  /**
   * Win/loss determination rule.
   */
  int32_t _rule;

  /**
   * True if the superko rule is applied.
   */
  bool _superko;

  /**
   * Initial value of the constant multiplied by the PUCB confidence bound.
   */
  float _pucbConstantInit;

  /**
   * Incremental value of the constant multiplied by the PUCB confidence bound.
   */
  float _pucbConstantBase;
};

}  // namespace deepgo
