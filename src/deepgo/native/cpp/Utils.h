#pragma once

#include "Config.h"

#define WIDTH (SIZE + 2)
#define LENGTH (WIDTH * WIDTH)

#define ENEMY(c) (-c)

#define POS(x, y) (((y + 1) * WIDTH) + (x + 1))
#define POS_X(p) ((p % WIDTH) - 1)
#define POS_Y(p) ((p / WIDTH) - 1)

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

const int32_t AROUNDS[] = {-1, -WIDTH, 1, WIDTH};

#define VALUE_LENGTH (((WIDTH * SIZE - 1) / 16) + 1)
#define VALUE_INDEX(p) (int((p - WIDTH) / 16))
#define VALUE_SHIFT(p, c) (((p - WIDTH) % 16) * 2 + ((c == BLACK) ? 0 : 1))
