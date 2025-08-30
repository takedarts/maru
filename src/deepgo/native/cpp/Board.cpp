#include "Board.h"

#include <algorithm>
#include <cstring>

#include "Config.h"

namespace deepgo {

#define AROUNDS {-1, -_width, 1, _width}

/**
 * Create a board object.
 * @param width Board width
 * @param height Board height
 */
Board::Board(int width, int height)
    : _width(width + 2),
      _height(height + 2),
      _length(_width * _height),
      _renIds(new int32_t[_length]),
      _renObjs(new Ren[_length]),
      _areaIds(),
      _areaFlags(),
      _koIndex(-1),
      _koColor(EMPTY),
      _histories(),
      _pattern(width, height),
      _areaUpdated(false),
      _shichoUpdated(false) {
  // Create arrays to store data
  _areaIds[0].reset(new int32_t[_length]);
  _areaIds[1].reset(new int32_t[_length]);
  _areaFlags[0].reset(new bool[_length]);
  _areaFlags[1].reset(new bool[_length]);

  // Initialize ren data
  for (int32_t i = 0; i < _length; i++) {
    _renIds[i] = -1;
  }

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
}

/**
 * Create a copied board object.
 * @param board Source board object to copy from
 */
Board::Board(const Board& board)
    : _width(board._width),
      _height(board._height),
      _length(board._length),
      _renIds(new int32_t[_length]),
      _renObjs(new Ren[_length]),
      _areaIds(),
      _areaFlags(),
      _koIndex(board._koIndex),
      _koColor(board._koColor),
      _histories(),
      _pattern(board._pattern),
      _areaUpdated(false),
      _shichoUpdated(false) {
  // Create arrays to store data
  _areaIds[0].reset(new int32_t[_length]);
  _areaIds[1].reset(new int32_t[_length]);
  _areaFlags[0].reset(new bool[_length]);
  _areaFlags[1].reset(new bool[_length]);

  // Duplicate the board
  copyFrom(&board);
}

/**
 * Initialize the board state.
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
  _histories[0].clear();
  _histories[1].clear();

  // Initialize stone arrangement information
  _pattern.clear();
}

/**
 * Get the board width.
 * @return Board width
 */
int32_t Board::getWidth() {
  return _width - 2;
}

/**
 * Get the board height.
 * @return Board height
 */
int32_t Board::getHeight() {
  return _height - 2;
}

/**
 * Place a stone.
 * @param x X coordinate to place the stone
 * @param y Y coordinate to place the stone
 * @param color Stone color
 * @return Number of captured stones (-1 if not allowed)
 */
int32_t Board::play(int32_t x, int32_t y, int32_t color) {
  // Reset ko information if pass
  if (!_isValidPosition(x, y)) {
    _koIndex = -1;
    _koColor = EMPTY;
    return 0;
  }

  // Check if the specified coordinates are a valid move
  int32_t index = _getIndex(x, y);
  int32_t op_color = OPPOSITE(color);

  if (!_isEnabled(index, color, false)) {
    return -1;
  }

  // Place the stone
  _put(index, color);

  // Add move coordinates to history
  if (color == BLACK) {
    _histories[0].add(index);
  } else if (color == WHITE) {
    _histories[1].add(index);
  }

  // Update the state around the move coordinates
  int32_t remove_size = 0;

  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    // Do nothing if empty coordinate
    if (ren_id == -1) {
      continue;
    }
    // Merge if own group exists
    else if (_renObjs[ren_id].color == color && ren_id != _renIds[index]) {
      _mergeRen(index, index + a);
    }
    // Remove if opponent's group exists and has no liberties
    else if (_renObjs[ren_id].color == op_color && _renObjs[ren_id].spaces.empty()) {
      remove_size += _renObjs[ren_id].positions.size();
      _removeRen(index + a);
      _koIndex = index + a;
    }
  }

  // If two or more are removed, or the placed stone's group has two or more,
  // or the placed stone has two or more liberties, clear ko judgment
  int32_t position_size = _renObjs[_renIds[index]].positions.size();
  int32_t space_size = _renObjs[_renIds[index]].spaces.size();

  if (remove_size != 1 || position_size > 1 || space_size > 1) {
    _koIndex = -1;
    _koColor = EMPTY;
  } else {
    _koColor = op_color;
  }

  // Reset flags for territory and ladder information
  _areaUpdated = false;
  _shichoUpdated = false;

  return remove_size;
}

/**
 * Get the ko coordinates.
 * If ko has not occurred, returns (-1, -1).
 * @param color Target stone color
 * @return Ko coordinates
 */
