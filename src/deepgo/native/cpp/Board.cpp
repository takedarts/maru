#include "Board.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

#include "Config.h"
#include "Constant.h"

namespace deepgo {

#define AROUNDS {-1, -_width, 1, _width}

/**
 * Sets the specified bit.
 * @param inputs Bit array
 * @param index Position of the bit to set
 */
inline void setInputBit(int32_t* inputs, int32_t index, int32_t value = 1) {
  inputs[index / 32] |= (value << (index % 32));
}

/**
 * Creates a board object.
 * @param width Width of the board
 * @param height Height of the board
 */
Board::Board(int width, int height)
    : _width(width + 2),
      _height(height + 2),
      _length(_width * _height),
      _renIds(_length, -1),
      _renObjs(_length),
      _areaIds(),
      _areaFlags(),
      _koIndex(-1),
      _koColor(EMPTY),
      _histories(),
      _pattern(width, height),
      _areaUpdated(false),
      _shichoUpdated(false),
      _hash(0),
      _bitBoard() {
  // Create arrays to store data
  _areaIds[0].resize(_length);
  _areaIds[1].resize(_length);
  _areaFlags[0].resize(_length);
  _areaFlags[1].resize(_length);

  // Set boundary data on the outside of the board
  _renObjs[0].color = EDGE;
  _renObjs[0].spaces.insert(-1);
  _renObjs[0].shicho = false;

  for (int32_t i = 0; i < _width; i++) {
    _renIds[i] = 0;
    _renIds[_width * (_height - 1) + i] = 0;
  }

  for (int32_t i = 1; i < _height - 1; i++) {
    _renIds[_width * i] = 0;
    _renIds[_width * i + _width - 1] = 0;
  }

  // Initialize the bitboard
  std::fill(std::begin(_bitBoard), std::end(_bitBoard), 0);
}

/**
 * Creates a copied board object.
 * @param board Source board object to copy from
 */
Board::Board(const Board& board)
    : _width(board._width),
      _height(board._height),
      _length(board._length),
      _renIds(board._renIds),
      _renObjs(board._renObjs),
      _areaIds(),
      _areaFlags(),
      _koIndex(board._koIndex),
      _koColor(board._koColor),
      _histories(),
      _pattern(board._pattern),
      _areaUpdated(false),
      _shichoUpdated(false),
      _hash(0),
      _bitBoard() {
  // Create arrays to store data
  _areaIds[0].resize(_length);
  _areaIds[1].resize(_length);
  _areaFlags[0].resize(_length);
  _areaFlags[1].resize(_length);

  // Copy the board
  copyFrom(&board);
}

/**
 * Initializes the board state.
 */
void Board::clear() {
  // Initialize group information
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      _renIds[index] = -1;
      _renObjs[index].color = EMPTY;
      _renObjs[index].positions.clear();
      _renObjs[index].spaces.clear();
    }
  }

  // Set flags
  _areaUpdated = false;
  _shichoUpdated = false;

  // Initialize ko
  _koIndex = -1;
  _koColor = EMPTY;

  // Initialize history
  _histories[0].clearMoves();
  _histories[1].clearMoves();

  // Initialize stone arrangement information
  _pattern.clear();

  // Initialize the board hash value
  _hash = 0;

  // Initialize the bitboard
  std::fill(std::begin(_bitBoard), std::end(_bitBoard), 0);
}

/**
 * Returns the width of the board.
 * @return Width of the board
 */
int32_t Board::getWidth() const {
  return _width - 2;
}

/**
 * Returns the height of the board.
 * @return Height of the board
 */
int32_t Board::getHeight() const {
  return _height - 2;
}

/**
 * Places a stone.
 * @param move Move information
 * @return Number of captured stones (or -1 if the move is illegal)
 */
int32_t Board::play(Move move) {
  // If passing, reset ko information
  if (!move.isValid(_width - 2, _height - 2)) {
    _koIndex = -1;
    _koColor = EMPTY;
    return 0;
  }

  // Validation
  int32_t index = _getIndex(move.getX(), move.getY());
  int8_t my_color = move.getColor();
  int8_t op_color = OPPOSITE(my_color);

  if (!_isEnabled(index, my_color, false)) {
    return -1;
  }

  // Place the stone
  _put(index, my_color);

  // Add the move coordinate to the history
  if (my_color == BLACK) {
    _histories[0].addMove(move);
  } else if (my_color == WHITE) {
    _histories[1].addMove(move);
  }

  // Update the state around the move coordinate
  int32_t remove_size = 0;

  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    // Do nothing for empty positions
    if (ren_id == -1) {
      continue;
    }
    // If there is a friendly group, merge it
    else if (_renObjs[ren_id].color == my_color && ren_id != _renIds[index]) {
      _mergeRen(index, index + a);
    }
    // If there is an opponent's group with no liberties, remove it
    else if (_renObjs[ren_id].color == op_color && _renObjs[ren_id].spaces.empty()) {
      remove_size += static_cast<int32_t>(_renObjs[ren_id].positions.size());
      _removeRen(index + a);
      _koIndex = index + a;
    }
  }

  // Clear ko condition if 2 or more captured, or placed stone's group size > 1, or placed stone's liberties > 1
  int32_t position_size = static_cast<int32_t>(_renObjs[_renIds[index]].positions.size());
  int32_t space_size = static_cast<int32_t>(_renObjs[_renIds[index]].spaces.size());

  if (remove_size != 1 || position_size > 1 || space_size > 1) {
    _koIndex = -1;
    _koColor = EMPTY;
  } else {
    _koColor = op_color;
  }

  // Reset flags for area information and ladder information
  _areaUpdated = false;
  _shichoUpdated = false;

  return remove_size;
}

