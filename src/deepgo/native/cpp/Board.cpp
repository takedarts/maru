#include "Board.h"

#include <algorithm>
#include <cstring>

#include "Config.h"

namespace deepgo {

#define AROUNDS {-1, -_width, 1, _width}

/**
 * 盤面オブジェクトを作成する。
 * @param width 盤面の幅
 * @param height 盤面の高さ
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
  // データを格納する配列を作成する
  _areaIds[0].reset(new int32_t[_length]);
  _areaIds[1].reset(new int32_t[_length]);
  _areaFlags[0].reset(new bool[_length]);
  _areaFlags[1].reset(new bool[_length]);

  // 連のデータを初期化する
  for (int32_t i = 0; i < _length; i++) {
    _renIds[i] = -1;
  }

  // 盤面の外側に境界データを設定する
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
 * コピーした盤面オブジェクトを作成する。
 * @param board コピー元の盤面オブジェクト
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
  // データを格納する配列を作成する
  _areaIds[0].reset(new int32_t[_length]);
  _areaIds[1].reset(new int32_t[_length]);
  _areaFlags[0].reset(new bool[_length]);
  _areaFlags[1].reset(new bool[_length]);

  // 盤面を複製する
  copyFrom(&board);
}

/**
 * 盤面の状態を初期化する。
 */
void Board::clear() {
  // 連の情報を初期化する
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      _renIds[index] = -1;
      _renObjs[index].color = EMPTY;
      _renObjs[index].positions.clear();
      _renObjs[index].spaces.clear();
    }
  }

  // フラグを設定する
  _areaUpdated = false;
  _shichoUpdated = false;

  // コウを初期化する
  _koIndex = -1;
  _koColor = EMPTY;

  // 履歴を初期化する
  _histories[0].clear();
  _histories[1].clear();

  // 石の並びの情報を初期化する
  _pattern.clear();
}

/**
 * 盤面の幅を取得する。
 * @return 盤面の幅
 */
int32_t Board::getWidth() {
  return _width - 2;
}

/**
 * 盤面の高さを取得する。
 * @return 盤面の高さ
 */
int32_t Board::getHeight() {
  return _height - 2;
}

/**
 * 石を置く。
 * @param x 石を置くX座標
 * @param y 石を置くY座標
 * @param color 石の色
 * @return 取り上げた石の数（おけない場合は-1）
 */
int32_t Board::play(int32_t x, int32_t y, int32_t color) {
  // パスの場合はコウの情報をリセットする
  if (!_isValidPosition(x, y)) {
    _koIndex = -1;
    _koColor = EMPTY;
    return 0;
  }

  // 確認
  int32_t index = _getIndex(x, y);
  int32_t op_color = OPPOSITE(color);

  if (!_isEnabled(index, color, false)) {
    return -1;
  }

  // 石を置く
  _put(index, color);

  // 履歴に着手座標を追加する
  if (color == BLACK) {
    _histories[0].add(index);
  } else if (color == WHITE) {
    _histories[1].add(index);
  }

  // 着手座標の周囲の状態を更新する
  int32_t remove_size = 0;

  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    // 空き座標の場合は何もしない
    if (ren_id == -1) {
      continue;
    }
    // 自分の連がある場合は連結する
    else if (_renObjs[ren_id].color == color && ren_id != _renIds[index]) {
      _mergeRen(index, index + a);
    }
    // 相手の連があってダメが無くなっている場合は削除する
    else if (_renObjs[ren_id].color == op_color && _renObjs[ren_id].spaces.empty()) {
      remove_size += _renObjs[ren_id].positions.size();
      _removeRen(index + a);
      _koIndex = index + a;
    }
  }

  // 2個以上削除 or 置いた石の連が2個以上 or 置いた石のダメが2個以上の場合はコウ判定を消去
  int32_t position_size = _renObjs[_renIds[index]].positions.size();
  int32_t space_size = _renObjs[_renIds[index]].spaces.size();

  if (remove_size != 1 || position_size > 1 || space_size > 1) {
    _koIndex = -1;
    _koColor = EMPTY;
  } else {
    _koColor = op_color;
  }

  // 領域情報とシチョウ情報のフラグをリセットする
  _areaUpdated = false;
  _shichoUpdated = false;

  return remove_size;
}

/**
 * コウの座標を取得する。
 * コウが発生していないなら(-1, -1)を返す。
 * @param color 対象の石の色
 * @return コウの座標
 */