std::pair<int32_t, int32_t> Board::getKo(int32_t color) {
  if (_koIndex != -1 && color == _koColor) {
    return std::make_pair(_getPosX(_koIndex), _getPosY(_koIndex));
  } else {
    return std::make_pair(-1, -1);
  }
}

/**
 * Return the list of most recent move coordinates.
 * @param color Stone color
 * @return List of move coordinates
 */
std::vector<std::pair<int32_t, int32_t>> Board::getHistories(int color) {
  std::vector<std::pair<int32_t, int32_t>> moves;
  int32_t history_index = (color == BLACK) ? 0 : 1;

  for (int32_t index : _histories[history_index].get()) {
    int32_t x = _getPosX(index);
    int32_t y = _getPosY(index);

    if (_isValidPosition(x, y)) {
      moves.push_back(std::make_pair(x, y));
    }
  }

  return moves;
}

/**
 * Get the color of the stone at the specified coordinates.
 * @param x X coordinate
 * @param y Y coordinate
 * @return Stone color
 */
int32_t Board::getColor(int32_t x, int32_t y) {
  return _getColor(_getIndex(x, y));
}

/**
 * Return the list of stone colors.
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
 * Get the size of the group at the specified coordinates.
 * @param x X coordinate
 * @param y Y coordinate
 * @return Group size
 */
int32_t Board::getRenSize(int32_t x, int32_t y) {
  int32_t ren_id = _renIds[_getIndex(x, y)];

  if (ren_id == -1) {
    return 0;
  } else {
    return _renObjs[ren_id].positions.size();
  }
}

/**
 * Get the number of dead stones in the group at the specified coordinates.
 * @param x X coordinate
 * @param y Y coordinate
 * @return Number of dead stones
 */
int32_t Board::getRenSpace(int32_t x, int32_t y) {
  int32_t ren_id = _renIds[_getIndex(x, y)];

  if (ren_id == -1) {
    return 0;
  } else {
    return _renObjs[ren_id].spaces.size();
  }
}

/**
 * Get the presence of a shicho (ladder) at the specified coordinates.
 * @param x X coordinate
 * @param y Y coordinate
 * @return True if shicho exists, false otherwise
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
 * Return true if a stone can be placed.
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Stone color
 * @param checkSeki True to check for seki
 * @return True if a stone can be placed
 */
bool Board::isEnabled(int32_t x, int32_t y, int32_t color, bool checkSeki) {
  return _isEnabled(_getIndex(x, y), color, checkSeki);
}

/**
 * Get the list of places where stones can be placed.
 * @param enableds List of places where stones can be placed
 * @param color Stone color
 * @param checkSeki True to check for seki
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
 * Return the data of fixed territories.
 * @param territories Data of fixed territories
 * @param color Reference stone color (set WHITE to return data judged for black and white)
 */
void Board::getTerritories(int32_t* territories, int32_t color) {
  // Update empty area data
  _updateArea();

  // Set data for confirmed territories
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      int32_t ren_id = _renIds[index];

      // Set fixed group
      if (ren_id != -1 && _renObjs[ren_id].fixed) {
        territories[y * (_width - 2) + x] = _renObjs[ren_id].color * color;
      }
      // Set confirmed territory for black
      else if (_areaIds[0][index] != -1 && _areaFlags[0][_areaIds[0][index]]) {
        territories[y * (_width - 2) + x] = BLACK * color;
      }
      // Set confirmed territory for white
      else if (_areaIds[1][index] != -1 && _areaFlags[1][_areaIds[1][index]]) {
        territories[y * (_width - 2) + x] = WHITE * color;
      }
      // Set unconfirmed territory
      else {
        territories[y * (_width - 2) + x] = EMPTY;
      }
    }
  }
}

/**
 * Get the owner data for each coordinate.
 * @param owners Owner data
 * @param color Reference stone color (set WHITE to return data judged for black and white)
 * @param rule Calculation rule (RULE_CH: Chinese rule, RULE_JP: Japanese rule, RULE_COM: Automatic match rule)
 */
