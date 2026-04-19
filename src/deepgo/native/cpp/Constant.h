#pragma once

#include <cstdint>

namespace deepgo {

// 盤面のハッシュ値を計算するための定数
// 手番、位置に対してランダムな64ビット整数値を格納している
extern const uint64_t BOARD_HASH_VALUES[4][361];

}  //  namespace deepgo
