#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include "History.h"
#include "Pattern.h"
#include "Ren.h"

namespace deepgo {

/**
 * Class of board information.
 */
class Board {
 public:
  /**
   * Creates a board object.
   * @param width Board width
   * @param height Board height
   */
  Board(int width, int height);

  /**
   * Creates a copied board object.
   * @param board Board object to copy from
   */
  Board(const Board& board);

  /**
   * Destroys the board object.
   */
  virtual ~Board() = default;

  /**
   * Initializes the state of the board.
   */
  void clear();

  /**
   * Gets the width of the board.
   * @return Board width
   */
  int32_t getWidth();

  /**
   * Gets the height of the board.
   * @return Board height
   */
  int32_t getHeight();

  /**
   * Play a stone.
   * @param x X coordinate to play
   * @param y Y coordinate to play
   * @param color Color of the stone
   * @return Number of captured stones (returns -1 if not playable)
   */
  int32_t play(int32_t x, int32_t y, int32_t color);

  /**
   * Gets the coordinates of Ko.
   * Returns (-1, -1) if Ko does not occur.
   * @param color Color of the target stone
   * @return Coordinates of Ko
   */
  std::pair<int32_t, int32_t> getKo(int32_t color);

  /**
   * Returns a list of the most recent move coordinates.
   * @param color Color of the stone
   * @return List of move coordinates
   */
  std::vector<std::pair<int32_t, int32_t>> getHistories(int color);

  /**
   * Gets the color of the stone at the specified coordinates.
   * @param x X coordinate
   * @param y Y coordinate
   * @return Color of the stone
   */
  int32_t getColor(int32_t x, int32_t y);

  /**
   * Returns a list of stone colors.
   * @param colors Data of stone colors
   * @param color Color of the stone
   */
  void getColors(int32_t* colors, int32_t color);

  /**
   * Gets the size of the group at the specified coordinates.
   * @param x X coordinate
   * @param y Y coordinate
   * @return Size of the group
   */
  int32_t getRenSize(int32_t x, int32_t y);

  /**
   * Gets the number of liberties of the group at the specified coordinates.
   * @param x X coordinate
   * @param y Y coordinate
   * @return Number of liberties
   */
  int32_t getRenSpace(int32_t x, int32_t y);

  /**
   * Gets whether the group at the specified coordinates is a ladder (shicho).
   * @param x X coordinate
   * @param y Y coordinate
   * @return True if the group is a ladder (shicho)
   */
  bool isShicho(int32_t x, int32_t y);

  /**
   * Returns True if a stone can be placed.
   * @param x X coordinate
   * @param y Y coordinate
   * @param color Color of the stone
   * @param checkSeki True to check for seki
   * @return True if a stone can be placed
   */
  bool isEnabled(int32_t x, int32_t y, int32_t color, bool checkSeki);

  /**
   * Gets a list of places where a stone can be placed.
   * @param enableds List of places where a stone can be placed
   * @param color Color of the stone
   * @param checkSeki True to check for seki
   */
  void getEnableds(int32_t* enableds, int32_t color, bool checkSeki);

  /**
   * Returns the data of fixed territory.
   * @param territories Data of fixed territory
   * @param color Reference stone color (if WHITE is set, returns data judged for both black and white)
   */
  void getTerritories(int32_t* territories, int32_t color);

  /**
   * Returns the data of the owner of each coordinate.
   * @param owners Data of owners
   * @param color Reference stone color (if WHITE is set, returns data judged for both black and white)
   * @param rule Calculation rule (RULE_CH: Chinese rule, RULE_JP: Japanese rule, RULE_COM: automatic match rule)
   */
  void getOwners(int32_t* owners, int32_t color, int32_t rule);

  /**
   * Gets the value representing the arrangement of stones.
   * @return Value representing the arrangement of stones
   */
  std::vector<int32_t> getPatterns();

  /**
   * Gets the data to input to the model.
   * @param inputs Data to input to the model
   * @param color Color of the stone to play
   * @param komi Komi points
   * @param rule Rule for determining the winner
   * @param superko True to apply the superko rule
   */
  void getInputs(float* inputs, int32_t color, float komi, int32_t rule, bool superko);

  /**
   * Gets the state of the board.
   * @return State of the board
   */
  std::vector<int32_t> getState();

  /**
   * Restores the state of the board.
   * @param state State of the board
   */
  void loadState(std::vector<int32_t> state);

  /**
   * Copies the state of the board.
   * @param board Board to copy from
   */
  void copyFrom(const Board* board);

  /**
   * Outputs the state of the board.
   * @param os Output destination
   */
  void print(std::ostream& os = std::cout);

 private:
  /**
   * Value of board width + 2.
   */
  int32_t _width;