/**
 * Returns the coordinates of the ko.
 * Returns (-1, -1) if no ko is in effect.
 * @param color Color of the stone in question
 * @return Coordinates of the ko
 */
std::pair<int32_t, int32_t> Board::getKo(int32_t color) const {
  if (_koIndex != -1 && color == _koColor) {
    return std::make_pair(_getPosX(_koIndex), _getPosY(_koIndex));
  } else {
    return std::make_pair(-1, -1);
  }
}

/**
 * Returns the list of most recent move coordinates.
 * @param color Stone color
 * @return List of move coordinates
 */
std::vector<Move> Board::getHistories(int color) const {
  int32_t history_index = (color == BLACK) ? 0 : 1;
  std::vector<Move> moves;

  for (const Move& move : _histories[history_index].getMoves()) {
    if (move.isValid(_width - 2, _height - 2)) {
      moves.push_back(move);
    }
  }

  return moves;
}

/**
 * Returns the color of the stone at the specified coordinates.
 * @param x X coordinate
 * @param y Y coordinate
 * @return Stone color
 */
int32_t Board::getColor(int32_t x, int32_t y) const {
  return _getColor(_getIndex(x, y));
}

/**
 * Returns the list of stone colors.
 * @param colors Stone color data
 * @param color Stone color
 */
void Board::getColors(int32_t* colors, int32_t color) {
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      colors[y * (_width - 2) + x] = getColor(x, y) * color;
    }
  }
}

/**
 * Returns the size of the group at the specified coordinates.
 * @param x X coordinate
 * @param y Y coordinate
 * @return Size of the group
 */
int32_t Board::getRenSize(int32_t x, int32_t y) {
  int32_t ren_id = _renIds[_getIndex(x, y)];

  if (ren_id == -1) {
    return 0;
  } else {
    return static_cast<int32_t>(_renObjs[ren_id].positions.size());
  }
}

/**
 * Returns the number of liberties of the group at the specified coordinates.
 * @param x X coordinate
 * @param y Y coordinate
 * @return Number of liberties
 */
int32_t Board::getRenSpace(int32_t x, int32_t y) {
  int32_t ren_id = _renIds[_getIndex(x, y)];

  if (ren_id == -1) {
    return 0;
  } else {
    return static_cast<int32_t>(_renObjs[ren_id].spaces.size());
  }
}

/**
 * Returns whether the group at the specified coordinates is in a ladder.
 * @param x X coordinate
 * @param y Y coordinate
 * @return true if the group is in a ladder
 */
bool Board::isShicho(int32_t x, int32_t y) {
  _updateShicho();

  int32_t ren_id = _renIds[_getIndex(x, y)];

  if (ren_id == -1) {
    return false;
  } else {
    return _renObjs[ren_id].shicho;
  }
}

/**
 * Returns true if a stone can be placed at the specified position.
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Stone color
 * @param checkSeki true to check for seki
 * @return true if the move is legal
 */
bool Board::isEnabled(int32_t x, int32_t y, int32_t color, bool checkSeki) {
  return _isEnabled(_getIndex(x, y), color, checkSeki);
}

/**
 * Returns the list of positions where a stone can be placed.
 * @param enableds List of legal positions
 * @param color Stone color
 * @param checkSeki true to check for seki
 */
void Board::getEnableds(int32_t* enableds, int32_t color, bool checkSeki) {
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      if (_isEnabled(_getIndex(x, y), color, checkSeki)) {
        enableds[y * (_width - 2) + x] = 1;
      } else {
        enableds[y * (_width - 2) + x] = 0;
      }
    }
  }
}

/**
 * Returns the settled territory data.
 * @param territories Territory data
 * @param color Reference stone color (setting WHITE returns data with black/white evaluated)
 */
void Board::getTerritories(int32_t* territories, int32_t color) {
  // Update empty area data
  _updateArea();

  // Set territory data
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      int32_t ren_id = _renIds[index];

      // Set settled groups
      if (ren_id != -1 && _renObjs[ren_id].fixed) {
        territories[y * (_width - 2) + x] = _renObjs[ren_id].color * color;
      }
      // Set black settled territory
      else if (_areaIds[0][index] != -1 && _areaFlags[0][_areaIds[0][index]]) {
        territories[y * (_width - 2) + x] = BLACK * color;
      }
      // Set white settled territory
      else if (_areaIds[1][index] != -1 && _areaFlags[1][_areaIds[1][index]]) {
        territories[y * (_width - 2) + x] = WHITE * color;
      }
      // Set unsettled territory
      else {
        territories[y * (_width - 2) + x] = EMPTY;
      }
    }
  }
}

/**
 * Returns the owner data for each coordinate.
 * @param owners Owner data
 * @param color Reference stone color (setting WHITE returns data with black/white inverted)
 * @param rule Scoring rule (RULE_CH: Chinese rules, RULE_JP: Japanese rules, RULE_COM: auto-match rules)
 */
void Board::getOwners(int32_t* owners, int32_t color, int32_t rule) {
  // Get territory data
  getTerritories(owners, color);

  // Set the owner for stones in unsettled territory
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t owner_index = y * (_width - 2) + x;

      if (owners[owner_index] == EMPTY) {
        owners[owner_index] = getColor(x, y) * color;
      }
    }
  }

  // If Japanese rules, finish setting the owner list
  if (rule == RULE_JP) {
    return;
  }

  // Create area data for regions surrounded by a single color
  std::vector<int32_t> areas(_length, EMPTY);
  std::vector<bool> checks(_length, false);

  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      int32_t color = getColor(x, y);

      // Skip if already checked
      // Skip if not an empty position
      if (checks[index] || color != EMPTY) {
        continue;
      }

      // Search for empty area
      std::set<int32_t> positions;
      std::set<int32_t> colors;
      std::vector<int32_t> stack;

      stack.push_back(index);

      while (!stack.empty()) {
        int32_t pos = stack.back();
        stack.pop_back();

        if (checks[pos]) {
          continue;
        }

        checks[pos] = true;
        positions.insert(pos);

        for (auto a : AROUNDS) {
          int32_t target = pos + a;
          int32_t target_color = _getColor(target);

          if (target_color == EMPTY) {
            stack.push_back(target);
          } else if (target_color != EDGE) {
            colors.insert(target_color);
          }
        }
      }

      // If surrounded by a single color, set area data
      if (colors.size() == 1) {
        for (int32_t pos : positions) {
          areas[pos] = *colors.begin();
        }
      }
    }
  }

  // Apply area data to owner data
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      int32_t owner_index = y * (_width - 2) + x;

      if (owners[owner_index] == EMPTY) {
        owners[owner_index] = areas[index] * color;
      }
    }
  }
}

