#include "Ren.h"

#include "Config.h"

namespace deepgo {

/**
 * Creates a group object.
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
 * Creates a copied group object.
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
