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
 * 盤面の情報を保持するクラス。
 */
class Board {
 public:
  /**
   * 盤面オブジェクトを作成する。
   * @param width 盤面の幅
   * @param height 盤面の高さ
   */
  Board(int width, int height);

  /**
   * コピーした盤面オブジェクトを作成する。
   * @param board コピー元の盤面オブジェクト
   */
  Board(const Board& board);

  /**
   * 盤面オブジェクトを破棄する。
   */
  virtual ~Board() = default;

  /**
   * 盤面の状態を初期化する。
   */
  void clear();

  /**
   * 盤面の幅を取得する。
   * @return 盤面の幅
   */
  int32_t getWidth();

  /**
   * 盤面の高さを取得する。
   * @return 盤面の高さ
   */
  int32_t getHeight();

  /**
   * 石を置く。
   * @param x 石を置くX座標
   * @param y 石を置くY座標
   * @param color 石の色
   * @return 取り上げた石の数（おけない場合は-1）
   */
  int32_t play(int32_t x, int32_t y, int32_t color);

  /**
   * コウの座標を取得する。
   * コウが発生していないなら(-1, -1)を返す。
   * @param color 対象の石の色
   * @return コウの座標
   */
  std::pair<int32_t, int32_t> getKo(int32_t color);

  /**
   * 最も最近の着手座標の一覧を返す。
   * @param color 石の色
   * @return 着手座標の一覧
   */
  std::vector<std::pair<int32_t, int32_t>> getHistories(int color);

  /**
   * 指定した座標の石の色を取得する。
   * @param x X座標
   * @param y Y座標
   * @return 石の色
   */
  int32_t getColor(int32_t x, int32_t y);

  /**
   * 石の色の一覧を返す。
   * @param colors 石の色のデータ
   * @param color 石の色
   */
  void getColors(int32_t* colors, int32_t color);

  /**
   * 指定した座標の連の大きさを取得する。
   * @param x X座標
   * @param y Y座標
   * @return 連の大きさ
   */
  int32_t getRenSize(int32_t x, int32_t y);

  /**
   * 指定した座標の連のダメの数を取得する。
   * @param x X座標
   * @param y Y座標
   * @return ダメの数
   */
  int32_t getRenSpace(int32_t x, int32_t y);

  /**
   * 指定した座標の連のシチョウの有無を取得する。
   * @param x X座標
   * @param y Y座標
   * @return シチョウであればtrue
   */
  bool isShicho(int32_t x, int32_t y);

  /**
   * 石を置けるならtrueを返す。
   * @param x X座標
   * @param y Y座標
   * @param color 石の色
   * @param checkSeki セキを判定するならtrue
     * @return 石を置けるならtrue
   */
  bool isEnabled(int32_t x, int32_t y, int32_t color, bool checkSeki);

  /**
   * 石を置ける場所の一覧を取得する。
   * @param enableds 石を置ける場所の一覧
   * @param color 石の色
   * @param checkSeki セキを判定するならtrue
   */
  void getEnableds(int32_t* enableds, int32_t color, bool checkSeki);

  /**
   * 確定地のデータを返す。
   * @param territories 確定地のデータ
   * @param color 基準となる石の色（WHITEを設定すると黒白を判定したデータを返す）
   */
  void getTerritories(int32_t* territories, int32_t color);

  /**
   * それぞれの座標の所有者のデータを返す。
   * @param owners 所有者のデータ
   * @param color 基準となる石の色（WHITEを設定すると黒白を判定したデータを返す）
   * @param rule 計算ルール（RULE_CH:中国ルール, RULE_JP:日本ルール, RULE_COM:自動対戦ルール）
   */
  void getOwners(int32_t* owners, int32_t color, int32_t rule);

  /**
   * 石の並びを表現する値を取得する。
   * @return 石の並びを表現する値
   */
  std::vector<int32_t> getPatterns();

  /**
   * モデルに入力するデータを取得する。
   * @param inputs モデルに入力する盤面データ
   * @param color 着手する石の色
   * @param komi コミの目数
   * @param rule 勝敗の判定ルール
   * @param superko スーパーコウルールを適用するならtrue
   */
  void getInputs(float* inputs, int32_t color, float komi, int32_t rule, bool superko);

  /**
   * 盤面の状態を取得する。
   * @return 盤面の状態
   */
  std::vector<int32_t> getState();

  /**
   * 盤面の状態を復元する。
   * @param state 盤面の状態
   */
  void loadState(std::vector<int32_t> state);

  /**
   * 盤面の状態をコピーする。
   * @param board コピー元の盤面
   */
  void copyFrom(const Board* board);

  /**
   * 盤面の状態を出力する。
   * @param os 出力先
   */
  void print(std::ostream& os = std::cout);

 private:
  /**
   * 盤面の幅+2の値。
   */
  int32_t _width;

  /**
   * 盤面の高さ+2の値。
   */
  int32_t _height;

