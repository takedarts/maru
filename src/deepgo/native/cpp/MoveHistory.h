#pragma once

#include <cstdint>
#include <vector>

#include "Move.h"

namespace deepgo {

/**
 * Class that holds the move history.
 * The move history retains the last 3 moves.
 */
class MoveHistory {
 public:
  /**
   * Creates an object that holds the move history.
   */
  MoveHistory();

  /**
   * Creates an object that holds a copy of the move history.
   * @param history source move history to copy from
   */
  MoveHistory(const MoveHistory& history);

  /**
   * Destructor.
   */
  virtual ~MoveHistory() = default;

  /**
   * Clears the history.
   */
  void clearMoves();

  /**
   * Adds a move.
   * @param move move to add
   */
  void addMove(Move move);

  /**
   * Returns the move history.
   * @return move history
   */
  std::vector<Move> getMoves() const;

 private:
  /**
   * Index position for the next entry.
   */
  int32_t _index;

  /**
   * List of moves.
   */
  Move _moves[3];
};

}  // namespace deepgo