void Board::getOwners(int32_t* owners, int32_t color, int32_t rule) {
  // Get data of fixed territories
  getTerritories(owners, color);

  // Set owner of stones in unfixed territories
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t owner_index = y * (_width - 2) + x;

      if (owners[owner_index] == EMPTY) {
        owners[owner_index] = getColor(x, y) * color;
      }
    }
  }

  // If Japanese rule, finish setting owner list
  if (rule == RULE_JP) {
    return;
  }

  // Create area data surrounded by a single color
  std::unique_ptr<int32_t[]> areas(new int32_t[_length]);
  std::unique_ptr<bool[]> checks(new bool[_length]);

  for (int32_t i = 0; i < _length; i++) {
    areas[i] = EMPTY;
    checks[i] = false;
  }

  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      int32_t color = getColor(x, y);

      // Do nothing if already checked
      // Do nothing if empty coordinate
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

  // Reflect area data in owner data
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
 * Get the pattern representation of the stones.
 * @return Pattern representation of the stones
 */
std::vector<int32_t> Board::getPatterns() {
  return _pattern.values();
}

/**
 * Get data to input to the model.
 * @param inputs Board data to input to the model
 * @param color Stone color to play
 * @param komi Komi value
 * @param rule Rule for determining winner
 * @param superko True to apply superko rule
 */
void Board::getInputs(float* inputs, int32_t color, float komi, int32_t rule, bool superko) {
  int length = MODEL_SIZE * MODEL_SIZE;
  int32_t offset_x = (MODEL_SIZE - _width + 2) / 2;
  int32_t offset_y = (MODEL_SIZE - _height + 2) / 2;

  // Update ladder (shicho) information
  _updateShicho();

  // Initialize input data
  for (int32_t i = 0; i < MODEL_INPUT_SIZE; i++) {
    inputs[i] = 0.0;
  }

  // Set stone arrangement
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t ren_id = _renIds[_getIndex(x, y)];
      int32_t index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      // Set mask
      inputs[length * MODEL_FEATURES + index] = 1.0;

      // Set value for empty coordinates
      if (ren_id == -1) {
        inputs[length * 0 + index] = 1.0;
        continue;
      }

      // Set value for coordinates with stones
      int32_t shicho = (_renObjs[ren_id].shicho) ? 1.0 : 0.0;
      int32_t size = std::min((int32_t)_renObjs[ren_id].spaces.size(), 8);

      // Set value for black stone coordinates
      if (_renObjs[ren_id].color * color == BLACK) {
        inputs[length * 1 + index] = 1.0;
        inputs[length * 2 + index] = shicho;
        inputs[length * (2 + size) + index] = 1.0;
      }
      // Set value for white stone coordinates
      else if (_renObjs[ren_id].color * color == WHITE) {
        inputs[length * 14 + index] = 1.0;
        inputs[length * 15 + index] = shicho;
        inputs[length * (15 + size) + index] = 1.0;
      }
    }
  }

  // Set move history
  std::vector<int32_t> black_histotires = _histories[(1 - color) / 2].get();
  std::vector<int32_t> white_histotires = _histories[(1 + color) / 2].get();

  std::reverse(black_histotires.begin(), black_histotires.end());
  std::reverse(white_histotires.begin(), white_histotires.end());

  for (int32_t i = 0; i < black_histotires.size(); i++) {
    if (black_histotires[i] > 0) {
      int32_t x = _getPosX(black_histotires[i]);
      int32_t y = _getPosY(black_histotires[i]);
      int32_t index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      inputs[length * (11 + i) + index] = 1.0;
    }
  }

  for (int32_t i = 0; i < white_histotires.size(); i++) {
    if (white_histotires[i] > 0) {
      int32_t x = _getPosX(white_histotires[i]);
      int32_t y = _getPosY(white_histotires[i]);
      int32_t index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      inputs[length * (24 + i) + index] = 1.0;
    }
  }

  // Set information for lines 1–4
  for (int i = 0; i < 4; i++) {
    int32_t begin_x = offset_x + i;
    int32_t end_x = offset_x + _width - 2 - i;
    int32_t begin_y = offset_y + i;
    int32_t end_y = offset_y + _height - 2 - i;

    for (int y = begin_y; y < end_y; y++) {
      inputs[length * (27 + i) + y * MODEL_SIZE + begin_x] = 1.0;
      inputs[length * (27 + i) + y * MODEL_SIZE + end_x - 1] = 1.0;
    }

    for (int x = begin_x; x < end_x; x++) {
      inputs[length * (27 + i) + begin_y * MODEL_SIZE + x] = 1.0;
      inputs[length * (27 + i) + (end_y - 1) * MODEL_SIZE + x] = 1.0;
    }
  }

  // Set ko information
  if (_koColor == color && _koIndex > 0) {
    int32_t x = _getPosX(_koIndex);
    int32_t y = _getPosY(_koIndex);
    int32_t index = y * MODEL_SIZE + x;

    inputs[length * 31 + index] = 1.0;
  }

  // Register turn
  int32_t info_offset = (MODEL_FEATURES + 1) * length;

  if (color == BLACK) {
    inputs[info_offset + 0] = 1.0;
  } else {
    inputs[info_offset + 1] = 1.0;
  }

  // Register komi value
  inputs[info_offset + 2] = (komi * color) / 13.0;

  // Register whether superko rule is applied
  if (superko) {
    inputs[info_offset + 3] = 1.0;
  }

  // Register whether ko has occurred
  if (_koColor == color && _koIndex > 0) {
    inputs[info_offset + 4] = 1.0;
  }

  // Register rule for determining winner
  if (rule != RULE_JP) {
    inputs[info_offset + 5] = 1.0;
  } else {
    inputs[info_offset + 6] = 1.0;
  }
}

