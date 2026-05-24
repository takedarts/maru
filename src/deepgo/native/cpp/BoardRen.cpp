#include "BoardRen.h"

#include "Config.h"

namespace deepgo {

/**
 * 連のオブジェクトを生成する。
 */
BoardRen::BoardRen()
    : color(EMPTY),
      positions(),
      spaces(),
      areas(),
      shicho(false),
      fixed(false) {
}

}  // namespace deepgo