  /**
   * 盤面データの長さ。
   * (幅+2) * (高さ+2)の値となる。
   */
  int32_t _length;

  /**
   * 連情報の識別番号一覧。
   */
  std::unique_ptr<int32_t[]> _renIds;

  /**
   * 連情報の一覧。
   */
  std::unique_ptr<Ren[]> _renObjs;

  /**
   * 空き領域情報の識別番号の一覧。
   */
  std::unique_ptr<int32_t[]> _areaIds[2];

  /**
   * 空き領域情報の一覧。
   */
  std::unique_ptr<bool[]> _areaFlags[2];

  /**
   * コウが発生している場所。
   */
  int32_t _koIndex;

  /**
   * コウの対象となる色。
   */
  int32_t _koColor;

  /**
   * 着手座標の履歴。
   */
  History _histories[2];

  /**
   * 盤面の石の並びを表現するオブジェクト。
   */
  Pattern _pattern;

  /**
   * 領域情報が更新されていればtrue。
   */
  bool _areaUpdated;

  /**
   * シチョウ情報が更新されていればtrue。
   */
  bool _shichoUpdated;

  /**
   * 指定した場所に石を置く。
   * 連の統合や削除は行わない。
   * @param index 位置番号
   * @param color 石の色
   */
  void _put(int32_t index, int32_t color);

  /**
   * 指定された連を統合する。
   * @param srcIndex 統合元の連の位置番号
   * @param dstIndex 統合先の連の位置番号
   */
  void _mergeRen(int32_t srcIndex, int32_t dstIndex);

  /**
   * 指定された連を削除する。
   * @param index 位置番号
   */
  void _removeRen(int32_t index);

  /**
   * 空き領域情報を更新する。
   */
  void _updateArea();

  /**
   * シチョウの情報を更新する。
   */
  void _updateShicho();

  /**
   * 指定された連がシチョウであるならTrueを返す。
   * @param index 位置番号
   * @return シチョウであるならTrue
   */
  bool _isShichoRen(int32_t index);

  /**
   * 指定された場所の石の色を取得する。
   * @param index 位置番号
   * @return 石の色
   */
  int32_t _getColor(int32_t index);

  /**
   * 指定した場所に石を置くことができればtrueを返す。
   * @param index 位置番号
   * @param color 石の色
   * @param checkSeki セキを判定するならtrue
   * @return 石を置くことができればtrue
   */
  bool _isEnabled(int32_t index, int32_t color, bool checkSeki);

  /**
   * 指定された場所がセキの対象となるならTrueを返す。
   * @param index 位置番号
   * @param color 石の色
   * @return セキの対象となるならTrue
   */
  bool _isSeki(int32_t index, int32_t color);

  /**
   * 指定された場所に石を置いたときに作成される連がセキの対象となるならTrueを返す。
   * @param index 位置番号
   * @param color 石の色
   * @param renIds 判定対象の連の識別番号一覧
   * @param spaceIndex 空き領域の位置番号
   * @return セキの対象となるならTrue
   */
  bool _isSekiRen(int32_t index, int32_t color, std::set<int32_t>& renIds, int32_t spaceIndex);

  /**
   * 指定された場所に石を置いたときに作成される領域がセキの対象となるならTrueを返す。
   * @param index 位置番号
   * @param color 石の色
   * @param renIds 判定対象の連の識別番号一覧
   * @param spacesIndices 空き領域の位置番号一覧
   * @return セキの対象となるならTrue
   */
  bool _isSekiArea(
      int32_t index, int32_t color, std::set<int32_t>& renIds, std::set<int32_t>& spacesIndices);

  /**
   * 指定された座標番号の一覧がナカデであるならTrueを返す。
   * @param positions 座標番号の一覧
   * @return ナカデであるならTrue
   */
  bool _isNakade(std::set<int32_t>& positions);

  /**
   * 指定された座標番号の一覧が単一領域に含まれているならTrueを返す。
   * @param positions 座標番号の一覧
   * @param color 領域を囲む石の色
   * @param excludedIndex 除外する位置番号
   * @return 単一領域に含まれているならTrue
   */
  bool _isSingleArea(std::set<int32_t>& positions, int32_t color, int32_t excludedIndex);

  /**
   * 指定された座標が有効な位置かどうかを返す。
   * @param x X座標
   * @param y Y座標
   * @return 有効な位置ならtrue
   */
  inline bool _isValidPosition(int32_t x, int32_t y);

  /**
   * 指定された座標の位置番号を取得する。
   * @param x X座標
   * @param y Y座標
   * @return 位置番号
   */
  inline int32_t _getIndex(int32_t x, int32_t y);

  /**
   * 指定された位置番号のX座標を取得する。
   * @param index 位置番号
   * @return X座標
   */
  inline int32_t _getPosX(int32_t index);

  /**
   * 指定された位置番号のY座標を取得する。
   * @param index 位置番号
   * @return Y座標
   */
  inline int32_t _getPosY(int32_t index);
};

}  // namespace deepgo