/**
 * Get the board state.
 * @return Board state
 */
std::vector<int32_t> Board::getState() {
  std::vector<int32_t> state;

  // Register values representing stone arrangement
  for (int32_t v : _pattern.values()) {
    state.push_back(v);
  }

  // Register ko information
  state.push_back((_koIndex + 1) << 2 | (_koColor + 1));

  // Register move history
  std::vector<int32_t> black_histotires = _histories[0].get();
  std::vector<int32_t> white_histotires = _histories[1].get();

  state.push_back(
      (black_histotires[0] + 1) << 20 |
      (black_histotires[1] + 1) << 10 |
      (black_histotires[2] + 1));
  state.push_back(
      (white_histotires[0] + 1) << 20 |
      (white_histotires[1] + 1) << 10 |
      (white_histotires[2] + 1));

  return state;
}

/**
 * Restore the board state.
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
        play(x, y, BLACK);
      } else if (value == 2) {
        play(x, y, WHITE);
      }
    }
  }

  // Restore ko information
  int32_t ko_info = state[state.size() - 3];

  _koIndex = (ko_info >> 2 & 0x3FFFF) - 1;
  _koColor = (ko_info & 3) - 1;

  // Restore history
  _histories[0].clear();
  _histories[1].clear();

  for (int32_t i = 0; i < 3; i++) {
    int32_t black_history = (state[state.size() - 2] >> (20 - i * 10) & 0x3FF) - 1;
    int32_t white_history = (state[state.size() - 1] >> (20 - i * 10) & 0x3FF) - 1;

    if (black_history != -1) {
      _histories[0].add(black_history);
    }

    if (white_history != -1) {
      _histories[1].add(white_history);
    }
  }

  // Initialize flags
  _areaUpdated = false;
  _shichoUpdated = false;
}

/**
 * Copy the board state.
 * @param board Source board
 */
void Board::copyFrom(const Board* board) {
  // Copy group information
  memcpy(_renIds.get(), board->_renIds.get(), sizeof(int32_t) * _length);

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

  // Initialize flags
  _areaUpdated = false;
  _shichoUpdated = false;
}

/**
 * Output the board state.
 * @param os Output destination
 */
void Board::print(std::ostream& os) {
  os << "   ";
  for (int32_t x = 0; x < _width - 2; x++) {
    printf("%2d", x);
  }
  os << std::endl;

  os << "  +";
  for (int32_t x = 0; x < _width - 2; x++) {
    os << "--";
  }
  os << "-+" << std::endl;

  for (int32_t y = 0; y < _height - 2; y++) {
    printf("%2d|", y);
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      int32_t color = getColor(x, y);

      if (index == _koIndex) {
        os << " K";
      } else if (color == BLACK) {
        os << " X";
      } else if (color == WHITE) {
        os << " O";
      } else {
        os << " .";
      }
    }
    os << " |" << std::endl;
  }

  os << "  +";
  for (int32_t x = 0; x < _width - 2; x++) {
    os << "--";
  }
  os << "-+" << std::endl;
}

/**
 * Place a stone at the specified location.
 * Does not merge or remove groups.
 * @param index Position number
 * @param color Stone color
 */
void Board::_put(int32_t index, int32_t color) {
  int32_t op_color = OPPOSITE(color);

  // Change arrangement representation value
  _pattern.put(_getPosX(index), _getPosY(index), color);

  // Create group information
  _renIds[index] = index;
  _renObjs[index].color = color;
  _renObjs[index].positions.insert(index);

  // Register information to adjacent groups (do not merge)
  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    // Register as liberty if there are empty coordinates around
    if (ren_id == -1) {
      _renObjs[index].spaces.insert(index + a);
    }
    // Remove liberty if there are groups around
    else {
      _renObjs[ren_id].spaces.erase(index);
    }
  }
}