  /**
   * Value of board height + 2.
   */
  int32_t _height;

  /**
   * Length of board data.
   * Value is (width + 2) * (height + 2).
   */
  int32_t _length;

  /**
   * List of group information IDs.
   */
  std::unique_ptr<int32_t[]> _renIds;

  /**
   * List of group information.
   */
  std::unique_ptr<Ren[]> _renObjs;

  /**
   * List of empty area information IDs.
   */
  std::unique_ptr<int32_t[]> _areaIds[2];

  /**
   * List of empty area information.
   */
  std::unique_ptr<bool[]> _areaFlags[2];

  /**
   * Location where Ko occurs.
   */
  int32_t _koIndex;

  /**
   * Color subject to Ko.
   */
  int32_t _koColor;

  /**
   * History of move coordinates.
   */
  History _histories[2];

  /**
   * Object representing the arrangement of stones on the board.
   */
  Pattern _pattern;

  /**
   * True if area information has been updated.
   */
  bool _areaUpdated;

  /**
   * True if atari information has been updated.
   */
  bool _shichoUpdated;

  /**
   * Places a stone at the specified location.
   * Does not merge or remove groups.
   * @param index Position number
   * @param color Color of the stone
   */
  void _put(int32_t index, int32_t color);

  /**
   * Merges the specified groups.
   * @param srcIndex Position number of the source group
   * @param dstIndex Position number of the destination group
   */
  void _mergeRen(int32_t srcIndex, int32_t dstIndex);

  /**
   * Removes the specified group.
   * @param index Position number
   */
  void _removeRen(int32_t index);

  /**
   * Updates empty area information.
   */
  void _updateArea();

  /**
   * Updates atari information.
   */
  void _updateShicho();

  /**
   * Returns true if the specified group is a ladder (shicho).
   * @param index Position number
   * @return True if the group is a ladder (shicho)
   */
  bool _isShichoRen(int32_t index);

  /**
   * Gets the color of the stone at the specified location.
   * @param index Position number
   * @return Color of the stone
   */
  int32_t _getColor(int32_t index);

  /**
   * Returns true if a stone can be placed at the specified location.
   * @param index Position number
   * @param color Color of the stone
   * @param checkSeki True to check for seki
   * @return True if a stone can be placed
   */
  bool _isEnabled(int32_t index, int32_t color, bool checkSeki);

  /**
   * Returns true if the specified location is subject to seki.
   * @param index Position number
   * @param color Color of the stone
   * @return True if subject to seki
   */
  bool _isSeki(int32_t index, int32_t color);

  /**
   * Returns true if the group created by placing a stone at the specified location is subject to seki.
   * @param index Position number
   * @param color Color of the stone
   * @param renIds List of group IDs to be judged
   * @param spaceIndex Position number of the empty area
   * @return True if subject to seki
   */
  bool _isSekiRen(int32_t index, int32_t color, std::set<int32_t>& renIds, int32_t spaceIndex);

  /**
   * Returns true if the area created by placing a stone at the specified location is subject to seki.
   * @param index Position number
   * @param color Color of the stone
   * @param renIds List of group IDs to be judged
   * @param spacesIndices List of position numbers of empty areas
   * @return True if subject to seki
   */
  bool _isSekiArea(
      int32_t index, int32_t color, std::set<int32_t>& renIds, std::set<int32_t>& spacesIndices);

  /**
   * Returns true if the specified list of position numbers forms a Nakade.
   * @param positions List of position numbers
   * @return True if it is a Nakade
   */
  bool _isNakade(std::set<int32_t>& positions);

  /**
   * Returns true if the specified list of position numbers is contained within a single area.
   * @param positions List of position numbers
   * @param color Color of the stones surrounding the area
   * @param excludedIndex Position number to exclude
   * @return True if contained within a single area
   */
  bool _isSingleArea(std::set<int32_t>& positions, int32_t color, int32_t excludedIndex);

  /**
   * Returns whether the specified coordinates are a valid position.
   * @param x X coordinate
   * @param y Y coordinate
   * @return True if the position is valid
   */
  inline bool _isValidPosition(int32_t x, int32_t y);

  /**
   * Gets the position number for the specified coordinates.
   * @param x X coordinate
   * @param y Y coordinate
   * @return Position number
   */
  inline int32_t _getIndex(int32_t x, int32_t y);

  /**
   * Gets the X coordinate for the specified position number.
   * @param index Position number
   * @return X coordinate
   */
  inline int32_t _getPosX(int32_t index);

  /**
   * Gets the Y coordinate for the specified position number.
   * @param index Position number
   * @return Y coordinate
   */
  inline int32_t _getPosY(int32_t index);
};

}  // namespace deepgo
