#pragma once

#include <cstdint>

namespace deepgo {

// Constants for computing board hash values
// Stores random 64-bit integer values indexed by turn and position
extern const uint64_t BOARD_HASH_VALUES[4][361];

}  //  namespace deepgo