std::pair<int32_t, int32_t> Board::getKo(int32_t color) {
  if (_koIndex != -1 && color == _koColor) {
    return std::make_pair(_getPosX(_koIndex), _getPosY(_koIndex));
  } else {
    return std::make_pair(-1, -1);
  }
}

/**
 * 最も最近の着手座標の一覧を返す。
 * @param color 石の色
 * @return 着手座標の一覧
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
 * 指定した座標の石の色を取得する。
 * @param x X座標
 * @param y Y座標
 * @return 石の色
 */
int32_t Board::getColor(int32_t x, int32_t y) {
  return _getColor(_getIndex(x, y));
}

/**
 * 石の色の一覧を返す。
 * @param colors 石の色のデータ
 * @param color 石の色
 */
void Board::getColors(int32_t* colors, int32_t color) {
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      colors[y * (_width - 2) + x] = getColor(x, y) * color;
    }
  }
}

/**
 * 指定した座標の連の大きさを取得する。
 * @param x X座標
 * @param y Y座標
 * @return 連の大きさ
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
 * 指定した座標の連のダメの数を取得する。
 * @param x X座標
 * @param y Y座標
 * @return ダメの数
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
 * 指定した座標の連のシチョウの有無を取得する。
 * @param x X座標
 * @param y Y座標
 * @return シチョウであればtrue
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
 * 石を置けるならtrueを返す。
 * @param x X座標
 * @param y Y座標
 * @param color 石の色
 * @param checkSeki セキを判定するならtrue
 * @return 石を置けるならtrue
 */
bool Board::isEnabled(int32_t x, int32_t y, int32_t color, bool checkSeki) {
  return _isEnabled(_getIndex(x, y), color, checkSeki);
}

/**
 * 石を置ける場所の一覧を取得する。
 * @param enableds 石を置ける場所の一覧
 * @param color 石の色
 * @param checkSeki セキを判定するならtrue
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
 * 確定地のデータを返す。
 * @param territories 確定地のデータ
 * @param color 基準となる石の色（WHITEを設定すると黒白を判定したデータを返す）
 */
void Board::getTerritories(int32_t* territories, int32_t color) {
  // 空き領域データを更新する
  _updateArea();

  // 確定地のデータを設定する
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t index = _getIndex(x, y);
      int32_t ren_id = _renIds[index];

      // 確定している連を設定する
      if (ren_id != -1 && _renObjs[ren_id].fixed) {
        territories[y * (_width - 2) + x] = _renObjs[ren_id].color * color;
      }
      // 黒の確定地を設定する
      else if (_areaIds[0][index] != -1 && _areaFlags[0][_areaIds[0][index]]) {
        territories[y * (_width - 2) + x] = BLACK * color;
      }
      // 白の確定地を設定する
      else if (_areaIds[1][index] != -1 && _areaFlags[1][_areaIds[1][index]]) {
        territories[y * (_width - 2) + x] = WHITE * color;
      }
      // 未確定地を設定する
      else {
        territories[y * (_width - 2) + x] = EMPTY;
      }
    }
  }
}

/**
 * それぞれの座標の所有者のデータを返す。
 * @param owners 所有者のデータ
 * @param color 基準となる石の色（WHITEを設定すると黒白を判定したデータを返す）
 * @param rule 計算ルール（RULE_CH:中国ルール, RULE_JP:日本ルール, RULE_COM:自動対戦ルール）
 */
void Board::getOwners(int32_t* owners, int32_t color, int32_t rule) {
  // 確定地のデータを取得する
  getTerritories(owners, color);

  // 未確定地にある石の所有者を設定する
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t owner_index = y * (_width - 2) + x;

      if (owners[owner_index] == EMPTY) {
        owners[owner_index] = getColor(x, y) * color;
      }
    }
  }

  // 日本ルールなら所有者の一覧の設定を終了する
  if (rule == RULE_JP) {
    return;
  }

  // 単色に囲まれている領域データを作成する
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

      // チェック済みなら何もしない
      // 空き座標なら何もしない
      if (checks[index] || color != EMPTY) {
        continue;
      }

      // 空き領域を探索する
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

      // 単色に囲まれている場合は領域データを設定する
      if (colors.size() == 1) {
        for (int32_t pos : positions) {
          areas[pos] = *colors.begin();
        }
      }
    }
  }

  // 領域データを所有者のデータに反映する
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
 * 石の並びを表現する値を取得する。
 * @return 石の並びを表現する値
 */
