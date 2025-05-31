#include "Ren.h"

#include "Config.h"

namespace deepgo {

/**
 * 連のオブジェクトを生成する。
 */
Ren::Ren()
    : color(EMPTY),
      positions(),
      spaces(),
      areas(),
      shicho(false),
      fixed(false) {
}

/**
 * コピーした連のオブジェクトを生成する。
 */
Ren::Ren(const Ren& ren)
    : color(ren.color),
      positions(ren.positions),
      spaces(ren.spaces),
      areas(ren.areas),
      shicho(ren.shicho),
      fixed(ren.fixed) {
}

}  // namespace deepgo
