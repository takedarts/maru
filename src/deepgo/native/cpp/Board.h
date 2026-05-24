#pragma once

#include <array>
#include <cstdint>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "BoardPattern.h"
#include "BoardRen.h"
#include "Config.h"
#include "Move.h"
#include "MoveHistory.h"

namespace deepgo {

#define BITBOARD_SIZE (MODEL_SIZE * MODEL_SIZE / 64 + 2)

/**
 * Class that holds the board state.
 */
class Board {
 private:
  /**
   * Class for computing the hash value of the board.
   * Declared as a friend class of Board to allow access to its private members.
   */
  friend class BoardHash;

 public:
  /**
   * Creates a board object.
   * @param width Width of the board
   * @param height Height of the board
   */
  Board(int width, int height);

  /**
   * Creates a copied board object.
   * @param board Source board object to copy from
   */
  Board(const Board& board);

  /**
   * Destroys the board object.
   */
  virtual ~Board() = default;

  /**
   * Initializes the board state.
   */
  void clear();

  /**
   * Returns the width of the board.
   * @return Width of the board
   */
  int32_t getWidth() const;

  /**
   * Returns the height of the board.
   * @return Height of the board
   */
  int32_t getHeight() const;

  /**
   * Places a stone.
   * @param move Move information
   * @return Number of captured stones (or -1 if the move is illegal)
   */
  int32_t play(Move move);

  /**
   * Returns the coordinates of the ko.
   * Returns (-1, -1) if no ko is in effect.
   * @param color Color of the stone in question
   * @return Coordinates of the ko
   */
  std::pair<int32_t, int32_t> getKo(int32_t color) const;

  /**
   * Returns the list of most recent move coordinates.
   * @param color Stone color
   * @return List of move coordinates
   */
  std::vector<Move> getHistories(int color) const;

  /**
   * Returns the color of the stone at the specified coordinates.
   * @param x X coordinate
   * @param y Y coordinate
   * @return Color of the stone
   */
  int32_t getColor(int32_t x, int32_t y) const;

  /**
   * Returns the list of stone colors.
   * @param colors Stone color data
   * @param color Stone color
   */
  void getColors(int32_t* colors, int32_t color);

  /**
   * Returns the size of the group at the specified coordinates.
   * @param x X coordinate
   * @param y Y coordinate
   * @return Size of the group
   */
  int32_t getRenSize(int32_t x, int32_t y);

  /**
   * Returns the number of liberties of the group at the specified coordinates.
   * @param x X coordinate
   * @param y Y coordinate
   * @return Number of liberties
   */
  int32_t getRenSpace(int32_t x, int32_t y);

  /**
   * Returns whether the group at the specified coordinates is in a ladder.
   * @param x X coordinate
   * @param y Y coordinate
   * @return true if the group is in a ladder
   */
  bool isShicho(int32_t x, int32_t y);

  /**
   * Returns true if a stone can be placed at the specified position.
   * @param x X coordinate
   * @param y Y coordinate
   * @param color Stone color
   * @param checkSeki true to check for seki
   * @return true if the move is legal
   */
  bool isEnabled(int32_t x, int32_t y, int32_t color, bool checkSeki);

  /**
   * Returns the list of positions where a stone can be placed.
   * @param enableds List of legal positions
   * @param color Stone color
   * @param checkSeki true to check for seki
   */
  void getEnableds(int32_t* enableds, int32_t color, bool checkSeki);

  /**
   * Returns the settled territory data.
   * @param territories Territory data
   * @param color Reference stone color (setting WHITE returns data with black/white evaluated)
   */
  void getTerritories(int32_t* territories, int32_t color);

  /**
   * Returns the owner data for each coordinate.
   * @param owners Owner data
   * @param color Reference stone color (setting WHITE returns data with black/white evaluated)
   * @param rule Scoring rule (RULE_CH: Chinese rules, RULE_JP: Japanese rules, RULE_COM: auto-match rules)
   */
  void getOwners(int32_t* owners, int32_t color, int32_t rule);

  /**
   * Returns the value representing the stone arrangement.
   * @return Value representing the stone arrangement
   */
  std::vector<int32_t> getPatterns();

  /**
   * Returns the input data for the model.
   * @param inputs Board data to feed into the model
   * @param color Color of the stone to play
   * @param komi Komi in points
   * @param rule Rule for determining win/loss
   * @param superko true to apply the superko rule
   */
  void getInputs(
      int32_t* inputs, int32_t color, float komi, int32_t rule, bool superko);

  /**
   * Returns the board state.
   * @return Board state
   */
  std::vector<int32_t> getState();

  /**
   * Restores the board state.
   * @param state Board state
   */
  void loadState(std::vector<int32_t> state);

  /**
   * Copies the board state.
   * @param board Source board to copy from
   */
  void copyFrom(const Board* board);

  /**
   * Converts the board state to a string.
   * @return String representation of the board state
   */
  std::string toString() const;

  /**
   * Writes the string representation of the board state to an output stream.
   * @param os Output stream
   * @param board Board object
   * @return Output stream
   */
  friend std::ostream& operator<<(std::ostream& os, const Board& board) {
    os << board.toString();
    return os;
  }