std::vector<int32_t> Board::getPatterns() {
  return _pattern.values();
}

/**
 * モデルに入力するデータを取得する。
 * @param inputs モデルに入力する盤面データ
 * @param color 着手する石の色
 * @param komi コミの目数
 * @param rule 勝敗の判定ルール
 * @param superko スーパーコウルールを適用するならtrue
 */
void Board::getInputs(float* inputs, int32_t color, float komi, int32_t rule, bool superko) {
  int length = MODEL_SIZE * MODEL_SIZE;
  int32_t offset_x = (MODEL_SIZE - _width + 2) / 2;
  int32_t offset_y = (MODEL_SIZE - _height + 2) / 2;

  // シチョウの情報を更新する
  _updateShicho();

  // 入力データを初期化する
  for (int32_t i = 0; i < MODEL_INPUT_SIZE; i++) {
    inputs[i] = 0.0;
  }

  // 石の並びを設定する
  for (int32_t y = 0; y < _height - 2; y++) {
    for (int32_t x = 0; x < _width - 2; x++) {
      int32_t ren_id = _renIds[_getIndex(x, y)];
      int32_t index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      // マスクを設定する
      inputs[length * MODEL_FEATURES + index] = 1.0;

      // 空き座標の場合の値を設定する。
      if (ren_id == -1) {
        inputs[length * 0 + index] = 1.0;
        continue;
      }

      // 石がおかれている座標の場合の値を設定する。
      int32_t shicho = (_renObjs[ren_id].shicho) ? 1.0 : 0.0;
      int32_t size = std::min((int32_t)_renObjs[ren_id].spaces.size(), 8);

      // 黒石座標の場合の値を設定する。
      if (_renObjs[ren_id].color * color == BLACK) {
        inputs[length * 1 + index] = 1.0;
        inputs[length * 2 + index] = shicho;
        inputs[length * (2 + size) + index] = 1.0;
      }
      // 白石座標の場合の値を設定する。
      else if (_renObjs[ren_id].color * color == WHITE) {
        inputs[length * 14 + index] = 1.0;
        inputs[length * 15 + index] = shicho;
        inputs[length * (15 + size) + index] = 1.0;
      }
    }
  }

  // 着手履歴を設定する
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

  // 1-4線の情報を設定する
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

  // コウの情報を設定する
  if (_koColor == color && _koIndex > 0) {
    int32_t x = _getPosX(_koIndex);
    int32_t y = _getPosY(_koIndex);
    int32_t index = y * MODEL_SIZE + x;

    inputs[length * 31 + index] = 1.0;
  }

  // 手番を登録する
  int32_t info_offset = (MODEL_FEATURES + 1) * length;

  if (color == BLACK) {
    inputs[info_offset + 0] = 1.0;
  } else {
    inputs[info_offset + 1] = 1.0;
  }

  // コミの目数を登録する
  inputs[info_offset + 2] = (komi * color) / 13.0;

  // スーパーコウルールの有無を登録する
  if (superko) {
    inputs[info_offset + 3] = 1.0;
  }

  // コウ発生の有無を登録する
  if (_koColor == color && _koIndex > 0) {
    inputs[info_offset + 4] = 1.0;
  }

  // 勝敗判定ルールを登録する
  if (rule != RULE_JP) {
    inputs[info_offset + 5] = 1.0;
  } else {
    inputs[info_offset + 6] = 1.0;
  }
}

/**
 * 盤面の状態を取得する。
 * @return 盤面の状態
 */