/**
 * Merge the specified groups.
 * @param srcIndex Source group position number
 * @param dstIndex Destination group position number
 */
void Board::_mergeRen(int32_t srcIndex, int32_t dstIndex) {
  int32_t src_id = _renIds[srcIndex];
  int32_t dst_id = _renIds[dstIndex];

  // Merge information
  _renObjs[dst_id].positions.insert(
      _renObjs[src_id].positions.begin(), _renObjs[src_id].positions.end());
  _renObjs[dst_id].spaces.insert(
      _renObjs[src_id].spaces.begin(), _renObjs[src_id].spaces.end());

  // Change identification number
  for (auto pos : _renObjs[src_id].positions) {
    _renIds[pos] = dst_id;
  }

  // Delete unused information
  _renObjs[src_id].color = EMPTY;
  _renObjs[src_id].positions.clear();
  _renObjs[src_id].spaces.clear();
}

/**
 * Remove the specified group.
 * @param index Position number
 */
void Board::_removeRen(int32_t index) {
  int32_t ren_id = _renIds[index];
  int32_t color = _renObjs[ren_id].color;

  // Execute removal process for all coordinates
  for (auto pos : _renObjs[ren_id].positions) {
    // Change identification number
    _renIds[pos] = -1;

    // Change value
    _pattern.remove(_getPosX(pos), _getPosY(pos), color);

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
 * Update empty area information.
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

    // Create list of group IDs
    std::set<int32_t> ren_ids;

    for (int32_t index = 0; index < _length; index++) {
      int32_t ren_id = _renIds[index];

      if (ren_id != -1 && _renObjs[ren_id].color == color) {
        ren_ids.insert(ren_id);
      }
    }

    // Initialize adjacent area information for groups
    // Initialize all groups to confirmed state
    for (int32_t ren_id : ren_ids) {
      _renObjs[ren_id].areas.clear();
      _renObjs[ren_id].fixed = true;
    }

    // Initialize check state for each coordinate
    std::unique_ptr<bool[]> checks(new bool[_length]);

    for (int32_t index = 0; index < _length; index++) {
      checks[index] = false;
    }

    // Create area information and register to group objects
    for (int32_t index = 0; index < _length; index++) {
      // Do nothing if already checked
      if (checks[index]) {
        continue;
      }

      // Do nothing if not empty area
      int32_t index_color = _getColor(index);

      if (index_color != EMPTY && index_color != op_color) {
        _areaIds[c][index] = -1;
        continue;
      }

      // Create list of connected group IDs
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
        // Get position number
        int32_t pos = stack.back();
        stack.pop_back();

        // Do nothing if already checked
        if (checks[pos]) {
          continue;
        }

        checks[pos] = true;

        // Set area ID
        _areaIds[c][pos] = index;

        // Get list of surrounding group IDs
        std::set<int32_t> around_ren_ids;

        for (auto a : AROUNDS) {
          int32_t target_id = _renIds[pos + a];

          if (target_id != -1 && _renObjs[target_id].color == color) {
            around_ren_ids.insert(target_id);
          }
        }

        // If there are no surrounding groups, mark as unconfirmed area
        if (around_ren_ids.empty()) {
          _areaFlags[c][pos] = false;
        }

        // If surrounding group IDs and connected group IDs differ, mark as unconfirmed area
        if (around_ren_ids != connected_ren_ids) {
          _areaFlags[c][index] = false;
        }

        // Add surrounding empty areas to stack
        for (auto a : AROUNDS) {
          int32_t around = pos + a;
          int32_t around_color = _getColor(around);

          if (around_color == EMPTY || around_color == op_color) {
            stack.push_back(around);
          }
        }
      }

      // Register area information to group objects
      if (_areaFlags[c][index]) {
        for (int32_t ren_id : connected_ren_ids) {
          _renObjs[ren_id].areas.insert(index);
        }
      }
    }

    // Set confirmation information for groups and areas
    bool updated = true;

    while (updated) {
      updated = false;

      // Update group information
      // Only confirm if connected to two or more confirmed areas
      // If connected confirmed areas are less than two,
      // mark connected areas as unconfirmed
      for (int32_t ren_id : ren_ids) {
        // Do nothing if group is unconfirmed
        if (!_renObjs[ren_id].fixed) {
          continue;
        }

        // Count connected confirmed areas
        int32_t fixed_count = 0;

        for (int32_t area_id : _renObjs[ren_id].areas) {
          if (_areaFlags[c][area_id]) {
            fixed_count += 1;
          }
        }

        // Do nothing if connected confirmed areas are two or more
        // (group remains confirmed)
        if (fixed_count >= 2) {
          continue;
        }

        // If connected confirmed areas are less than two,
        // mark connected areas as unconfirmed
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

  // Set update flag
  _areaUpdated = true;
}

/**
 * Update ladder (shicho) information.
 */
void Board::_updateShicho() {
  // Do nothing if already updated
  if (_shichoUpdated) {
    return;
  }

  // Update ladder (shicho) information
  for (int32_t index = 0; index < _length; index++) {
    int32_t ren_id = _renIds[index];

    // Do nothing if coordinate number and group number differ
    // One of the coordinate numbers belonging to the group is always the same as the group number
    if (ren_id != index) {
      continue;
    }

    // Determine ladder (shicho)
    _renObjs[ren_id].shicho = _isShichoRen(index);
  }

  // Set update flag
  _shichoUpdated = true;
}

/**
 * Return True if the specified group is a ladder (shicho).
 * @param index Position number
 * @return True if ladder
 */
bool Board::_isShichoRen(int32_t index) {
  // Not a ladder if number of liberties is not 1
  if (_renObjs[index].spaces.size() > 1) {
    return false;
  }

  // Use depth-first search to check all moves
  // If the escaping side has one candidate move and the following search is OK, ladder is confirmed
  // If the chasing side has two candidate moves and the following search is NG, check other branches
  std::vector<Board> stack({*this});

  while (!stack.empty()) {
    // Get board
    Board board = stack.back();
    stack.pop_back();

    // Get group ID
    int32_t ren_id = board._renIds[index];
    int32_t color = board._renObjs[ren_id].color;
    int32_t op_color = OPPOSITE(color);

    // If adjacent opponent's group has 1 liberty -> NG (opponent's stone can be captured)
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

    // Create board after placing stone
    // No candidate move to escape -> OK (ladder)
    Board curr_board(board);
    int32_t curr_pos = *board._renObjs[ren_id].spaces.begin();
    int32_t curr_pos_x = curr_board._getPosX(curr_pos);
    int32_t curr_pos_y = curr_board._getPosY(curr_pos);

    if (curr_board.play(curr_pos_x, curr_pos_y, color) < 0) {
      return true;
    }

    // If escaped board has 1 liberty -> OK (ladder)
    // If escaped board has 3 or more liberties -> NG (not ladder)
    int32_t curr_ren_id = curr_board._renIds[index];

    if (curr_board._renObjs[curr_ren_id].spaces.size() == 1) {
      return true;
    } else if (curr_board._renObjs[curr_ren_id].spaces.size() > 2) {
      continue;
    }

    // Create board by placing opponent's stone in liberty and add to search queue
    for (int32_t next_pos : curr_board._renObjs[curr_ren_id].spaces) {
      Board next_board(curr_board);
      int32_t next_pos_x = next_board._getPosX(next_pos);
      int32_t next_pos_y = next_board._getPosY(next_pos);

      next_board.play(next_pos_x, next_pos_y, op_color);
      stack.push_back(next_board);
    }
  }

  // Not a ladder
  return false;
}

/**
 * Get the color of the stone at the specified location.
 * @param index Position number
 * @return Stone color
 */
int32_t Board::_getColor(int32_t index) {
  int32_t ren_id = _renIds[index];

  if (ren_id == -1) {
    return EMPTY;
  } else {
    return _renObjs[ren_id].color;
  }
}

/**
 * Return true if a stone can be placed at the specified location.
 * @param index Position number
 * @param color Stone color
 * @param checkSeki True to check for seki
 * @return True if a stone can be placed
 */
bool Board::_isEnabled(int32_t index, int32_t color, bool checkSeki) {
  // If there is already a stone -> cannot place
  if (_renIds[index] != -1) {
    return false;
  }

  // If ko -> cannot place
  if (index == _koIndex && color == _koColor) {
    return false;
  }

  // If seki -> cannot place
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

    // Check groups around
    Ren ren = _renObjs[_renIds[target]];

    // If there is an allied stone with liberties around -> can place
    if (ren.color == color && ren.spaces.size() > 1) {
      return true;
    }

    // If there is a capturable enemy stone around -> can place
    if (ren.color == op_color && ren.spaces.size() == 1) {
      return true;
    }
  }

  // Cannot place
  return false;
}

/**
 * Return True if the specified location is subject to seki.
 * @param index Position number
 * @param color Stone color
 * @return True if subject to seki
 */
bool Board::_isSeki(int32_t index, int32_t color) {
  // Check adjacent opponent groups
  // If there is an opponent group with 1 liberty around the move coordinate -> NG (opponent's stone can be captured)
  int32_t op_color = OPPOSITE(color);

  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    if (ren_id != -1 &&
        _renObjs[ren_id].color == op_color &&
        _renObjs[ren_id].spaces.size() == 1) {
      return false;
    }
  }

  // Create list of adjacent groups
  std::set<int32_t> ren_ids;

  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    if (ren_id != -1 && _renObjs[ren_id].color == color) {
      ren_ids.insert(ren_id);
    }
  }

  // If there is no allied group around the move coordinate -> NG (not subject to seki)
  if (ren_ids.size() == 0) {
    return false;
  }

  // Check liberty coordinates (if 9 or more liberties (8 or more after move), not subject to judgment)
  std::set<int32_t> spaces;

  for (auto a : AROUNDS) {
    if (_renIds[index + a] == -1) {
      spaces.insert(index + a);
    }
  }

  for (auto id : ren_ids) {
    spaces.insert(_renObjs[id].spaces.begin(), _renObjs[id].spaces.end());

    if (spaces.size() >= 9) {
      return false;
    }
  }

  // Remove own coordinate from liberties
  spaces.erase(index);

  // If liberties are 0, not seki (cannot place stone)
  if (spaces.size() == 0) {
    return false;
  }
  // If there is 1 liberty
  else if (spaces.size() == 1) {
    return _isSekiRen(index, color, ren_ids, *spaces.begin());
  }
  // If there are 2 to 7 liberties
  else {
    return _isSekiArea(index, color, ren_ids, spaces);
  }
}

