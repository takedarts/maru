#pragma once

#include <cstdint>
#include <set>

namespace deepgo {

/**
 * Struct that holds group information.
 */
struct Ren {
  /**
   * Creates a group object.
   */
  Ren();

  /**
   * Creates a copied group object.
   */
  Ren(const Ren& ren);

  /**
   * Destroys the group object.
   */
  virtual ~Ren() = default;

  /**
   * Stone color.
   */
  int32_t color;

  /**
   * List of stone coordinates.
   */
  std::set<int32_t> positions;

  /**
   * List of liberty coordinates.
   */
  std::set<int32_t> spaces;

  /**
   * List of adjacent empty areas.
   */
  std::set<int32_t> areas;

  /**
   * True if in atari.
   */
  bool shicho;

  /**
   * True if life is confirmed.
   * Life is confirmed if two or more areas adjacent to this group become territory.
   */
  bool fixed;
};

}  // namespace deepgo