/**
 * Returns the value representing the stone arrangement.
 * @return Value representing the stone arrangement
 */
std::vector<int32_t> Board::getPatterns() {
  return _pattern.values();
}

/**
 * Returns the input data for the model.
 * @param inputs Board data to feed into the model
 * @param color Color of the stone to play
 * @param komi Komi in points
 * @param rule Rule for determining win/loss
 * @param superko true to apply the superko rule
 */
void Board::getInputs(
    int32_t* inputs, int32_t color, float komi, int32_t rule, bool superko) {
  int length = MODEL_SIZE * MODEL_SIZE;
  int32_t offset_x = (MODEL_SIZE - _width + 2) / 2;
  int32_t offset_y = (MODEL_SIZE - _height + 2) / 2;

  // Update ladder information
  _updateShicho();

  // Initialize input data
  for (int32_t i = 0; i < MODEL_INPUT_PACK_SIZE; i++) {
    inputs[i] = 0;
  }

  // Set stone arrangement
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t ren_id = _renIds[_getIndex(x, y)];
      int32_t index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      // Set mask
      setInputBit(inputs, length * MODEL_FEATURES + index);

      // Set value for empty positions.
      if (ren_id == -1) {
        setInputBit(inputs, length * 0 + index);
        continue;
      }

      // Set value for positions with a stone placed.
      int32_t shicho = (_renObjs[ren_id].shicho) ? 1 : 0;
      int32_t size = std::min(static_cast<int32_t>(_renObjs[ren_id].spaces.size()), 8);

      // Set value for black stone positions.
      if (_renObjs[ren_id].color * color == BLACK) {
        setInputBit(inputs, length * 1 + index);
        setInputBit(inputs, length * 2 + index, shicho);
        setInputBit(inputs, length * (2 + size) + index);
      }
      // Set value for white stone positions.
      else if (_renObjs[ren_id].color * color == WHITE) {
        setInputBit(inputs, length * 14 + index);
        setInputBit(inputs, length * 15 + index, shicho);
        setInputBit(inputs, length * (15 + size) + index);
      }
    }
  }

  // Set move history
  std::vector<Move> black_moves = _histories[(1 - color) / 2].getMoves();
  std::vector<Move> white_moves = _histories[(1 + color) / 2].getMoves();

  std::reverse(black_moves.begin(), black_moves.end());
  std::reverse(white_moves.begin(), white_moves.end());

  for (int32_t i = 0; i < black_moves.size(); i++) {
    if (black_moves[i].isValid(_width - 2, _height - 2)) {
      int32_t x = black_moves[i].getX();
      int32_t y = black_moves[i].getY();
      int32_t index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      setInputBit(inputs, length * (11 + i) + index);
    }
  }

  for (int32_t i = 0; i < white_moves.size(); i++) {
    if (white_moves[i].isValid(_width - 2, _height - 2)) {
      int32_t x = white_moves[i].getX();
      int32_t y = white_moves[i].getY();
      int32_t index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      setInputBit(inputs, length * (24 + i) + index);
    }
  }

  // Set information for lines 1-4
  for (int i = 0; i < 4; i++) {
    int32_t begin_x = offset_x + i;
    int32_t end_x = offset_x + _width - 2 - i;
    int32_t begin_y = offset_y + i;
    int32_t end_y = offset_y + _height - 2 - i;

    for (int y = begin_y; y < end_y; y++) {
      setInputBit(inputs, length * (27 + i) + y * MODEL_SIZE + begin_x);
      setInputBit(inputs, length * (27 + i) + y * MODEL_SIZE + end_x - 1);
    }

    for (int x = begin_x; x < end_x; x++) {
      setInputBit(inputs, length * (27 + i) + begin_y * MODEL_SIZE + x);
      setInputBit(inputs, length * (27 + i) + (end_y - 1) * MODEL_SIZE + x);
    }
  }

  // Set ko information
  if (_koColor == color && _koIndex > 0) {
    int32_t x = _getPosX(_koIndex);
    int32_t y = _getPosY(_koIndex);
    int32_t index = y * MODEL_SIZE + x;

    setInputBit(inputs, length * 31 + index);
  }

  // Register the current turn
  int32_t info_offset = (MODEL_FEATURES + 1) * length;

  if (color == BLACK) {
    setInputBit(inputs, info_offset + 0);
  } else {
    setInputBit(inputs, info_offset + 1);
  }

  // Register komi in points
  inputs[MODEL_INPUT_PACK_SIZE - 1] = (int32_t)((komi * color) / 13.0 * 0xfffff);

  // Register whether superko rule applies
  if (superko) {
    setInputBit(inputs, info_offset + 3);
  }

  // Register whether ko has occurred
  if (_koColor == color && _koIndex > 0) {
    setInputBit(inputs, info_offset + 4);
  }

  // Register the win/loss rule
  if (rule != RULE_JP) {
    setInputBit(inputs, info_offset + 5);
  } else {
    setInputBit(inputs, info_offset + 6);
  }
}

