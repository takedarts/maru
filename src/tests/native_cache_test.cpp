/** Standalone regressions for inference cache keys, LRU eviction, and synchronization. */
#include <array>
#include <cassert>
#include <thread>
#include <vector>

#include "Board.h"
#include "Config.h"
#include "InferenceCache.h"
#include "InferenceHash.h"
#include "InferenceResult.h"

using namespace deepgo;

/** Compare two InferenceHash references; return bool indicating key equivalence. */
static bool sameKey(const InferenceHash& left, const InferenceHash& right) {
  return !(left < right) && !(right < left);
}

/** Verify cache behavior without a model or GPU; return int exit status. */
int main() {
  Board board(5, 5);
  Board small(3, 3);
  InferenceHash a(&board, COLOR_BLACK, 7.5f, RULE_CH, false);
  InferenceHash b(&board, COLOR_WHITE, 7.5f, RULE_CH, false);
  InferenceHash c(&board, COLOR_BLACK, 6.5f, RULE_CH, false);

  // Every model input condition must distinguish otherwise identical positions.
  assert(!sameKey(a, b));
  assert(!sameKey(a, c));
  assert(!sameKey(a, InferenceHash(&board, COLOR_BLACK, 7.5f, RULE_JP, false)));
  assert(!sameKey(a, InferenceHash(&board, COLOR_BLACK, 7.5f, RULE_CH, true)));
  assert(!sameKey(a, InferenceHash(&small, COLOR_BLACK, 7.5f, RULE_CH, false)));
  auto move_result = board.play(Move(0, 0, COLOR_BLACK));
  assert(!sameKey(a, InferenceHash(&board, COLOR_BLACK, 7.5f, RULE_CH, false)));
  board.undo(move_result);
  assert(sameKey(a, InferenceHash(&board, COLOR_BLACK, 7.5f, RULE_CH, false)));

  // Accessing A must preserve it when adding C evicts B from a two-entry cache.
  std::array<float, MODEL_TERRITORY_SIZE> territories{};
  InferenceResult first(0.25f, 4.0f, {}, territories);
  InferenceResult second(0.75f, 8.0f, {}, territories);
  InferenceResult result;
  InferenceCache cache(2);
  cache.put(a, first);
  cache.put(b, second);
  assert(cache.get(a, result));
  assert(result.getScore() == 4.0f);
  cache.put(c, second);
  assert(!cache.get(b, result));
  assert(cache.get(a, result));
  assert(cache.get(c, result));
  cache.put(a, second);
  assert(cache.get(a, result));
  assert(result.getValue() == 0.25f);
  assert(cache.getHitRate() > 0.0f && cache.getHitRate() < 1.0f);

  // Zero and negative capacities disable storage.
  for (int capacity : {0, -1}) {
    InferenceCache disabled(capacity);
    disabled.put(a, first);
    assert(!disabled.get(a, result));
  }

  // Clearing ko changes the affected player's inference key, but not stone placement.
  for (auto pos : {std::pair{1, 1}, std::pair{3, 1}, std::pair{2, 0}}) {
    board.play(Move(pos.first, pos.second, COLOR_BLACK));
  }
  for (auto pos : {std::pair{2, 1}, std::pair{1, 2}, std::pair{3, 2}, std::pair{2, 3}}) {
    board.play(Move(pos.first, pos.second, COLOR_WHITE));
  }
  board.play(Move(2, 2, COLOR_BLACK));
  const auto pattern = board.getPattern();
  InferenceHash ko_white(&board, COLOR_WHITE, 7.5f, RULE_CH, false);
  InferenceHash ko_black(&board, COLOR_BLACK, 7.5f, RULE_CH, false);
  assert(board.getKo(COLOR_WHITE) == std::make_pair(2, 1));
  board.play(Move::createPassMove(COLOR_BLACK));
  assert(board.getPattern() == pattern);
  assert(!sameKey(ko_white, InferenceHash(&board, COLOR_WHITE, 7.5f, RULE_CH, false)));
  assert(sameKey(ko_black, InferenceHash(&board, COLOR_BLACK, 7.5f, RULE_CH, false)));

  // Concurrent accesses must preserve the value belonging to each key.
  InferenceCache shared(8);
  std::vector<std::thread> workers;
  for (int index = 0; index < 4; ++index) {
    workers.emplace_back([&shared, &board, &territories, index]() {
      InferenceHash key(&board, COLOR_BLACK, static_cast<float>(index), RULE_CH, false);
      InferenceResult value(0.5f, static_cast<float>(index), {}, territories);
      for (int iteration = 0; iteration < 100; ++iteration) {
        shared.put(key, value);
        InferenceResult found;
        assert(shared.get(key, found));
        assert(found.getScore() == index);
      }
    });
  }
  for (auto& worker : workers) {
    worker.join();
  }
  return 0;
}