/**
 * Return True if the group created by placing a stone at the specified location is subject to seki.
 * @param index Position number
 * @param color Stone color
 * @param renIds List of group IDs to judge
 * @param spaceIndex Position number of empty area
 * @return True if subject to seki
 */
bool Board::_isSekiRen(
    int32_t index, int32_t color, std::set<int32_t>& renIds, int32_t spaceIndex) {
  // Create list of opponent groups adjacent to move coordinate and liberty
  // If there are empty coordinates around move coordinate and liberty -> NG (not subject to seki)
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

  // If there are no opponent groups around move coordinate and liberty -> NG (not subject to seki)
  if (op_ren_ids.size() == 0) {
    return false;
  }

  // If the number of liberties of opponent groups adjacent to move coordinate is not 2 -> NG (not subject to seki)
  // If the number of liberties of opponent groups adjacent to own liberty is not 2 -> NG (not subject to seki)
  for (auto ren_id : op_ren_ids) {
    if (_renObjs[ren_id].spaces.size() != 2) {
      return false;
    }
  }

  // Check group coordinates
  // If own group size is 7 or more -> OK (seki)
  std::set<int32_t> positions;

  positions.insert(index);

  for (auto ren_id : renIds) {
    positions.insert(
        _renObjs[ren_id].positions.begin(), _renObjs[ren_id].positions.end());

    if (positions.size() >= 7) {
      return true;
    }
  }

  // If own group shape is not nakade -> OK (seki)
  if (positions.size() >= 4 && !_isNakade(positions)) {
    return true;
  }

  // Create list of liberties of opponent groups adjacent to move point and liberty
  std::set<int32_t> op_spaces;

  for (auto ren_id : op_ren_ids) {
    op_spaces.insert(
        _renObjs[ren_id].spaces.begin(), _renObjs[ren_id].spaces.end());
  }

  // If opponent groups adjacent to move coordinate have liberties other than own liberty -> OK (seki)
  // If opponent groups adjacent to own liberty have liberties other than own liberty -> OK (seki)
  op_spaces.erase(index);
  op_spaces.erase(spaceIndex);

  if (!op_spaces.empty()) {
    return true;
  }

  // Otherwise -> NG (nakade)
  return false;
}

