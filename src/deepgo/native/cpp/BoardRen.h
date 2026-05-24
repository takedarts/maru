#pragma once

#include <cstdint>
#include <set>

namespace deepgo {

/**
 * Struct that holds group information.
 */
struct BoardRen {
  /**
   * Creates a group object.
   */
  BoardRen();

  /**
   * Creates a copy of the group object.
   */
  BoardRen(const BoardRen& ren) = default;

  /**
   * Destroys the group object.
   */
  virtual ~BoardRen() = default;

  /**
   * Stone color.
   */
  int32_t color;

  /**
   * List of stone positions.
   */
  std::set<int32_t> positions;

  /**
   * List of liberty positions.
   */
  std::set<int32_t> spaces;

  /**
   * List of adjacent empty areas.
   */
  std::set<int32_t> areas;

  /**
   * true if the group is in a ladder.
   */
  bool shicho;

  /**
   * true if the group is confirmed alive.
   * A group is confirmed alive if two or more adjacent areas become territory.
   */
  bool fixed;
};

}  // namespace deepgo