/**
 * Returns the board state.
 * @return Board state
 */
std::vector<int32_t> Board::getState() {
  std::vector<int32_t> state;

  // Register the value representing the stone arrangement
  for (int32_t v : _pattern.values()) {
    state.push_back(v);
  }

  // Register ko information
  state.push_back((_koIndex + 1) << 2 | (_koColor + 1));

  // Register move history
  std::vector<Move> black_moves = _histories[0].getMoves();
  std::vector<Move> white_moves = _histories[1].getMoves();

  state.push_back(
      (_getIndex(black_moves[0].getX(), black_moves[0].getY()) + 1) << 20 |
      (_getIndex(black_moves[1].getX(), black_moves[1].getY()) + 1) << 10 |
      (_getIndex(black_moves[2].getX(), black_moves[2].getY()) + 1));
  state.push_back(
      (_getIndex(white_moves[0].getX(), white_moves[0].getY()) + 1) << 20 |
      (_getIndex(white_moves[1].getX(), white_moves[1].getY()) + 1) << 10 |
      (_getIndex(white_moves[2].getX(), white_moves[2].getY()) + 1));

  return state;
}

/**
 * Restores the board state.
 * @param state Board state
 */
void Board::loadState(std::vector<int32_t> state) {
  // Initialize the board
  clear();

  // Place stones
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t pos = y * (_width - 2) + x;
      int32_t index = pos / 16;
      int32_t shift = (pos % 16) * 2;
      int32_t value = state[index] >> shift & 3;

      if (value == 1) {
        play(Move(x, y, BLACK));
      } else if (value == 2) {
        play(Move(x, y, WHITE));
      }
    }
  }

  // Restore ko information
  int32_t ko_info = state[state.size() - 3];

  _koIndex = (ko_info >> 2 & 0x3FFFF) - 1;
  _koColor = (ko_info & 3) - 1;

  // Restore history
  _histories[0].clearMoves();
  _histories[1].clearMoves();

  for (int32_t i = 0; i < 3; i++) {
    int32_t black_history = (state[state.size() - 2] >> (20 - i * 10) & 0x3FF) - 1;
    int32_t white_history = (state[state.size() - 1] >> (20 - i * 10) & 0x3FF) - 1;

    if (black_history != -1) {
      _histories[0].addMove(
          Move(_getPosX(black_history), _getPosY(black_history), BLACK));
    }

    if (white_history != -1) {
      _histories[1].addMove(
          Move(_getPosX(white_history), _getPosY(white_history), WHITE));
    }
  }

  // Initialize flags
  _areaUpdated = false;
  _shichoUpdated = false;
}

/**
 * Copies the board state.
 * @param board Source board to copy from
 */
void Board::copyFrom(const Board* board) {
  // Copy group information
  _renIds = board->_renIds;

  for (int32_t i = 0; i < _length; i++) {
    _renObjs[i] = board->_renObjs[i];
  }

  // Copy ko information
  _koIndex = board->_koIndex;
  _koColor = board->_koColor;

  // Copy board information
  _pattern.copyFrom(board->_pattern);

  // Copy history
  _histories[0] = board->_histories[0];
  _histories[1] = board->_histories[1];

  // Copy the board hash value
  _hash = board->_hash;

  // Copy the bitboard
  for (int i = 0; i < BITBOARD_SIZE; i++) {
    _bitBoard[i] = board->_bitBoard[i];
  }

  // Initialize flags
  _areaUpdated = false;
  _shichoUpdated = false;
}

/**
 * Converts the board state to a string.
 * @return String representation of the board state
 */
std::string Board::toString() const {
  std::stringstream ss;

  ss << "   ";
  for (int32_t x = 0; x < _width - 2; x++) {
    ss << std::setw(2) << x;
  }
  ss << std::endl;

  ss << "  +";
  for (int32_t x = 0; x < _width - 2; x++) {
    ss << "--";
  }
  ss << "-+" << std::endl;

  for (int32_t y = 0; y < _height - 2; y++) {
    ss << std::setw(2) << y << "|";
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      int32_t color = getColor(x, y);

      if (index == _koIndex) {
        ss << " K";
      } else if (color == BLACK) {
        ss << " X";
      } else if (color == WHITE) {
        ss << " O";
      } else {
        ss << " .";
      }
    }
    ss << " |" << std::endl;
  }

  ss << "  +";
  for (int32_t x = 0; x < _width - 2; x++) {
    ss << "--";
  }
  ss << "-+";

  return ss.str();
}

/**
 * Places a stone at the specified position.
 * Does not merge or remove groups.
 * @param index Position index
 * @param color Stone color
 */
void Board::_put(int32_t index, int32_t color) {
  int32_t op_color = OPPOSITE(color);

  // Update the stone arrangement value
  _pattern.put(_getPosX(index), _getPosY(index), color);

  // Update the hash value
  _hash ^= BOARD_HASH_VALUES[(color == BLACK) ? 0 : 1][index];

  // Update the bitboard
  _bitBoard[index / 64] |= (1ULL << (index % 64));

  // Create group information
  _renIds[index] = index;
  _renObjs[index].color = color;
  _renObjs[index].positions.insert(index);

  // Register information to adjacent groups (no merging)
  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    // If there is an empty position nearby, register as liberty
    if (ren_id == -1) {
      _renObjs[index].spaces.insert(index + a);
    }
    // If there is a group nearby, remove liberty
    else {
      _renObjs[ren_id].spaces.erase(index);
    }
  }
}

/**
 * Merges the specified groups.
 * @param srcIndex Position index of the source group
 * @param dstIndex Position index of the destination group
 */
