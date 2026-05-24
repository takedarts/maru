#include "BoardRen.h"

#include "Config.h"

namespace deepgo {

/**
 * Creates a group object.
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
