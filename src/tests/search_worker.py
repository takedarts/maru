'''Exercise native search in a child process so the parent can enforce a timeout.'''

import sys
import unittest
from pathlib import Path

import numpy as np
import torch

from deepgo.config import COLOR_BLACK, COLOR_WHITE, MOVE_PASS, RULE_CH, RULE_JP
from deepgo.player import Player
from deepgo.processor import Processor


def check_search(path: Path) -> None:
    '''Verify search behavior with model path (Path); return None.
    Returns:
        None: No return value.
    '''
    torch.set_num_threads(1)
    check = unittest.TestCase()
    processor = Processor(path, [-1], batch_size=4, threads_per_gpu=1,
                          cache_size=128, deterministic=True)

    # A pass probability is retained under Japanese rules and filtered under Chinese rules.
    for rule in (RULE_CH, RULE_JP):
        player = Player(processor, threads=1, width=3, height=3, rule=rule, komi=0.5)
        board = player.get_board()
        before = board.get_colors().copy()
        for _ in range(2):
            value = processor.native.predict(board.native, COLOR_BLACK, 0.5, rule, False)
            check.assertAlmostEqual(value, 0.5)
        check.assertGreater(processor.get_cache_hit_rate(), 0.0)
        candidate = player.get_pass_candidate()
        check.assertEqual(candidate.pos, MOVE_PASS)
        check.assertAlmostEqual(candidate.score, -0.5)
        np.testing.assert_array_equal(before, player.get_board().get_colors())
        candidates = player.evaluate(visits=3, width=1, timelimit=2)
        check.assertEqual(candidates[0].pos == MOVE_PASS, rule == RULE_JP)
        check.assertEqual(candidates[0].territories.shape, (3, 3, 3))
        check.assertFalse(hasattr(candidates[0], 'playouts'))

        # Promote consecutive pass nodes and verify search can resume from the new root.
        player.play(MOVE_PASS)
        player.evaluate(visits=4, timelimit=2)
        player.play(MOVE_PASS)
        check.assertTrue(player.evaluate(visits=4, timelimit=2))
        player.play((0, 0))
        check.assertEqual(player.get_board().get_color((0, 0)), COLOR_BLACK)
        player.initialize()
        check.assertEqual(player.get_color(), COLOR_BLACK)
        check.assertEqual(np.count_nonzero(player.get_board().get_colors()), 0)

    # Check score perspective and aggregation using a single search branch.
    player = Player(processor, width=3, height=3, komi=0.5)
    check.assertAlmostEqual(player.evaluate(2, width=1)[0].score, -6.0, places=4)
    check.assertAlmostEqual(player.evaluate(3, width=1)[0].score, 0.0, places=4)
    check.assertAlmostEqual(player.get_predicted_score(), 6.0, places=4)
    player.play((0, 0))
    check.assertAlmostEqual(player.get_predicted_score(), -6.0, places=4)

    # An unreachable target must still terminate at the configured maximum.
    player = Player(processor, width=3, height=3, max_visits=3)
    player.evaluate(100, timelimit=2)
    root_visits = int(str(player).split('Visits=')[1].split(',')[0])
    check.assertGreaterEqual(root_visits, 3)
    check.assertLess(root_visits, 100)

    # PUCB's minimum visit ratio distributes visits across the chosen children.
    player = Player(processor, width=3, height=3, pucb_min_visits_rate=1.0)
    candidates = player.evaluate(60, width=3, criterion='visits')
    visits = [candidate.visits for candidate in candidates]
    check.assertEqual(len(visits), 3)
    check.assertLessEqual(max(visits) - min(visits), 2)

    # Superko must still exclude a historical position after ko is cleared by passes.
    for superko in (False, True):
        player = Player(processor, width=5, height=5, rule=RULE_JP, superko=superko)
        for pos in ((1, 1), (3, 1), (2, 0)):
            player.play(pos, COLOR_BLACK)
        for pos in ((2, 1), (1, 2), (3, 2), (2, 3)):
            player.play(pos, COLOR_WHITE)
        player.play((2, 2), COLOR_BLACK)
        check.assertEqual(player.get_captured(COLOR_WHITE), 1)
        player.play(MOVE_PASS, COLOR_WHITE)
        player.play(MOVE_PASS, COLOR_BLACK)
        candidates = player.evaluate(100, equally=True, width=32, timelimit=2)
        check.assertEqual((2, 1) in [candidate.pos for candidate in candidates], not superko)

    # Pondering, reading the board, and resetting must not race with root replacement.
    player = Player(processor, threads=2, width=9, height=9, max_visits=32)
    for _ in range(3):
        candidates = player.evaluate(4, ponder=True, timelimit=2)
        player.get_predicted_territories()
        player.get_predicted_score()
        player.get_board()
        player.play(candidates[0].pos)
        player.stop_evaluation()
        player.initialize()
    check.assertEqual(player.get_captured(COLOR_BLACK), 0)
    check.assertEqual(player.get_captured(COLOR_WHITE), 0)


if __name__ == '__main__':
    check_search(Path(sys.argv[1]))