void Board::_mergeRen(int32_t srcIndex, int32_t dstIndex) {
  int32_t src_id = _renIds[srcIndex];
  int32_t dst_id = _renIds[dstIndex];

  // Merge information
  _renObjs[dst_id].positions.insert(
      _renObjs[src_id].positions.begin(), _renObjs[src_id].positions.end());
  _renObjs[dst_id].spaces.insert(
      _renObjs[src_id].spaces.begin(), _renObjs[src_id].spaces.end());

  // Update ID numbers
  for (auto pos : _renObjs[src_id].positions) {
    _renIds[pos] = dst_id;
  }

  // Delete unused information
  _renObjs[src_id].color = EMPTY;
  _renObjs[src_id].positions.clear();
  _renObjs[src_id].spaces.clear();
}

/**
 * Removes the specified group.
 * @param index Position index
 */
void Board::_removeRen(int32_t index) {
  int32_t ren_id = _renIds[index];
  int32_t color = _renObjs[ren_id].color;

  // Execute removal process for all positions
  for (auto pos : _renObjs[ren_id].positions) {
    // Update ID numbers
    _renIds[pos] = -1;

    // Update the stone arrangement value
    _pattern.remove(_getPosX(pos), _getPosY(pos), color);

    // Update the hash value
    _hash ^= BOARD_HASH_VALUES[(color == BLACK) ? 0 : 1][pos];

    // Update the bitboard
    _bitBoard[pos / 64] &= ~(1ULL << (pos % 64));

    // Add liberties to surrounding groups
    for (auto a : AROUNDS) {
      int32_t target_id = _renIds[pos + a];

      if (target_id != -1) {
        _renObjs[target_id].spaces.insert(pos);
      }
    }
  }

  // Delete information
  _renObjs[ren_id].color = EMPTY;
  _renObjs[ren_id].positions.clear();
  _renObjs[ren_id].spaces.clear();
}

/**
 * Updates the empty area information.
 */
void Board::_updateArea() {
  // Do nothing if already updated
  if (_areaUpdated) {
    return;
  }

  // Create area information for both black and white
  for (int32_t c = 0; c < 2; c++) {
    int32_t color = (c == 0) ? BLACK : WHITE;
    int32_t op_color = OPPOSITE(color);

    // Create a list of group IDs
    std::set<int32_t> ren_ids;

    for (int32_t index = 0; index < _length; index++) {
      int32_t ren_id = _renIds[index];

      if (ren_id != -1 && _renObjs[ren_id].color == color) {
        ren_ids.insert(ren_id);
      }
    }

    // Initialize adjacent area information for groups
    // Initialize all groups as settled
    for (int32_t ren_id : ren_ids) {
      _renObjs[ren_id].areas.clear();
      _renObjs[ren_id].fixed = true;
    }

    // Initialize the check state for each position
    std::vector<bool> checks(_length, false);

    // Create area information and register it to group objects
    for (int32_t index = 0; index < _length; index++) {
      // Skip if already checked
      if (checks[index]) {
        continue;
      }

      // Skip if not an empty area
      int32_t index_color = _getColor(index);

      if (index_color != EMPTY && index_color != op_color) {
        _areaIds[c][index] = -1;
        continue;
      }

      // Create a list of IDs of connected groups
      std::set<int32_t> connected_ren_ids;

      for (auto a : AROUNDS) {
        if (_getColor(index + a) == color) {
          connected_ren_ids.insert(_renIds[index + a]);
        }
      }

      // Create area data
      std::vector<int32_t> stack;

      stack.push_back(index);
      _areaFlags[c][index] = true;

      while (!stack.empty()) {
        // Get the position index
        int32_t pos = stack.back();
        stack.pop_back();

        // Skip if already checked
        if (checks[pos]) {
          continue;
        }

        checks[pos] = true;

        // Set the area ID
        _areaIds[c][pos] = index;

        // Get the list of group IDs in the surrounding area
        std::set<int32_t> around_ren_ids;

        for (auto a : AROUNDS) {
          int32_t target_id = _renIds[pos + a];

          if (target_id != -1 && _renObjs[target_id].color == color) {
            around_ren_ids.insert(target_id);
          }
        }

        // If no adjacent groups, mark as unsettled area
        if (around_ren_ids.empty()) {
          _areaFlags[c][pos] = false;
        }

        // If the surrounding and connected group ID lists differ, mark as unsettled area
        if (around_ren_ids != connected_ren_ids) {
          _areaFlags[c][index] = false;
        }

        // Add surrounding empty areas to the stack
        for (auto a : AROUNDS) {
          int32_t around = pos + a;
          int32_t around_color = _getColor(around);

          if (around_color == EMPTY || around_color == op_color) {
            stack.push_back(around);
          }
        }
      }

      // Register area information to the group objects
      if (_areaFlags[c][index]) {
        for (int32_t ren_id : connected_ren_ids) {
          _renObjs[ren_id].areas.insert(index);
        }
      }
    }

    // Set confirmed status for groups and areas
    bool updated = true;

    while (updated) {
      updated = false;

      // Update group information
      // Mark as settled only if connected to 2 or more settled areas
      // If connected to fewer than 2 settled areas, mark connected areas as unsettled
      for (int32_t ren_id : ren_ids) {
        // Do nothing for unsettled groups
        if (!_renObjs[ren_id].fixed) {
          continue;
        }

        // Count the number of connected settled areas
        int32_t fixed_count = 0;

        for (int32_t area_id : _renObjs[ren_id].areas) {
          if (_areaFlags[c][area_id]) {
            fixed_count += 1;
          }
        }

        // If 2 or more connected settled areas, do nothing (group remains settled)
        if (fixed_count >= 2) {
          continue;
        }

        // If fewer than 2 connected settled areas, mark connected areas as unsettled
        _renObjs[ren_id].fixed = false;

        for (int32_t area_id : _renObjs[ren_id].areas) {
          if (_areaFlags[c][area_id]) {
            _areaFlags[c][area_id] = false;
            updated = true;
          }
        }
      }
    }
  }

  // Set the update flag
  _areaUpdated = true;
}