/**
 * Return True if the area created by placing a stone at the specified location is subject to seki.
 * @param index Position number
 * @param color Stone color
 * @param renIds List of group IDs to judge
 * @param spacesIndices List of empty area position numbers
 * @return True if subject to seki
 */
bool Board::_isSekiArea(
    int32_t index, int32_t color, std::set<int32_t>& renIds, std::set<int32_t>& spacesIndices) {
  // Create a list of area coordinates and adjacent groups on the board before the move
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

    // If size of adjacent area is 9 or more -> NG (not seki)
    if (positions.size() >= 9) {
      return false;
    }
  }

  // If adjacent area is connected to groups other than those connected to move coordinate -> NG (not seki)
  if (ren_ids != renIds) {
    return false;
  }

  // If group connected to move coordinate is connected to only one area
  // If any of the coordinate lists excluding any empty coordinate from area coordinates is nakade -> NG (not subject to seki)
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

  // Check adjacent area after move
  positions.erase(index);

  // If group connected to move coordinate is connected to multiple areas -> NG (not seki)
  if (!_isSingleArea(positions, color, index)) {
    return false;
  }

  // If any of the coordinate lists excluding any empty coordinate from area coordinates is nakade -> OK (seki)
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
 * Returns true if the specified list of position numbers forms a Nakade.
 * @param positions List of position numbers
 * @return True if it is a Nakade
 */