std::vector<int32_t> Board::getState() {
  std::vector<int32_t> state;

  // 石の並びを表現する値を登録
  for (int32_t v : _pattern.values()) {
    state.push_back(v);
  }

  // コウの情報を登録
  state.push_back((_koIndex + 1) << 2 | (_koColor + 1));

  // 着手履歴を登録
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
 * 盤面の状態を復元する。
 * @param state 盤面の状態
 */
void Board::loadState(std::vector<int32_t> state) {
  // 盤面を初期化する
  clear();

  // 石を置く
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

  // コウの情報を復元する
  int32_t ko_info = state[state.size() - 3];

  _koIndex = (ko_info >> 2 & 0x3FFFF) - 1;
  _koColor = (ko_info & 3) - 1;

  // 履歴を復元する
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

  // フラグを初期化する
  _areaUpdated = false;
  _shichoUpdated = false;
}

/**
 * 盤面の状態をコピーする。
 * @param board コピー元の盤面
 */
void Board::copyFrom(const Board* board) {
  // 連の情報をコピー
  memcpy(_renIds.get(), board->_renIds.get(), sizeof(int32_t) * _length);

  for (int32_t i = 0; i < _length; i++) {
    _renObjs[i] = board->_renObjs[i];
  }

  // コウの情報をコピー
  _koIndex = board->_koIndex;
  _koColor = board->_koColor;

  // 盤面の情報をコピー
  _pattern.copyFrom(board->_pattern);

  // 履歴をコピー
  _histories[0] = board->_histories[0];
  _histories[1] = board->_histories[1];

  // フラグを初期化する
  _areaUpdated = false;
  _shichoUpdated = false;
}

/**
 * 盤面の状態を出力する。
 * @param os 出力先
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
 * 指定した場所に石を置く。
 * 連の統合や削除は行わない。
 * @param index 位置番号
 * @param color 石の色
 */
void Board::_put(int32_t index, int32_t color) {
  int32_t op_color = OPPOSITE(color);

  // 並びの表現値を変更
  _pattern.put(_getPosX(index), _getPosY(index), color);

  // 連情報を作成
  _renIds[index] = index;
  _renObjs[index].color = color;
  _renObjs[index].positions.insert(index);

  // 近接する連に情報を登録(マージはしない)
  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    // 周りに空き座標がある場合はダメとして登録する
    if (ren_id == -1) {
      _renObjs[index].spaces.insert(index + a);
    }
    // 周りに連がある場合はダメを削除する
    else {
      _renObjs[ren_id].spaces.erase(index);
    }
  }
}

/**
 * 指定された連を統合する。
 * @param srcIndex 統合元の連の位置番号
 * @param dstIndex 統合先の連の位置番号
 */
void Board::_mergeRen(int32_t srcIndex, int32_t dstIndex) {
  int32_t src_id = _renIds[srcIndex];
  int32_t dst_id = _renIds[dstIndex];

  // 情報を統合する
  _renObjs[dst_id].positions.insert(
      _renObjs[src_id].positions.begin(), _renObjs[src_id].positions.end());
  _renObjs[dst_id].spaces.insert(
      _renObjs[src_id].spaces.begin(), _renObjs[src_id].spaces.end());

  // 識別番号を変更する
  for (auto pos : _renObjs[src_id].positions) {
    _renIds[pos] = dst_id;
  }

  // 使わない情報を削除する
  _renObjs[src_id].color = EMPTY;
  _renObjs[src_id].positions.clear();
  _renObjs[src_id].spaces.clear();
}

/**
 * 指定された連を削除する。
 * @param index 位置番号
 */
void Board::_removeRen(int32_t index) {
  int32_t ren_id = _renIds[index];
  int32_t color = _renObjs[ren_id].color;

  // すべての座標に対して削除処理を実行する
  for (auto pos : _renObjs[ren_id].positions) {
    // 識別番号を変更する
    _renIds[pos] = -1;

    // 値を変更する
    _pattern.remove(_getPosX(pos), _getPosY(pos), color);

    // 周りの連にダメを追加する
    for (auto a : AROUNDS) {
      int32_t target_id = _renIds[pos + a];

      if (target_id != -1) {
        _renObjs[target_id].spaces.insert(pos);
      }
    }
  }

  // 情報を削除
  _renObjs[ren_id].color = EMPTY;
  _renObjs[ren_id].positions.clear();
  _renObjs[ren_id].spaces.clear();
}

/**
 * 空き領域情報を更新する。
 */