/**
 * Updates the ladder information.
 */
void Board::_updateShicho() {
  // Do nothing if already updated
  if (_shichoUpdated) {
    return;
  }

  // Update ladder information
  for (int32_t index = 0; index < _length; index++) {
    int32_t ren_id = _renIds[index];

    // Skip if the position index differs from the group index
    // One of the position indices in a group always matches the group index
    if (ren_id != index) {
      continue;
    }

    // Determine if the group is in a ladder
    _renObjs[ren_id].shicho = _isShichoRen(index);
  }

  // Set the update flag
  _shichoUpdated = true;
}

/**
 * Returns true if the specified group is in a ladder.
 * @param index Position index
 * @return true if the group is in a ladder
 */
bool Board::_isShichoRen(int32_t index) {
  // If the number of liberties is not 1, it is not a ladder
  if (_renObjs[index].spaces.size() > 1) {
    return false;
  }

  // Verify all moves using depth-first search
  // The escaping side has 1 candidate move, so if the search returns OK, the ladder is confirmed
  // The chasing side has 2 candidate moves, so if the search returns NG, check other branches
  std::vector<Board> stack({*this});

  while (!stack.empty()) {
    // Get the board
    Board board = stack.back();
    stack.pop_back();

    // Get the group ID
    int32_t ren_id = board._renIds[index];
    int32_t color = board._renObjs[ren_id].color;
    int32_t op_color = OPPOSITE(color);

    // Adjacent opponent group has 1 liberty -> NG (can capture opponent stones)
    bool escaped = false;

    for (int32_t pos : board._renObjs[ren_id].positions) {
      for (auto a : AROUNDS) {
        int32_t target_ren_id = board._renIds[pos + a];

        if (target_ren_id != -1 &&
            board._renObjs[target_ren_id].color == op_color &&
            board._renObjs[target_ren_id].spaces.size() == 1) {
          escaped = true;
          break;
        }
      }

      if (escaped) {
        break;
      }
    }

    if (escaped) {
      continue;
    }

    // Create the board after placing the stone
    // No candidate move to escape -> OK (ladder)
    Board curr_board(board);
    int32_t curr_pos = *board._renObjs[ren_id].spaces.begin();
    int32_t curr_pos_x = curr_board._getPosX(curr_pos);
    int32_t curr_pos_y = curr_board._getPosY(curr_pos);

    if (curr_board.play(Move(curr_pos_x, curr_pos_y, color)) < 0) {
      return true;
    }

    // Board after escape has 1 liberty -> OK (ladder)
    // Board after escape has 3 or more liberties -> NG (not a ladder)
    int32_t curr_ren_id = curr_board._renIds[index];

    if (curr_board._renObjs[curr_ren_id].spaces.size() == 1) {
      return true;
    } else if (curr_board._renObjs[curr_ren_id].spaces.size() > 2) {
      continue;
    }

    // Place opponent stones at the liberties and add the resulting boards to the search queue
    for (int32_t next_pos : curr_board._renObjs[curr_ren_id].spaces) {
      Board next_board(curr_board);
      int32_t next_pos_x = next_board._getPosX(next_pos);
      int32_t next_pos_y = next_board._getPosY(next_pos);

      next_board.play(Move(next_pos_x, next_pos_y, op_color));
      stack.push_back(next_board);
    }
  }

  // Not a ladder
  return false;
}

/**
 * Returns the stone color at the specified position.
 * @param index Position index
 * @return Stone color
 */
int32_t Board::_getColor(int32_t index) const {
  int32_t ren_id = _renIds[index];

  if (ren_id == -1) {
    return EMPTY;
  } else {
    return _renObjs[ren_id].color;
  }
}

/**
 * Returns true if a stone can be placed at the specified position.
 * @param index Position index
 * @param color Stone color
 * @param checkSeki true to check for seki
 * @return true if a stone can be placed
 */
bool Board::_isEnabled(int32_t index, int32_t color, bool checkSeki) {
  // Already has a stone -> cannot place
  if (_renIds[index] != -1) {
    return false;
  }

  // Ko target -> cannot place
  if (index == _koIndex && color == _koColor) {
    return false;
  }

  // Seki target -> cannot place
  if (checkSeki && _isSeki(index, color)) {
    return false;
  }

  // Check surroundings
  int32_t op_color = OPPOSITE(color);

  for (auto a : AROUNDS) {
    int32_t target = index + a;

    // If there is space around -> can place
    if (_renIds[target] == -1) {
      return true;
    }

    // Check groups nearby
    BoardRen ren = _renObjs[_renIds[target]];

    // Friendly stone with spare liberties nearby -> can place
    if (ren.color == color && ren.spaces.size() > 1) {
      return true;
    }

    // Capturable opponent stone nearby -> can place
    if (ren.color == op_color && ren.spaces.size() == 1) {
      return true;
    }
  }

  // Cannot place
  return false;
}

/**
 * Returns true if the specified position is subject to seki.
 * @param index Position index
 * @param color Stone color
 * @return true if subject to seki
 */
