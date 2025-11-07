#pragma once

#include <cstdint>
#include <vector>

namespace deepgo {

class History {
 public:
  /**
   * Create an object to hold move history.
   */
  History();

  /**
   * Create an object to hold copied move history.
   * @param history Source move history to copy from
   */
  History(const History& moves);

  /**
   * Destroy the object.
   */
  virtual ~History() = default;

  /**
   * Initialize the history.
   */
  void clear();

  /**
   * Add a move coordinate.
   * @param move Move coordinate
   */
  void add(int32_t move);

  /**
   * Get the move history.
   * @return Move history
   */
  std::vector<int32_t> get();

 private:
  /**
   * Position to add the value.
   */
  int32_t _index;

  /**
   * List of move coordinates.
   */
  int32_t _moves[3];
};

}  // namespace deepgo