void Board::_updateArea() {
  // 更新済みなら何もしない
  if (_areaUpdated) {
    return;
  }

  // 黒白両方の領域情報を作成する
  for (int32_t c = 0; c < 2; c++) {
    int32_t color = (c == 0) ? BLACK : WHITE;
    int32_t op_color = OPPOSITE(color);

    // 連のIDの一覧を作成する
    std::set<int32_t> ren_ids;

    for (int32_t index = 0; index < _length; index++) {
      int32_t ren_id = _renIds[index];

      if (ren_id != -1 && _renObjs[ren_id].color == color) {
        ren_ids.insert(ren_id);
      }
    }

    // 連の隣接領域情報を初期化する
    // すべての連を確定状態に初期化する
    for (int32_t ren_id : ren_ids) {
      _renObjs[ren_id].areas.clear();
      _renObjs[ren_id].fixed = true;
    }

    // 各座標の確認状態を初期化する
    std::unique_ptr<bool[]> checks(new bool[_length]);

    for (int32_t index = 0; index < _length; index++) {
      checks[index] = false;
    }

    // 領域情報を作成して連オブジェクトに登録する
    for (int32_t index = 0; index < _length; index++) {
      // チェック済みなら何もしない
      if (checks[index]) {
        continue;
      }

      // 空き領域でない場合は何もしない
      int32_t index_color = _getColor(index);

      if (index_color != EMPTY && index_color != op_color) {
        _areaIds[c][index] = -1;
        continue;
      }

      // 接続している連のID一覧を作成する
      std::set<int32_t> connected_ren_ids;

      for (auto a : AROUNDS) {
        if (_getColor(index + a) == color) {
          connected_ren_ids.insert(_renIds[index + a]);
        }
      }

      // 領域データを作成する
      std::vector<int32_t> stack;

      stack.push_back(index);
      _areaFlags[c][index] = true;

      while (!stack.empty()) {
        // 位置番号を取得する
        int32_t pos = stack.back();
        stack.pop_back();

        // チェック済みなら何もしない
        if (checks[pos]) {
          continue;
        }

        checks[pos] = true;

        // 領域IDを設定する
        _areaIds[c][pos] = index;

        // 周りの連のID一覧を取得する
        std::set<int32_t> around_ren_ids;

        for (auto a : AROUNDS) {
          int32_t target_id = _renIds[pos + a];

          if (target_id != -1 && _renObjs[target_id].color == color) {
            around_ren_ids.insert(target_id);
          }
        }

        // 周りに連がない場合は未確定領域とする
        if (around_ren_ids.empty()) {
          _areaFlags[c][pos] = false;
        }

        // 周りの連のID一覧と接続している連のID一覧が異なるなら未確定領域とする
        if (around_ren_ids != connected_ren_ids) {
          _areaFlags[c][index] = false;
        }

        // 周りの空き領域をスタックに追加する
        for (auto a : AROUNDS) {
          int32_t around = pos + a;
          int32_t around_color = _getColor(around);

          if (around_color == EMPTY || around_color == op_color) {
            stack.push_back(around);
          }
        }
      }

      // 連オブジェクトに領域情報を登録する
      if (_areaFlags[c][index]) {
        for (int32_t ren_id : connected_ren_ids) {
          _renObjs[ren_id].areas.insert(index);
        }
      }
    }

    // 連と領域の確定情報を設定する
    bool updated = true;

    while (updated) {
      updated = false;

      // 連の情報を更新する
      // 2個以上の確定領域と接続している場合のみ確定とする
      // 接続している確定領域が2個未満である場合は接続領域も未確定とする
      for (int32_t ren_id : ren_ids) {
        // 未確定の連の場合は何もしない
        if (!_renObjs[ren_id].fixed) {
          continue;
        }

        // 接続している確定領域を数える
        int32_t fixed_count = 0;

        for (int32_t area_id : _renObjs[ren_id].areas) {
          if (_areaFlags[c][area_id]) {
            fixed_count += 1;
          }
        }

        // 接続している確定領域が2個以上なら何もしない（連は確定状態のまな）
        if (fixed_count >= 2) {
          continue;
        }

        // 接続している確定領域が2個未満なら接続領域を未確定にする
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

  // 更新フラグを設定する
  _areaUpdated = true;
}

/**
 * シチョウの情報を更新する。
 */
void Board::_updateShicho() {
  // 更新済みなら何もしない
  if (_shichoUpdated) {
    return;
  }

  // シチョウの情報を更新する
  for (int32_t index = 0; index < _length; index++) {
    int32_t ren_id = _renIds[index];

    // 座標番号と連番号が異なる場合は何もしない
    // 連に属する座標番号の1つは必ず連番号と同じになる
    if (ren_id != index) {
      continue;
    }

    // シチョウを判定する
    _renObjs[ren_id].shicho = _isShichoRen(index);
  }

  // 更新フラグを設定する
  _shichoUpdated = true;
}

/**
 * 指定された連がシチョウであるならTrueを返す。
 * @param index 位置番号
 * @return シチョウであるならTrue
 */
bool Board::_isShichoRen(int32_t index) {
  // ダメの数が1個でないならシチョウでない
  if (_renObjs[index].spaces.size() > 1) {
    return false;
  }

  // 深さ優先探索ですべての着手を検証する
  // 逃げる側の候補手は1個なので以下の探索でOKならシチョウが確定する
  // 追いかける側の候補手は2個なので以下の探索でNGの場合は他の分岐を検証する
  std::vector<Board> stack({*this});

  while (!stack.empty()) {
    // 盤面を取得
    Board board = stack.back();
    stack.pop_back();

    // 連のIDを取得
    int32_t ren_id = board._renIds[index];
    int32_t color = board._renObjs[ren_id].color;
    int32_t op_color = OPPOSITE(color);

    // 隣接する相手の連のダメが1個 -> NG（相手の石が取れる）
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

    // 石を置いた盤面を作成
    // 逃げるための候補手がない -> OK（シチョウ）
    Board curr_board(board);
    int32_t curr_pos = *board._renObjs[ren_id].spaces.begin();
    int32_t curr_pos_x = curr_board._getPosX(curr_pos);
    int32_t curr_pos_y = curr_board._getPosY(curr_pos);

    if (curr_board.play(curr_pos_x, curr_pos_y, color) < 0) {
      return true;
    }

    // 逃げた盤面のダメが1個 -> OK（シチョウ）
    // 逃げた盤面でダメが3個以上 -> NG（シチョウではない）
    int32_t curr_ren_id = curr_board._renIds[index];

    if (curr_board._renObjs[curr_ren_id].spaces.size() == 1) {
      return true;
    } else if (curr_board._renObjs[curr_ren_id].spaces.size() > 2) {
      continue;
    }

    // ダメに相手の石を置いた盤面を作成して探索キューに追加
    for (int32_t next_pos : curr_board._renObjs[curr_ren_id].spaces) {
      Board next_board(curr_board);
      int32_t next_pos_x = next_board._getPosX(next_pos);
      int32_t next_pos_y = next_board._getPosY(next_pos);

      next_board.play(next_pos_x, next_pos_y, op_color);
      stack.push_back(next_board);
    }
  }

  // シチョウでない
  return false;
}

/**
 * 指定された場所の石の色を取得する。
 * @param index 位置番号
 * @return 石の色
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
 * 指定した場所に石を置くことができればtrueを返す。
 * @param index 位置番号
 * @param color 石の色
 * @param checkSeki セキを判定するならtrue
 * @return 石を置くことができればtrue
 */
bool Board::_isEnabled(int32_t index, int32_t color, bool checkSeki) {
  // 既に石がある場合 -> 置けない
  if (_renIds[index] != -1) {
    return false;
  }

  // コウの対象 -> 置けない
  if (index == _koIndex && color == _koColor) {
    return false;
  }

  // セキの対象 -> 置けない
  if (checkSeki && _isSeki(index, color)) {
    return false;
  }

  // まわりの判定
  int32_t op_color = OPPOSITE(color);

  for (auto a : AROUNDS) {
    int32_t target = index + a;

    // まわりに空間がある場合 -> 置ける
    if (_renIds[target] == -1) {
      return true;
    }

    // まわりにある連を確認する
    Ren ren = _renObjs[_renIds[target]];

    // まわりに余裕のある味方の石がある場合 -> 置ける
    if (ren.color == color && ren.spaces.size() > 1) {
      return true;
    }

    // まわりに取れる敵の石がある場合 -> 置ける
    if (ren.color == op_color && ren.spaces.size() == 1) {
      return true;
    }
  }

  // 置けない
  return false;
}

/**
 * 指定された場所がセキの対象となるならTrueを返す。
 * @param index 位置番号
 * @param color 石の色
 * @return セキの対象となるならTrue
 */
bool Board::_isSeki(int32_t index, int32_t color) {
  // 隣接する相手の連を確認
  // 着手座標の周りにダメが1個の相手の連がある -> NG（相手の石が取れる）
  int32_t op_color = OPPOSITE(color);

  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    if (ren_id != -1 &&
        _renObjs[ren_id].color == op_color &&
        _renObjs[ren_id].spaces.size() == 1) {
      return false;
    }
  }

  // 隣接する連の一覧を作成する
  std::set<int32_t> ren_ids;

  for (auto a : AROUNDS) {
    int32_t ren_id = _renIds[index + a];

    if (ren_id != -1 && _renObjs[ren_id].color == color) {
      ren_ids.insert(ren_id);
    }
  }

  // 着手座標の周りに自分の連が存在しない -> NG（セキ判定の対象外）
  if (ren_ids.size() == 0) {
    return false;
  }

  // ダメの座標を確認(ダメが9個以上(着手後のダメが8個以上)なら判定対象外)
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

  // 自身の座標を呼吸点から削除
  spaces.erase(index);

  // 呼吸点が0ならセキではない(石を置けない)
  if (spaces.size() == 0) {
    return false;
  }
  // 呼吸点が1個の場合
  else if (spaces.size() == 1) {
    return _isSekiRen(index, color, ren_ids, *spaces.begin());
  }
  // 呼吸点が2個以上7個以下の場合
  else {
    return _isSekiArea(index, color, ren_ids, spaces);
  }
}

/**
 * 指定された場所に石を置いたときに作成される連がセキの対象となるならTrueを返す。
 * @param index 位置番号
 * @param color 石の色
 * @param renIds 判定対象の連の識別番号一覧
 * @param spaceIndex 空き領域の位置番号
 * @return セキの対象となるならTrue
 */
bool Board::_isSekiRen(
    int32_t index, int32_t color, std::set<int32_t>& renIds, int32_t spaceIndex) {
  // 着手座標とダメに隣接する相手の連の一覧を作成
  // 着手座標とダメの周りに空き座標がある -> NG(セキ判定の対象外)
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

  // 着手座標とダメの周りに相手の連がない -> NG(セキ判定の対象外)
  if (op_ren_ids.size() == 0) {
    return false;
  }

  // 着手座標と隣接している相手の連のダメの数が2個ではない -> NG(セキ判定の対象外)
  // 自分のダメと隣接している相手の連のダメの数が2個ではない -> NG(セキ判定の対象外)
  for (auto ren_id : op_ren_ids) {
    if (_renObjs[ren_id].spaces.size() != 2) {
      return false;
    }
  }

  // 連の座標を確認
  // 自分の連の大きさが7個以上 -> OK（セキ）
  std::set<int32_t> positions;

  positions.insert(index);

  for (auto ren_id : renIds) {
    positions.insert(
        _renObjs[ren_id].positions.begin(), _renObjs[ren_id].positions.end());

    if (positions.size() >= 7) {
      return true;
    }
  }

  // 自分の連の形がナカデではない -> OK（セキ）
  if (positions.size() >= 4 && !_isNakade(positions)) {
    return true;
  }

  // 着手点と呼吸点に隣接する相手の連のダメの一覧を作成する
  std::set<int32_t> op_spaces;

  for (auto ren_id : op_ren_ids) {
    op_spaces.insert(
        _renObjs[ren_id].spaces.begin(), _renObjs[ren_id].spaces.end());
  }

  // 着手座標と隣接している相手の連に自分のダメ以外の場所にダメが存在する -> OK（セキ）
  // 自分のダメと隣接している相手の連に自分のダメ以外の場所にダメが存在する -> OK（セキ）
  op_spaces.erase(index);
  op_spaces.erase(spaceIndex);

  if (!op_spaces.empty()) {
    return true;
  }

  // それ以外 -> NG（ナカデ）
  return false;
}

/**
 * 指定された場所に石を置いたときに作成される領域がセキの対象となるならTrueを返す。
 * @param index 位置番号
 * @param color 石の色
 * @param renIds 判定対象の連の識別番号一覧
 * @param spacesIndices 空き領域の位置番号一覧
 * @return セキの対象となるならTrue
 */
bool Board::_isSekiArea(
    int32_t index, int32_t color, std::set<int32_t>& renIds, std::set<int32_t>& spacesIndices) {
  // 着手前の盤面で領域の座標一覧と隣接している連の一覧を作成する
  int32_t op_color = OPPOSITE(color);
  std::set<int32_t> positions;
  std::set<int32_t> ren_ids;
  std::vector<int32_t> stack;

  positions.insert(index);

  for (int32_t space_index : spacesIndices) {
    stack.push_back(space_index);
    positions.insert(space_index);
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

    // 隣接領域の大きさが9以上 -> NG（セキではない）
    if (positions.size() >= 9) {
      return false;
    }
  }

  // 隣接領域が着手座標と接続する連以外の連と接している -> NG（セキではない）
  if (ren_ids != renIds) {
    return false;
  }

  // 着手座標と接続する連が１つの領域とのみ接続しており
  // 領域座標から任意の空き座標を除外した座標一覧のいずれかがナカデ -> NG（セキ判定の対象外）
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

  // 着手後の隣接領域を確認する
  positions.erase(index);

  // 着手座標と接続する連が複数の領域と接続している -> NG（セキではない）
  if (!_isSingleArea(positions, color, index)) {
    return false;
  }

  // 領域座標から任意の空き座標を除外した座標一覧のいずれかがナカデ -> OK（セキ）
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

  // それ以外 -> NG（セキではない）
  return false;
}

/**
 * 指定された座標番号の一覧がナカデであるならTrueを返す。
 * @param positions 座標番号の一覧
 * @return ナカデであるならTrue
 */
bool Board::_isNakade(std::set<int32_t>& positions) {
  const int32_t length = 5;
  const int32_t arounds[] = {1, -1, length, -length};
  const int32_t horizontals[] = {1, -1, 1, -1};
  const int32_t verticals[] = {length, length, -length, -length};

  // 座標の数が0ならばナカデではない
  if (positions.size() == 0) {
    return false;
  }

  // 連の大きさが7以上 -> NG（ナカデではない）
  if (positions.size() >= 7) {
    return false;
  }

  // 座標の左上と右下を確認する
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

  // 左上と右下までの距離を確認する
  // 3x3以上なら急所は存在しない -> ナカデではない
  if (end_x - start_x > 3 || end_y - start_y > 3) {
    return false;
  }

  // 計算用の盤面を作成する
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

  // 急所を探す
  for (int32_t y = 1; y < length - 1; y++) {
    for (int32_t x = 1; x < length - 1; x++) {
      int32_t p = y * length + x;

      // 計算対象の座標ではない場合
      if (board[p] != 1) {
        continue;
      }

      // 縦横の接続数を計算
      int32_t direct_connections = 0;

      for (auto a : arounds) {
        direct_connections += board[p + a];
      }

      // 斜めの接続数を計算
      int32_t skew_connections = 0;
      int32_t corner_connections = 0;

      for (int32_t i = 0; i < 4; i++) {
        int32_t v = verticals[i];
        int32_t h = horizontals[i];

        // 対象を確認
        if (board[p + v + h] != 1) {
          continue;
        }

        // 隅に対する接続の場合
        if (corner_connections == 0 && corner[p + v] == 1 && board[p + v] == 1) {
          corner_connections = 1;
        } else if (corner_connections == 0 && corner[p + h] == 1 && board[p + h] == 1) {
          corner_connections = 1;
        }
        // 斜めの接続の場合
        else if (skew_connections == 0 && board[p + v] == 1 && board[p + h] == 1) {
          skew_connections = 1;
        }
      }

      // 接続数が指定された値以上の場合は急所と判定
      // 以下の条件を満たす座標（急所）がある -> OK（ナカデ）
      // (1) 縦横方向に隣接している石
      // (2) 斜め方向に隣接している石(1箇所まで)
      // (3) 斜め方向に隣接している隅の石(1箇所まで)
      if (direct_connections + skew_connections + corner_connections >= int(positions.size()) - 1) {
        return true;
      }
    }
  }

  return false;
}

/**
 * 指定された座標番号の一覧が単一領域に含まれているならTrueを返す。
 * @param positions 座標番号の一覧
 * @param color 領域を囲む石の色
 * @param excludedIndex 除外する位置番号
 * @return 単一領域に含まれているならTrue
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
 * 指定された座標が有効な位置かどうかを返す。
 * @param x X座標
 * @param y Y座標
 * @return 有効な位置ならtrue
 */
inline bool Board::_isValidPosition(int32_t x, int32_t y) {
  return (x >= 0 && x < _width - 2 && y >= 0 && y < _height - 2);
}

/**
 * 指定された座標の位置番号を取得する。
 * @param x X座標
 * @param y Y座標
 * @return 位置番号
 */
inline int32_t Board::_getIndex(int32_t x, int32_t y) {
  return ((y + 1) * _width) + (x + 1);
}

/**
 * 指定された位置番号のX座標を取得する。
 * @param index 位置番号
 * @return X座標
 */
inline int32_t Board::_getPosX(int32_t index) {
  return (index % _width) - 1;
}

/**
 * 指定された位置番号のY座標を取得する。
 * @param index 位置番号
 * @return Y座標
 */
inline int32_t Board::_getPosY(int32_t index) {
  return (index / _width) - 1;
}

}  // namespace deepgo