bool Board::_isSeki(int32_t index, int32_t color) {
  // Check adjacent opponent groups
  // An adjacent opponent group with only 1 liberty at the move position -> NG (can capture opponent stone)
  int32_t op_color = OPPOSITE(color);

  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    if (ren_id != -1 &&
        _renObjs[ren_id].color == op_color &&
        _renObjs[ren_id].spaces.size() == 1) {
      return false;
    }
  }

  // Create a list of adjacent groups
  std::set<int32_t> ren_ids;

  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    if (ren_id != -1 && _renObjs[ren_id].color == color) {
      ren_ids.insert(ren_id);
    }
  }

  // No own groups around the move position -> NG (not a seki candidate)
  if (ren_ids.size() == 0) {
    return false;
  }

  // Pre-move liberty count of the group is 9 or more -> NG (not a seki candidate)
  std::set<int32_t> spaces;

  for (auto id : ren_ids) {
    spaces.insert(_renObjs[id].spaces.begin(), _renObjs[id].spaces.end());

    if (spaces.size() >= 9) {
      return false;
    }
  }

  // Pre-move liberty count of the group is 1 -> NG (not a seki candidate)
  if (spaces.size() == 1) {
    return false;
  }

  // Remove own position from liberties
  spaces.erase(index);

  // 0 liberties remaining is not seki (cannot place stone)
  if (spaces.size() == 0) {
    return false;
  }
  // 1 liberty remaining
  else if (spaces.size() == 1) {
    return _isSekiRen(index, color, ren_ids, *spaces.begin());
  }
  // 2 to 7 liberties remaining
  else {
    return _isSekiArea(index, color, ren_ids, spaces);
  }
}

/**
 * Returns true if the group created by placing a stone at the specified position would be subject to seki.
 * @param index Position index
 * @param color Stone color
 * @param renIds List of group IDs to check
 * @param spaceIndex Position index of the empty area
 * @return true if subject to seki
 */
bool Board::_isSekiRen(
    int32_t index, int32_t color, std::set<int32_t>& renIds, int32_t spaceIndex) {
  // Create a list of opponent groups adjacent to the move position and the liberty
  // Empty position around the move position and liberty -> NG (not a seki candidate)
  int32_t op_color = OPPOSITE(color);
  std::set<int32_t> op_ren_ids;

  for (auto a : AROUNDS) {
    int32_t targets[2] = {index + a, spaceIndex + a};

    for (int32_t target : targets) {
      int32_t ren_id = _renIds[target];

      if (target != index && target != spaceIndex && ren_id == -1) {
        return false;
      }

      if (ren_id != -1 && _renObjs[ren_id].color == op_color) {
        op_ren_ids.insert(ren_id);
      }
    }
  }

  // No opponent groups around the move position and liberty -> NG (not a seki candidate)
  if (op_ren_ids.size() == 0) {
    return false;
  }

  // Opponent group adjacent to move position does not have exactly 2 liberties -> NG (not a seki candidate)
  // Opponent group adjacent to own liberty does not have exactly 2 liberties -> NG (not a seki candidate)
  for (auto ren_id : op_ren_ids) {
    if (_renObjs[ren_id].spaces.size() != 2) {
      return false;
    }
  }

  // Check group positions
  // Own group size is 7 or more -> OK (seki)
  std::set<int32_t> positions;

  positions.insert(index);

  for (auto ren_id : renIds) {
    positions.insert(
        _renObjs[ren_id].positions.begin(), _renObjs[ren_id].positions.end());

    if (positions.size() >= 7) {
      return true;
    }
  }

  // Own group shape is not nakade -> OK (seki)
  if (positions.size() >= 4 && !_isNakade(positions)) {
    return true;
  }

  // Add opponent groups adjacent to own group to the list
  for (auto position : positions) {
    for (auto a : AROUNDS) {
      int32_t ren_id = _renIds[position + a];

      if (ren_id != -1 &&
          _renObjs[ren_id].color == op_color) {
        op_ren_ids.insert(ren_id);
      }
    }
  }

  // Create a list of liberties of opponent groups adjacent to the move position, own liberty, and own group
  std::set<int32_t> op_spaces;

  for (auto ren_id : op_ren_ids) {
    op_spaces.insert(
        _renObjs[ren_id].spaces.begin(), _renObjs[ren_id].spaces.end());
  }

  // Opponent group adjacent to move position has liberty other than move position and own liberty -> OK (seki)
  // Opponent group adjacent to own liberty has liberty other than move position and own liberty -> OK (seki)
  // Opponent group adjacent to own group has liberty other than move position and own liberty -> OK (seki)
  op_spaces.erase(index);
  op_spaces.erase(spaceIndex);

  if (!op_spaces.empty()) {
    return true;
  }

  // Otherwise -> NG (nakade)
  return false;
}

/**
 * Returns true if the area created by placing a stone at the specified position would be subject to seki.
 * @param index Position index
 * @param color Stone color
 * @param renIds List of group IDs to check
 * @param spacesIndices List of position indices in the empty area
 * @return true if subject to seki
 */
bool Board::_isSekiArea(
    int32_t index, int32_t color, std::set<int32_t>& renIds, std::set<int32_t>& spacesIndices) {
  // Create area and adjacent group lists for the board before placing
  int32_t op_color = OPPOSITE(color);
  std::set<int32_t> positions;
  std::set<int32_t> ren_ids;
  std::vector<int32_t> stack;

  positions.insert(index);
  stack.push_back(index);

  for (int32_t space_index : spacesIndices) {
    positions.insert(space_index);
    stack.push_back(space_index);
  }

  while (!stack.empty()) {
    int32_t pos = stack.back();
    stack.pop_back();

    for (auto a : AROUNDS) {
      int32_t target = pos + a;
      int32_t ren_id = _renIds[target];

      if ((ren_id == -1 || _renObjs[ren_id].color == op_color) &&
          positions.find(target) == positions.end()) {
        stack.push_back(target);
        positions.insert(target);
      }

      if (ren_id != -1 && _renObjs[ren_id].color == color) {
        ren_ids.insert(ren_id);
      }
    }

    // Adjacent area size is 9 or more -> NG (not seki)
    if (positions.size() >= 9) {
      return false;
    }
  }

  // Adjacent area is connected to groups other than those connected to the move position -> NG (not seki)
  if (ren_ids != renIds) {
    return false;
  }

  // Groups connected to the move position are connected to only one area, and
  // removing any empty position from the area positions yields nakade -> NG (not a seki candidate)
  if (_isSingleArea(positions, color, -1)) {
    for (int32_t pos : positions) {
      if (_renIds[pos] != -1) {
        continue;
      }

      std::set<int32_t> tmp_positions = positions;
      tmp_positions.erase(pos);

      if (_isNakade(tmp_positions)) {
        return false;
      }
    }
  }

  // Check the adjacent area after placing the stone
  positions.erase(index);

  // Groups connected to the move position are connected to multiple areas -> NG (not seki)
  if (!_isSingleArea(positions, color, index)) {
    return false;
  }

  // Removing any empty position from the area positions yields nakade -> OK (seki)
  for (int32_t pos : positions) {
    if (_renIds[pos] != -1) {
      continue;
    }

    std::set<int32_t> tmp_positions = positions;
    tmp_positions.erase(pos);

    if (_isNakade(tmp_positions)) {
      return true;
    }
  }

  // Otherwise -> NG (not seki)
  return false;
}

