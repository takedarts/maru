'''CPU regressions for input decoding, native inference, and threaded search.'''

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import numpy as np
import torch

from deepgo.board import Board
from deepgo.config import (
    COLOR_BLACK, COLOR_WHITE, MODEL_INPUT_PACK_SIZE, MODEL_INPUT_SIZE,
    MODEL_OUTPUT_SIZE, RULE_CH, RULE_JP,
)
from deepgo.native import NativeInferenceModel
from deepgo.processor import Processor
from tests.support import ProbeModel, save_search_model, unpack


class InferenceTest(unittest.TestCase):
    '''Verify the Python, Cython, and LibTorch interfaces without trained weights.'''

    def test_input_decoding(self) -> None:
        '''Compare every decoded feature and variable CPU batches; return None.
        Returns:
            None: No return value.
        '''
        boards = [Board(9, 9), Board(13, 13), Board(19, 19)]
        for board in boards:
            board.play((1, 1), COLOR_BLACK)
            board.play((2, 2), COLOR_WHITE)
        packed = np.stack([
            board.get_inputs(color, 6.5, rule, superko)
            for board in boards
            for color, rule, superko in [(COLOR_BLACK, RULE_CH, False),
                                         (COLOR_WHITE, RULE_JP, True)]
        ])
        self.assertEqual(packed.shape, (6, MODEL_INPUT_PACK_SIZE))
        self.assertEqual((MODEL_INPUT_SIZE, MODEL_OUTPUT_SIZE), (11936, 1810))
        expanded = unpack(packed)
        self.assertAlmostEqual(expanded[0, 11917], -1.0, places=4)
        self.assertAlmostEqual(expanded[0, 11918], 0.65, places=4)
        self.assertAlmostEqual(expanded[1, 11918], -0.65, places=4)

        # Shift the probe output to cover every feature, including the final scalar inputs.
        offsets = list(range(0, MODEL_INPUT_SIZE - MODEL_OUTPUT_SIZE, MODEL_OUTPUT_SIZE))
        offsets.append(MODEL_INPUT_SIZE - MODEL_OUTPUT_SIZE)
        with tempfile.TemporaryDirectory() as temporary:
            for offset in offsets:
                with self.subTest(offset=offset):
                    path = Path(temporary) / f'probe-{offset}.pt'
                    model = ProbeModel(offset).eval()
                    torch.jit.trace(model, torch.tensor(expanded[:1])).save(str(path))
                    expected = expanded[:, offset:offset + MODEL_OUTPUT_SIZE]
                    native = NativeInferenceModel(str(path), -1, False, True)
                    np.testing.assert_allclose(native.forward(packed), expected, atol=1e-6)
                    processor = Processor(
                        path, [-1], batch_size=4, deterministic=True, threads_per_gpu=1)
                    np.testing.assert_allclose(processor.execute(packed), expected, atol=1e-6)

    def test_search_and_gtp(self) -> None:
        '''Run native lifecycle and GTP checks with a process timeout; return None.
        Returns:
            None: No return value.
        '''
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'search.pt'
            save_search_model(path)
            # A native deadlock or crash must fail instead of blocking the test suite.
            result = subprocess.run(
                [sys.executable, str(Path(__file__).with_name('search_worker.py')), str(path)],
                text=True, capture_output=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run(
                [sys.executable, 'src/run.py', str(path), '--gpus=-1', '--threads', '1',
                 '--boardsize', '9', '--visits', '4', '--max-visits', '8',
                 '--initial-turn', '0', '--timelimit', '2'],
                input='1 protocol_version\n2 name\n3 version\n4 play b D4\n'
                      '5 genmove w\n6 undo\n7 clear_board\n8 quit\n',
                text=True, capture_output=True, timeout=45)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertNotIn('?', result.stdout)
            for response in ('=1 2', '=2 Maru', '=3 8.3', '=4', '=5', '=6', '=7', '=8'):
                self.assertIn(response, result.stdout)


class CommandLineTest(unittest.TestCase):
    '''Check public search options and rejected values independently of a real model.'''

    def test_search_options(self) -> None:
        '''Propagate search limits, PUCB settings, and cache defaults; return None.
        Returns:
            None: No return value.
        '''
        from unittest.mock import patch
        from run import parse_args

        with patch.object(sys, 'argv', [
            'run.py', 'model.pt', '--gpus=-1', '--visits', '100', '--max-visits', '20',
            '--pucb-min-visits-rate', '0.25',
        ]):
            args = parse_args()
        self.assertEqual(args.visits, 100)
        self.assertEqual(args.max_visits, 20)
        self.assertEqual(args.cache_size, 100)
        self.assertEqual(args.pucb_min_visits_rate, 0.25)
        self.assertEqual(args.gpus, [-1])

    def test_invalid_search_options(self) -> None:
        '''Reject removed or invalid search arguments; return None.
        Returns:
            None: No return value.
        '''
        import io
        from unittest.mock import patch
        from run import parse_args

        for option, value in [('--playouts', '1'), ('--visits', '-1'),
                              ('--max-visits', '0'), ('--pucb-min-visits-rate', '-0.1')]:
            with self.subTest(option=option), \
                    patch.object(sys, 'argv', ['run.py', 'model.pt', option, value]), \
                    patch.object(sys, 'stderr', io.StringIO()), self.assertRaises(SystemExit):
                parse_args()