 private:
  /**
   * Width of the board plus 2.
   */
  int32_t _width;

  /**
   * Height of the board plus 2.
   */
  int32_t _height;

  /**
   * Length of the board data array.
   * Equals (width + 2) * (height + 2).
   */
  int32_t _length;

  /**
   * List of group ID numbers.
   */
  std::vector<int32_t> _renIds;

  /**
   * List of group objects.
   */
  std::vector<BoardRen> _renObjs;

  /**
   * List of empty area ID numbers.
   */
  std::array<std::vector<int32_t>, 2> _areaIds;

  /**
   * List of empty area flags.
   */
  std::array<std::vector<bool>, 2> _areaFlags;

  /**
   * Position where ko is in effect.
   */
  int32_t _koIndex;

  /**
   * Color subject to the ko restriction.
   */
  int32_t _koColor;

  /**
   * History of move coordinates.
   */
  MoveHistory _histories[2];

  /**
   * Object representing the arrangement of stones on the board.
   */
  BoardPattern _pattern;

  /**
   * true if the area information has been updated.
   */
  bool _areaUpdated;

  /**
   * true if the ladder information has been updated.
   */
  bool _shichoUpdated;

  /**
   * Hash value of the board.
   * Represents only the stone placement; does not include ko information.
   */
  uint64_t _hash;

  /**
   * Bitboard representing the placement of stones on the board.
   * Each bit corresponds to one cell; 1 if a stone is placed, 0 if empty.
   */
  uint64_t _bitBoard[BITBOARD_SIZE];

  /**
   * Places a stone at the specified position.
   * Does not merge or remove groups.
   * @param index Position index
   * @param color Stone color
   */
  void _put(int32_t index, int32_t color);

  /**
   * Merges the specified groups.
   * @param srcIndex Position index of the source group
   * @param dstIndex Position index of the destination group
   */
  void _mergeRen(int32_t srcIndex, int32_t dstIndex);

  /**
   * Removes the specified group.
   * @param index Position index
   */
  void _removeRen(int32_t index);

  /**
   * Updates the empty area information.
   */
  void _updateArea();

  /**
   * Updates the ladder information.
   */
  void _updateShicho();

  /**
   * Returns true if the specified group is in a ladder.
   * @param index Position index
   * @return True if the group is in a ladder
   */
  bool _isShichoRen(int32_t index);

  /**
   * Returns the color of the stone at the specified position.
   * @param index Position index
   * @return Stone color
   */
  int32_t _getColor(int32_t index) const;

  /**
   * Returns true if a stone can be placed at the specified position.
   * @param index Position index
   * @param color Stone color
   * @param checkSeki true to check for seki
   * @return true if the move is legal
   */
  bool _isEnabled(int32_t index, int32_t color, bool checkSeki);

  /**
   * Returns true if the specified position is subject to seki.
   * @param index Position index
   * @param color Stone color
   * @return True if the position is subject to seki
   */
  bool _isSeki(int32_t index, int32_t color);

  /**
   * Returns true if the group formed by placing a stone at the specified position is subject to seki.
   * @param index Position index
   * @param color Stone color
   * @param renIds List of group IDs to evaluate
   * @param spaceIndex Position index of the empty area
   * @return True if subject to seki
   */
  bool _isSekiRen(int32_t index, int32_t color, std::set<int32_t>& renIds, int32_t spaceIndex);

  /**
   * Returns true if the area formed by placing a stone at the specified position is subject to seki.
   * @param index Position index
   * @param color Stone color
   * @param renIds List of group IDs to evaluate
   * @param spacesIndices List of empty area position indices
   * @return True if subject to seki
   */
  bool _isSekiArea(
      int32_t index, int32_t color, std::set<int32_t>& renIds, std::set<int32_t>& spacesIndices);

  /**
   * Returns true if the specified list of position indices forms a nakade.
   * @param positions List of position indices
   * @return True if the positions form a nakade
   */
  bool _isNakade(std::set<int32_t>& positions);

  /**
   * Returns true if the specified list of position indices is contained within a single area.
   * @param positions List of position indices
   * @param color Color of the stones surrounding the area
   * @param excludedIndex Position index to exclude
   * @return True if all positions are in a single area
   */
  bool _isSingleArea(std::set<int32_t>& positions, int32_t color, int32_t excludedIndex);

  /**
   * Returns the position index for the specified coordinates.
   * @param x X coordinate
   * @param y Y coordinate
   * @return Position index
   */
  inline int32_t _getIndex(int32_t x, int32_t y) const {
    return ((y + 1) * _width) + (x + 1);
  }

  /**
   * Returns the X coordinate of the specified position index.
   * @param index Position index
   * @return X coordinate
   */
  inline int32_t _getPosX(int32_t index) const {
    return (index % _width) - 1;
  }

  /**
   * Returns the Y coordinate of the specified position index.
   * @param index Position index
   * @return Y coordinate
   */
  inline int32_t _getPosY(int32_t index) const {
    return (index / _width) - 1;
  }
};

}  // namespace deepgo