/**
 * Returns true if the specified list of position indices represents nakade.
 * @param positions List of position indices
 * @return true if nakade
 */
bool Board::_isNakade(std::set<int32_t>& positions) {
  const int32_t length = 5;
  const int32_t arounds[] = {1, -1, length, -length};
  const int32_t horizontals[] = {1, -1, 1, -1};
  const int32_t verticals[] = {length, length, -length, -length};

  // If the number of positions is 0, it is not nakade
  if (positions.size() == 0) {
    return false;
  }

  // Group size is 7 or more -> NG (not nakade)
  if (positions.size() >= 7) {
    return false;
  }

  // Check the top-left and bottom-right of the positions
  int32_t start_x = _width - 2;
  int32_t start_y = _height - 2;
  int32_t end_x = 0;
  int32_t end_y = 0;

  for (auto p : positions) {
    int32_t x = _getPosX(p);
    int32_t y = _getPosY(p);

    start_x = std::min(x, start_x);
    start_y = std::min(y, start_y);
    end_x = std::max(x, end_x);
    end_y = std::max(y, end_y);
  }

  // Check the distance from top-left to bottom-right
  // If 3x3 or larger, no vital point exists -> not nakade
  if (end_x - start_x > 3 || end_y - start_y > 3) {
    return false;
  }

  // Create a working board
  int32_t board[length * length] = {0};
  int32_t corner[length * length] = {0};

  for (auto p : positions) {
    int32_t src_x = _getPosX(p);
    int32_t src_y = _getPosY(p);
    int32_t dst_x = src_x - start_x + 1;
    int32_t dst_y = src_y - start_y + 1;

    board[dst_y * length + dst_x] = 1;

    if ((src_x == 0 || src_x == _width - 3) &&
        (src_y == 0 || src_y == _height - 3)) {
      corner[dst_y * length + dst_x] = 1;
    }
  }

  // Find the vital point
  for (int32_t y = 1; y < length - 1; y++) {
    for (int32_t x = 1; x < length - 1; x++) {
      int32_t p = y * length + x;

      // Skip if not a target position
      if (board[p] != 1) {
        continue;
      }

      // Count orthogonal connections
      int32_t direct_connections = 0;

      for (auto a : arounds) {
        direct_connections += board[p + a];
      }

      // Count diagonal connections
      int32_t skew_connections = 0;
      int32_t corner_connections = 0;

      for (int32_t i = 0; i < 4; i++) {
        int32_t v = verticals[i];
        int32_t h = horizontals[i];

        // Check target
        if (board[p + v + h] != 1) {
          continue;
        }

        // Corner connection case
        if (corner_connections == 0 && corner[p + v] == 1 && board[p + v] == 1) {
          corner_connections = 1;
        } else if (corner_connections == 0 && corner[p + h] == 1 && board[p + h] == 1) {
          corner_connections = 1;
        }
        // Diagonal connection case
        else if (skew_connections == 0 && board[p + v] == 1 && board[p + h] == 1) {
          skew_connections = 1;
        }
      }

      // If the number of connections is greater than or equal to the specified value, it is determined to be a vital point
      // A position (vital point) satisfying the following conditions exists -> OK (nakade)
      // (1) Stones orthogonally adjacent
      // (2) Stones diagonally adjacent (up to 1)
      // (3) Corner stones diagonally adjacent (up to 1)
      if (direct_connections + skew_connections + corner_connections >= int(positions.size()) - 1) {
        return true;
      }
    }
  }

  return false;
}

/**
 * Returns true if the specified list of position indices is contained in a single area.
 * @param positions List of position indices
 * @param color Color of stones surrounding the area
 * @param excludedIndex Position index to exclude
 * @return true if contained in a single area
 */
bool Board::_isSingleArea(
    std::set<int32_t>& positions, int32_t color, int32_t excludedIndex) {
  int32_t op_color = OPPOSITE(color);
  std::vector<int32_t> stack;
  std::set<int32_t> areas;

  stack.push_back(*positions.begin());
  areas.insert(*positions.begin());

  while (!stack.empty()) {
    int32_t pos = stack.back();
    stack.pop_back();

    for (auto a : AROUNDS) {
      int32_t target = pos + a;
      int32_t ren_id = _renIds[target];

      if ((ren_id == -1 || _renObjs[ren_id].color == op_color) &&
          target != excludedIndex && areas.find(target) == areas.end()) {
        stack.push_back(target);
        areas.insert(target);
      }
    }
  }

  for (auto p : positions) {
    if (areas.find(p) == areas.end()) {
      return false;
    }
  }

  return true;
}

}  // namespace deepgo