bool Board::_isNakade(std::set<int32_t>& positions) {
  const int32_t length = 5;
  const int32_t arounds[] = {1, -1, length, -length};
  const int32_t horizontals[] = {1, -1, 1, -1};
  const int32_t verticals[] = {length, length, -length, -length};

  // If the number of positions is 0, it is not a Nakade
  if (positions.size() == 0) {
    return false;
  }

  // If the size of the group is 7 or more -> NG (not a Nakade)
  if (positions.size() >= 7) {
    return false;
  }

  // Check the upper left and lower right of the positions
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

  // Check the distance between the upper left and lower right
  // If it is 3x3 or larger, there is no vital point -> not a Nakade
  if (end_x - start_x > 3 || end_y - start_y > 3) {
    return false;
  }

  // Create a board for calculation
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

  // Search for vital points
  for (int32_t y = 1; y < length - 1; y++) {
    for (int32_t x = 1; x < length - 1; x++) {
      int32_t p = y * length + x;

      // If it is not a target position for calculation
      if (board[p] != 1) {
        continue;
      }

      // Calculate the number of direct (vertical/horizontal) connections
      int32_t direct_connections = 0;

      for (auto a : arounds) {
        direct_connections += board[p + a];
      }

      // Calculate the number of diagonal connections
      int32_t skew_connections = 0;
      int32_t corner_connections = 0;

      for (int32_t i = 0; i < 4; i++) {
        int32_t v = verticals[i];
        int32_t h = horizontals[i];

        // Check the target
        if (board[p + v + h] != 1) {
          continue;
        }

        // If it is a connection to a corner
        if (corner_connections == 0 && corner[p + v] == 1 && board[p + v] == 1) {
          corner_connections = 1;
        } else if (corner_connections == 0 && corner[p + h] == 1 && board[p + h] == 1) {
          corner_connections = 1;
        }
        // If it is a diagonal connection
        else if (skew_connections == 0 && board[p + v] == 1 && board[p + h] == 1) {
          skew_connections = 1;
        }
      }

      // If the number of connections is greater than or equal to the specified value, judge as a vital point
      // If there is a position (vital point) that satisfies the following conditions -> OK (Nakade)
      // (1) Stones adjacent in vertical/horizontal directions
      // (2) Stones adjacent in diagonal directions (up to 1 place)
      // (3) Corner stones adjacent in diagonal directions (up to 1 place)
      if (direct_connections + skew_connections + corner_connections >= int(positions.size()) - 1) {
        return true;
      }
    }
  }

  return false;
}

/**
 * Returns true if the specified list of position numbers is contained within a single area.
 * @param positions List of position numbers
 * @param color Color of the stones surrounding the area
 * @param excludedIndex Position number to exclude
 * @return True if contained within a single area
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

/**
 * Returns whether the specified coordinates are a valid position.
 * @param x X coordinate
 * @param y Y coordinate
 * @return True if the position is valid
 */
inline bool Board::_isValidPosition(int32_t x, int32_t y) {
  return (x >= 0 && x < _width - 2 && y >= 0 && y < _height - 2);
}

/**
 * Gets the position number for the specified coordinates.
 * @param x X coordinate
 * @param y Y coordinate
 * @return Position number
 */
inline int32_t Board::_getIndex(int32_t x, int32_t y) {
  return ((y + 1) * _width) + (x + 1);
}

/**
 * Gets the X coordinate for the specified position number.
 * @param index Position number
 * @return X coordinate
 */
inline int32_t Board::_getPosX(int32_t index) {
  return (index % _width) - 1;
}

/**
 * Gets the Y coordinate for the specified position number.
 * @param index Position number
 * @return Y coordinate
 */
inline int32_t Board::_getPosY(int32_t index) {
  return (index / _width) - 1;
}

}  // namespace deepgo
