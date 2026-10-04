import itertools
import math
import unittest
from typing import List, Set, Tuple

import numpy as np
from deepgo.board import Board, get_color_name, get_opposite_color
from deepgo.config import (COLOR_BLACK, COLOR_EMPTY, COLOR_WHITE, DEFAULT_SIZE,
                           MODEL_BOARD_SIZE, MODEL_FEATURE_NUM,
                           MODEL_INFO_OFFSET, MODEL_INFO_SIZE,
                           MODEL_MASK_OFFSET, MODEL_VALUE_SCALE, MOVE_PASS,
                           RULE_CH, RULE_COM, RULE_JP)
from deepgo.exception import GoException
from tests.support import unpack
from tests.records import read_record


def _get_areas(board: Board) -> np.ndarray:
    '''Create territory data for regions surrounded by a single color.
    Args:
        board (Board): Board object
    Returns:
        np.ndarray: Regions surrounded by a single color
    '''
    areas = np.zeros((board.get_height(), board.get_width()), dtype=int)
    checks = np.zeros((board.get_height(), board.get_width()), dtype=bool)

    for y, x in np.ndindex(board.get_height(), board.get_width()):
        if checks[y, x]:
            continue

        if board.get_color((x, y)) != COLOR_EMPTY:
            checks[y, x] = True
            continue

        positions: Set[Tuple[int, int]] = set()
        colors: Set[int] = set()
        stack: List[Tuple[int, int]] = [(x, y)]

        while len(stack) > 0:
            pos = stack.pop()

            if checks[pos[1], pos[0]]:
                continue

            positions.add(pos)
            checks[pos[1], pos[0]] = True

            for ax, ay in ((0, -1), (1, 0), (0, 1), (-1, 0)):
                nx, ny = pos[0] + ax, pos[1] + ay

                if not board.is_valid_position((nx, ny)):
                    continue

                color = board.get_color((nx, ny))

                if color == COLOR_EMPTY:
                    stack.append((nx, ny))
                else:
                    colors.add(color)

        if len(colors) == 1:
            color = colors.pop()

            for pos in positions:
                areas[pos[1], pos[0]] = color

    return areas


class BoardTest(unittest.TestCase):
    '''Regression tests for the Board class.'''

    def test_get_pattern(self) -> None:
        '''Test the values representing the stone arrangement.
        Returns:
            None: No return value.
        '''
        board = Board(DEFAULT_SIZE, DEFAULT_SIZE)
        bitboard_size = MODEL_BOARD_SIZE * MODEL_BOARD_SIZE // 64 + 2

        # All values must be zero on an empty board
        self.assertEqual((0,) * (bitboard_size * 4), board.get_pattern())

        # Place black stones across a 32-bit boundary and white stones across a 64-bit boundary
        board.play((0, 0), COLOR_BLACK)
        board.play((10, 0), COLOR_BLACK)
        board.play((9, 1), COLOR_WHITE)
        board.play((0, 2), COLOR_WHITE)

        expected = [0] * (bitboard_size * 4)
        expected[0] = 1 << 22
        expected[1] = 1
        expected[bitboard_size * 2 + 1] = 1 << 20
        expected[bitboard_size * 2 + 2] = 1
        self.assertEqual(tuple(expected), board.get_pattern())

        # Passing must not change the stone arrangement
        pattern = board.get_pattern()
        result = board.play(MOVE_PASS, COLOR_BLACK)
        self.assertEqual(pattern, board.get_pattern())

        # Undo must restore the original arrangement
        board.undo(result)
        self.assertEqual(pattern, board.get_pattern())

        # Changing ko information must not change the stone arrangement
        record, _, _, _, _, _, _, _ = read_record('011')
        board = record.create_board()
        pattern = board.get_pattern()
        board.play(MOVE_PASS, COLOR_BLACK)
        self.assertEqual(pattern, board.get_pattern())

    def test_play_result(self) -> None:
        '''Test move results.
        Returns:
            None: No return value.
        '''
        # Check a move without captures
        board = Board(3, 3)
        result = board.play((1, 1), COLOR_BLACK)
        self.assertEqual(
            ((1, 1), COLOR_BLACK, 0, (False, False, False, False), MOVE_PASS),
            result)

        # Check a capture in the upward direction
        board = Board(3, 3)
        board.play((1, 0), COLOR_WHITE)
        board.play((0, 0), COLOR_BLACK)
        board.play((2, 0), COLOR_BLACK)
        result = board.play((1, 1), COLOR_BLACK)
        self.assertEqual(
            ((1, 1), COLOR_BLACK, 1, (True, False, False, False), MOVE_PASS),
            result)

        # Check captures in all directions
        board = Board(5, 5)
        for pos in ((2, 0), (1, 1), (3, 1), (4, 2),
                    (3, 3), (2, 4), (1, 3), (0, 2)):
            board.play(pos, COLOR_BLACK)
        for pos in ((2, 1), (3, 2), (2, 3), (1, 2)):
            board.play(pos, COLOR_WHITE)
        result = board.play((2, 2), COLOR_BLACK)
        self.assertEqual(
            ((2, 2), COLOR_BLACK, 4, (True, True, True, True), MOVE_PASS),
            result)

        # Check the result of a pass
        result = board.play(MOVE_PASS, COLOR_WHITE)
        self.assertEqual(
            (MOVE_PASS, COLOR_WHITE, 0, (False, False, False, False), MOVE_PASS),
            result)

        # Return the previous ko coordinate regardless of which color is subject to ko
        record, _, _, _, _, _, _, _ = read_record('011')
        board = record.create_board()
        self.assertEqual(MOVE_PASS, board.get_ko(COLOR_BLACK))
        result = board.play(MOVE_PASS, COLOR_BLACK)
        self.assertEqual((6, 10), result[4])
        self.assertEqual(MOVE_PASS, board.get_ko(COLOR_WHITE))

    def test_play_error(self) -> None:
        '''Test exceptions for illegal moves.
        Returns:
            None: No return value.
        '''
        board = Board(3, 3)
        board.play((1, 1), COLOR_BLACK)

        # Wrap C++ exceptions in GoException
        with self.assertRaisesRegex(GoException, 'Move is not legal'):
            board.play((1, 1), COLOR_WHITE)

        with self.assertRaisesRegex(GoException, 'Move position is outside the board'):
            board.play((3, 0), COLOR_BLACK)

        with self.assertRaisesRegex(GoException, 'Move color is invalid'):
            board.play((0, 0), COLOR_EMPTY)

    def test_undo(self) -> None:
        '''Test undoing moves.
        Returns:
            None: No return value.
        '''
        # Undo a move without captures
        board = Board(5, 5)
        board.play((1, 2), COLOR_BLACK)
        board.play((3, 2), COLOR_BLACK)
        self._test_undo_move(board, (2, 2), COLOR_BLACK)

        # Undo captures of distinct groups in all directions
        board = Board(5, 5)
        for pos in ((2, 0), (1, 1), (3, 1), (4, 2),
                    (3, 3), (2, 4), (1, 3), (0, 2)):
            board.play(pos, COLOR_BLACK)
        for pos in ((2, 1), (3, 2), (2, 3), (1, 2)):
            board.play(pos, COLOR_WHITE)
        self._test_undo_move(board, (2, 2), COLOR_BLACK)

        # Undo capturing a group that touches the move from multiple directions
        board = Board(3, 3)
        for pos in ((0, 0), (1, 0), (0, 1)):
            board.play(pos, COLOR_WHITE)
        board.play((2, 0), COLOR_BLACK)
        board.play((0, 2), COLOR_BLACK)
        self._test_undo_move(board, (1, 1), COLOR_BLACK)

        # Restore ko cleared by a pass
        record, _, _, _, _, _, _, _ = read_record('011')
        board = record.create_board()
        self._test_undo_move(board, MOVE_PASS, COLOR_BLACK)

    def _test_undo_move(
        self,
        board: Board,
        pos: Tuple[int, int],
        color: int,
    ) -> None:
        '''Check that the public state is restored by undo.
        Args:
            board (Board): Board to check
            pos (Tuple[int, int]): Move coordinate
            color (int): Moving color
        Returns:
            None: No return value.
        '''
        # Save the public game state before the move
        colors = board.get_colors().copy()
        enableds = {
            target_color: board.get_enableds(target_color).copy()
            for target_color in (COLOR_BLACK, COLOR_WHITE)
        }
        territories = board.get_fixed_territories().copy()
        owners = board.get_owners(territories).copy()
        ko = {
            target_color: board.get_ko(target_color)
            for target_color in (COLOR_BLACK, COLOR_WHITE)
        }
        ren_sizes = np.array([
            board.get_ren_size((x, y))
            for y, x in np.ndindex(board.get_height(), board.get_width())
        ])
        ren_spaces = np.array([
            board.get_ren_space((x, y))
            for y, x in np.ndindex(board.get_height(), board.get_width())
        ])
        shichos = np.array([
            board.is_shicho((x, y))
            for y, x in np.ndindex(board.get_height(), board.get_width())
        ])
        inputs = {
            target_color: board.get_inputs(target_color, 7.5, RULE_COM, True).copy()
            for target_color in (COLOR_BLACK, COLOR_WHITE)
        }

        # Undo the move and compare against the saved state
        result = board.play(pos, color)
        board.undo(result)

        np.testing.assert_array_equal(colors, board.get_colors())
        np.testing.assert_array_equal(territories, board.get_fixed_territories())
        np.testing.assert_array_equal(
            owners, board.get_owners(board.get_fixed_territories()))

        for target_color in (COLOR_BLACK, COLOR_WHITE):
            np.testing.assert_array_equal(
                enableds[target_color], board.get_enableds(target_color))
            self.assertEqual(ko[target_color], board.get_ko(target_color))

        actual_ren_sizes = np.array([
            board.get_ren_size((x, y))
            for y, x in np.ndindex(board.get_height(), board.get_width())
        ])
        actual_ren_spaces = np.array([
            board.get_ren_space((x, y))
            for y, x in np.ndindex(board.get_height(), board.get_width())
        ])
        actual_shichos = np.array([
            board.is_shicho((x, y))
            for y, x in np.ndindex(board.get_height(), board.get_width())
        ])
        np.testing.assert_array_equal(ren_sizes, actual_ren_sizes)
        np.testing.assert_array_equal(ren_spaces, actual_ren_spaces)
        np.testing.assert_array_equal(shichos, actual_shichos)

        for target_color in (COLOR_BLACK, COLOR_WHITE):
            np.testing.assert_array_equal(
                inputs[target_color],
                board.get_inputs(target_color, 7.5, RULE_COM, True),
            )

    def test_play(self) -> None:
        '''Test stone placement.
        Returns:
            None: No return value.
        '''
        for name in ('001', '002', '003', '004', '005', '006', '007', '008', '009'):
            with self.subTest(name):
                self._test_play(name)

    def _test_play(self, name: str) -> None:
        '''Test stone placement.
        Args:
            name (str): Name.
        Returns:
            None: No return value.
        '''
        # Read fixture data
        record, colors, _, _, _, _, _, _ = read_record(name)
        assert colors is not None
        size = int(record.properties['sz'])

        # Create the board
        board = record.create_board()

        # Check stone colors on the board
        for y, x in np.ndindex(size, size):
            if colors[y][x] is None:
                continue

            self.assertEqual(
                colors[y][x], board.get_color((x, y)), f'pos={(x, y)}')

    def test_colors(self) -> None:
        '''Test retrieval of all stone colors.
        Returns:
            None: No return value.
        '''
        for name in ('001', '002', '003', '004', '005', '006', '007', '008', '009'):
            with self.subTest(name):
                self._test_colors(name)

    def _test_colors(self, name: str) -> None:
        '''Test retrieval of all stone colors.
        Args:
            name (str): Name.
        Returns:
            None: No return value.
        '''
        # Read fixture data
        record, colors, _, _, _, _, _, _ = read_record(name)
        assert colors is not None

        # Create the board
        board = record.create_board()

        # Validation
        black_colors = board.get_colors(color=COLOR_BLACK)
        white_colors = board.get_colors(color=COLOR_WHITE)

        for y, x in np.ndindex(board.get_height(), board.get_width()):
            black_color = colors[y][x]

            if black_color is None:
                continue

            white_color = get_opposite_color(black_color)
            self.assertEqual(black_color, black_colors[y, x], f'pos={(x, y)}')
            self.assertEqual(white_color, white_colors[y, x], f'pos={(x, y)}')

    def test_territories(self) -> None:
        '''Test territory calculations.
        Returns:
            None: No return value.
        '''
        # Check the initial board
        board = Board(DEFAULT_SIZE, DEFAULT_SIZE)
        territories = board.get_fixed_territories()
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            self.assertEqual(territories[y, x], COLOR_EMPTY, f'pos={(x, y)}')

        board.play((0, 0), COLOR_BLACK)
        territories = board.get_fixed_territories()
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            self.assertEqual(territories[y, x], COLOR_EMPTY, f'pos={(x, y)}')

        # Check recorded positions
        for name in ('001', '002', '003', '004', '005', '006', '007', '008', '009'):
            with self.subTest(name):
                self._test_territories(name)

    def _test_territories(self, name: str) -> None:
        '''Test territory calculations.
        Args:
            name (str): Name.
        Returns:
            None: No return value.
        '''
        # Read fixture data
        record, _, territories, _, _, _, _, _ = read_record(name)
        assert territories is not None

        # Create the board
        board = record.create_board()

        # Validate the result
        values = board.get_fixed_territories()

        for y, x in np.ndindex(board.get_height(), board.get_width()):
            if territories[y][x] is None:
                continue

            self.assertEqual(territories[y][x], values[y, x], f'pos={(x, y)}')

    def test_owner_with_territories(self) -> None:
        '''Test ownership using supplied territory data.
        Returns:
            None: No return value.
        '''
        board = Board(3, 3)
        board.play((0, 0), COLOR_BLACK)
        board.play((2, 2), COLOR_WHITE)

        # Create territory data that differs from the board's settled territories
        territories = np.zeros((3, 3), dtype=np.int32)
        territories[1, 1] = COLOR_BLACK
        expected_territories = territories.copy()

        # Under Japanese rules, apply only territory data and on-board stones to ownership
        expected = np.zeros((3, 3), dtype=np.int32)
        expected[0, 0] = COLOR_BLACK
        expected[1, 1] = COLOR_BLACK
        expected[2, 2] = COLOR_WHITE

        owners = board.get_owners(territories, RULE_JP)

        np.testing.assert_array_equal(expected, owners)
        np.testing.assert_array_equal(expected_territories, territories)

    def test_owner_territory_shape(self) -> None:
        '''Test territory shape validation.
        Returns:
            None: No return value.
        '''
        board = Board(3, 3)
        territories = np.zeros((2, 3), dtype=np.int32)

        with self.assertRaises(ValueError):
            board.get_owners(territories)

    def test_owner_requires_territories(self) -> None:
        '''Require territory data when calculating ownership.
        Returns:
            None: No return value.
        '''
        board = Board(3, 3)

        with self.assertRaises(TypeError):
            board.get_owners()  # type: ignore[call-arg]

    def test_owner(self) -> None:
        '''Test ownership calculations.
        Returns:
            None: No return value.
        '''
        for rule, name in itertools.product(
                (RULE_JP, RULE_CH, RULE_COM),
                ('001', '002', '003', '004', '005', '006', '007', '008', '009')):
            with self.subTest(f'{name}:{rule}'):
                self._test_owner(name, rule)

    def _test_owner(self, name: str, rule: int) -> None:
        '''Test ownership calculations.
        Args:
            name (str): Name.
            rule (int): Rule.
        Returns:
            None: No return value.
        '''
        # Read fixture data
        record, _, territories, _, _, _, _, _ = read_record(name)
        assert territories is not None

        # Create the board
        board = record.create_board()
        colors = board.get_colors()
        owners = np.zeros((board.get_height(), board.get_width()), dtype=int)

        # Build expected ownership data
        if rule == RULE_CH or rule == RULE_COM:
            areas = _get_areas(board)
        else:
            areas = np.zeros((board.get_height(), board.get_width()), dtype=int)

        for y, x in np.ndindex(board.get_height(), board.get_width()):
            if territories[y][x] is None:
                continue
            elif territories[y][x] == COLOR_EMPTY and colors[y][x] != COLOR_EMPTY:
                owners[y, x] = colors[y][x]
            elif territories[y][x] == COLOR_EMPTY and areas[y, x] != COLOR_EMPTY:
                owners[y, x] = areas[y, x]
            else:
                owners[y, x] = territories[y][x]

        # Validate the result
        board_territories = board.get_fixed_territories()
        values = board.get_owners(board_territories, rule)

        for x, y in np.ndindex(board.get_width(), board.get_height()):
            if territories[y][x] is None:
                continue

            self.assertEqual(owners[y, x], values[y, x], f'pos={(x, y)}')

    def test_score_color(self) -> None:
        '''Test score calculation with ownership from Black's perspective.
        Returns:
            None: No return value.
        '''
        board = Board(3, 3)
        board.play((0, 0), COLOR_BLACK)
        board.play((2, 2), COLOR_WHITE)
        komi = 6.5

        territories = board.get_fixed_territories()
        owners_sum = board.get_owners(territories, RULE_JP).sum()

        self.assertEqual(
            owners_sum - komi, board.get_score(COLOR_BLACK, RULE_JP, komi))
        self.assertEqual(
            -owners_sum + komi, board.get_score(COLOR_WHITE, RULE_JP, komi))

    def test_score_with_territories(self) -> None:
        '''Test scoring with supplied territory data.
        Returns:
            None: No return value.
        '''
        board = Board(3, 3)
        board.play((0, 0), COLOR_BLACK)
        board.play((2, 2), COLOR_WHITE)
        komi = 6.5

        # Create territory data that differs from the board's settled territories
        territories = np.zeros((3, 3), dtype=np.int32)
        territories[1, 1] = COLOR_BLACK
        expected_territories = territories.copy()
        owners_sum = board.get_owners(territories, RULE_JP).sum()

        self.assertEqual(
            owners_sum - komi,
            board.get_score(
                COLOR_BLACK, RULE_JP, komi, territories=territories),
        )
        np.testing.assert_array_equal(expected_territories, territories)

    def test_ren_size(self) -> None:
        '''Test group sizes.
        Returns:
            None: No return value.
        '''
        for name in ('001', '002', '003', '004', '005', '006', '007', '008', '009'):
            with self.subTest(name):
                self._test_ren_size(name)

    def _test_ren_size(self, name: str) -> None:
        '''Test group sizes.
        Args:
            name (str): Name.
        Returns:
            None: No return value.
        '''
        # Read fixture data
        record, _, _, ren_sizes, _, _, _, _ = read_record(name)
        assert ren_sizes is not None

        # Create the board
        board = record.create_board()

        # Validation
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            if ren_sizes[y][x] is None:
                continue

            ren_size = min(board.get_ren_size((x, y)), 9)
            self.assertEqual(ren_sizes[y][x], ren_size, f'pos={(x, y)}')

    def test_ren_space(self) -> None:
        '''Test group liberty counts.
        Returns:
            None: No return value.
        '''
        for name in ('001', '002', '003', '004', '005', '006', '007', '008', '009'):
            with self.subTest(name):
                self._test_ren_space(name)

    def _test_ren_space(self, name: str) -> None:
        '''Test group liberty counts.
        Args:
            name (str): Name.
        Returns:
            None: No return value.
        '''
        # Read fixture data
        record, _, _, _, ren_spaces, _, _, _ = read_record(name)
        assert ren_spaces is not None

        # Create the board
        board = record.create_board()

        # Validation
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            if ren_spaces[y][x] is None:
                continue

            ren_space = min(board.get_ren_space((x, y)), 9)
            self.assertEqual(ren_spaces[y][x], ren_space, f'pos={(x, y)}')

    def test_enabled(self) -> None:
        '''Test legal moves.
        Returns:
            None: No return value.
        '''
        for name in ('001', '002', '003', '004', '005', '006', '007', '008', '009'):
            with self.subTest(name):
                self._test_enabled(name)

    def _test_enabled(self, name: str) -> None:
        '''Test legal moves.
        Args:
            name (str): Name.
        Returns:
            None: No return value.
        '''
        # Read fixture data
        record, _, _, _, _, black_enableds, white_enableds, _ = read_record(name)
        assert black_enableds is not None
        assert white_enableds is not None

        # Create the board
        board = record.create_board()

        # Validation
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            if black_enableds[y][x] is not None:
                self.assertEqual(
                    black_enableds[y][x] == COLOR_EMPTY,
                    board.is_enabled((x, y), COLOR_BLACK),
                    f'black - pos={(x, y)}')

            if white_enableds[y][x] is not None:
                self.assertEqual(
                    white_enableds[y][x] == COLOR_EMPTY,
                    board.is_enabled((x, y), COLOR_WHITE),
                    f'white - pos={(x, y)}')

    def test_enableds(self) -> None:
        '''Test legal moves.
        Returns:
            None: No return value.
        '''
        for name in ('001', '002', '003', '004', '005', '006', '007', '008', '009'):
            with self.subTest(name):
                self._test_enableds(name)

    def _test_enableds(self, name: str) -> None:
        '''Test legal moves.
        Args:
            name (str): Name.
        Returns:
            None: No return value.
        '''
        # Create the board
        record, _, _, _, _, _, _, _ = read_record(name)
        board = record.create_board()

        # Validate the result
        for color in (COLOR_BLACK, COLOR_WHITE):
            enableds = board.get_enableds(color)

            for y, x in np.ndindex(board.get_height(), board.get_width()):
                enabled = board.is_enabled((x, y), color)
                message = f'pos={(x, y)}, color={color},'
                self.assertEqual(enableds[y, x], int(enabled), message)

    def test_ko(self) -> None:
        '''Test ko coordinates.
        Returns:
            None: No return value.
        '''
        for name, pos, color in (
            ('011', (6, 10), COLOR_WHITE),  # Active ko
            ('012', MOVE_PASS, COLOR_WHITE),  # No ko
            ('013', (18, 16), COLOR_WHITE),  # Active ko
            ('014', MOVE_PASS, COLOR_WHITE),  # Ko cleared by a move
            ('015', MOVE_PASS, COLOR_WHITE),  # Ko cleared by a move
            ('016', MOVE_PASS, COLOR_WHITE),  # Ko cleared by a pass
        ):
            with self.subTest(f'{name}:{pos}:{color}'):
                self._test_ko(name, pos, color)

    def _test_ko(self, name: str, pos: Tuple[int, int], color: int) -> None:
        '''Test ko coordinates.
        Args:
            name (str): Name.
            pos (Tuple[int, int]): Pos.
            color (int): Color.
        Returns:
            None: No return value.
        '''
        # Read fixture data
        record, _, _, _, _, black_enableds, white_enableds, _ = read_record(name)
        assert black_enableds is not None
        assert white_enableds is not None

        # Create the board
        board = record.create_board()

        # Check legal moves
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            self.assertEqual(
                black_enableds[y][x] == COLOR_EMPTY,
                board.is_enabled((x, y), COLOR_BLACK),
                f'black - pos={(x, y)}')
            self.assertEqual(
                white_enableds[y][x] == COLOR_EMPTY,
                board.is_enabled((x, y), COLOR_WHITE),
                f'white - pos={(x, y)}')

        # Check ko coordinates
        self.assertEqual(pos, board.get_ko(color))
        self.assertEqual(MOVE_PASS, board.get_ko(get_opposite_color(color)))

    def test_seki(self) -> None:
        '''Test seki detection.
        Returns:
            None: No return value.
        '''
        for name in (
            '001', '002', '003', '004', '005', '006', '007', '008', '009',
            '051', '052', '053', '054', '055', '056', '057', '058', '059',
            '060', '061', '062', '063',
        ):
            with self.subTest(name):
                self._test_seki(name)

    def _test_seki(self, name: str) -> None:
        '''Test seki detection.
        Args:
            name (str): Name.
        Returns:
            None: No return value.
        '''
        # Read fixture data
        record, _, _, _, _, black_enableds, white_enableds, _ = read_record(name)
        assert black_enableds is not None
        assert white_enableds is not None

        # Create the board
        board_size = int(record.properties['sz'])
        board = Board(board_size, board_size)

        if 'ha' in record.properties:
            board.set_handicap(int(record.properties['ha']))

        for pos, color, _ in record.moves:
            if not board.is_valid_position(pos):
                continue

            self.assertTrue(
                board.is_enabled(pos, color, check_seki=True),
                f'pos={pos}, color={get_color_name(color)}')
            board.play(pos, color)

        # Validate the result
        board_string = str(board)
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            if black_enableds[y][x] is not None:
                self.assertEqual(
                    black_enableds[y][x] == COLOR_EMPTY,
                    board.is_enabled((x, y), COLOR_BLACK, check_seki=True),
                    f'black - pos={(x, y)}\n{board_string}')

            if white_enableds[y][x] is not None:
                self.assertEqual(
                    white_enableds[y][x] == COLOR_EMPTY,
                    board.is_enabled((x, y), COLOR_WHITE, check_seki=True),
                    f'white - pos={(x, y)}\n{board_string}')

    def test_shicho(self) -> None:
        '''Test ladder detection.
        Returns:
            None: No return value.
        '''
        for name in (
            '001', '002', '003', '004', '005', '006', '007', '008', '009',
            '101', '102', '103', '104', '105', '106', '107', '108',
        ):
            with self.subTest(name):
                self._test_shicho(name)

    def _test_shicho(self, name: str) -> None:
        '''Test ladder detection.
        Args:
            name (str): Name.
        Returns:
            None: No return value.
        '''
        record, _, _, _, _, black_enableds, white_enableds, shichos = read_record(name)
        assert black_enableds is not None
        assert white_enableds is not None

        # Create the board
        board_size = int(record.properties['sz'])
        board = Board(board_size, board_size)

        if 'ha' in record.properties:
            board.set_handicap(int(record.properties['ha']))

        for pos, color, _ in record.moves:
            if not board.is_valid_position(pos):
                continue

            board.play(pos, color)

        # Validate the result
        board_string = str(board)
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            if shichos is not None:
                self.assertEqual(
                    shichos[y][x] == 1,
                    board.is_shicho((x, y)),
                    f'shicho - pos={(x, y)}\n{board_string}')

    def test_inputs(self) -> None:
        '''Test model input construction.
        Returns:
            None: No return value.
        '''
        for name, color, komi, rule, superko in (
            ('001', COLOR_BLACK, 6.5, RULE_CH, False),
            ('002', COLOR_WHITE, 6.5, RULE_CH, False),
            ('003', COLOR_BLACK, 7.5, RULE_JP, False),
            ('004', COLOR_WHITE, 7.5, RULE_JP, False),
            ('005', COLOR_BLACK, 5.5, RULE_CH, True),
            ('006', COLOR_WHITE, 5.5, RULE_CH, True),
            ('007', COLOR_BLACK, 7.5, RULE_JP, True),
            ('008', COLOR_WHITE, 7.5, RULE_JP, True),
            ('101', COLOR_BLACK, 6.5, RULE_CH, False),
            ('102', COLOR_WHITE, 6.5, RULE_CH, False),
            ('103', COLOR_BLACK, 8.5, RULE_JP, False),
            ('104', COLOR_WHITE, 8.5, RULE_JP, False),
            ('105', COLOR_BLACK, 4.5, RULE_CH, True),
            ('106', COLOR_WHITE, 4.5, RULE_CH, True),
            ('107', COLOR_BLACK, 9.5, RULE_JP, True),
            ('108', COLOR_WHITE, 9.5, RULE_JP, True),
            ('011', COLOR_WHITE, 3.5, RULE_CH, False),
            ('013', COLOR_WHITE, 3.5, RULE_CH, True),
        ):
            with self.subTest(f'{name},{get_color_name(color)},{komi},{superko}'):
                self._test_inputs(name, color, komi, rule, superko)

    def _test_inputs(
        self, name: str, color: int, komi: float, rule: int, superko: bool,
    ) -> None:
        '''Test model input construction.
        Args:
            name (str): Name.
            color (int): Color.
            komi (float): Komi.
            rule (int): Rule.
            superko (bool): Superko.
        Returns:
            None: No return value.
        '''
        # Create the board
        board = read_record(name)[0].create_board()
        ko_pos = board.get_ko(color)

        # Build inference inputs
        inputs = board.get_inputs(color, komi, rule, superko)
        inputs = unpack(inputs[np.newaxis, :])[0]
        (board_inputs, mask_inputs, info_inputs) = np.split(
            inputs, [MODEL_MASK_OFFSET, MODEL_INFO_OFFSET], axis=0)
        board_inputs = board_inputs.reshape(MODEL_FEATURE_NUM, MODEL_BOARD_SIZE, MODEL_BOARD_SIZE)
        mask_inputs = mask_inputs.reshape(MODEL_BOARD_SIZE, MODEL_BOARD_SIZE)

        # Build expected data
        board_valids = np.zeros(
            (MODEL_FEATURE_NUM,
             MODEL_BOARD_SIZE,
             MODEL_BOARD_SIZE),
            dtype=np.float32)
        mask_valids = np.zeros((MODEL_BOARD_SIZE, MODEL_BOARD_SIZE), dtype=np.float32)
        info_valids = np.zeros((MODEL_INFO_SIZE,), dtype=np.float32)
        offset_x = (MODEL_BOARD_SIZE - board.get_width()) // 2
        offset_y = (MODEL_BOARD_SIZE - board.get_height()) // 2

        # Build expected stone features
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            c = board.get_color((x, y)) * color

            if c == COLOR_BLACK:
                board_valids[1, offset_y + y, offset_x + x] = 1
            elif c == COLOR_WHITE:
                board_valids[11, offset_y + y, offset_x + x] = 1
            else:
                board_valids[0, offset_y + y, offset_x + x] = 1

        # Build expected ladder features
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            c = board.get_color((x, y)) * color

            if c == COLOR_BLACK:
                board_valids[2, offset_y + y, offset_x + x] = int(board.is_shicho((x, y)))
            elif c == COLOR_WHITE:
                board_valids[12, offset_y + y, offset_x + x] = int(board.is_shicho((x, y)))

        # Build expected group features
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            c = board.get_color((x, y)) * color
            s = board.get_ren_space((x, y))

            if c == COLOR_BLACK and 0 < s <= 8:
                board_valids[3 + s - 1, offset_y + y, offset_x + x] = 1
            elif c == COLOR_WHITE and 0 < s <= 8:
                board_valids[13 + s - 1, offset_y + y, offset_x + x] = 1

        # Build expected distance-to-edge features
        for i in range(10):
            width = board.get_width()
            height = board.get_height()
            begin_x = offset_x + i
            end_x = offset_x + width - i
            begin_y = offset_y + i
            end_y = offset_y + height - i

            if begin_x < end_x and begin_y < end_y:
                board_valids[21 + i, begin_y, begin_x:end_x] = 1
                board_valids[21 + i, end_y - 1, begin_x:end_x] = 1
                board_valids[21 + i, begin_y:end_y, begin_x] = 1
                board_valids[21 + i, begin_y:end_y, end_x - 1] = 1

        # Build expected ko coordinates
        if board.is_valid_position(ko_pos):
            board_valids[31, offset_y + ko_pos[1], offset_x + ko_pos[0]] = 1

        # Build the expected board mask
        for y, x in np.ndindex(board.get_height(), board.get_width()):
            mask_valids[offset_y + y, offset_x + x] = 1

        # Build the expected superko feature
        if superko:
            info_valids[0] = 1

        # Build the expected ko status
        if board.is_valid_position(ko_pos):
            info_valids[1] = 1

        # Build the expected rule features
        if rule == RULE_JP:
            info_valids[2] = 1
        else:
            info_valids[3] = 1

        # Build the expected board size
        info_valids[4] = (math.sqrt(board.get_width() * board.get_height()) - 14.0) / 5.0

        # Build the expected komi
        info_valids[5] = (komi * color) / 10.0

        # Check array shapes
        self.assertEqual(board_valids.shape, board_inputs.shape)
        self.assertEqual(mask_valids.shape, mask_inputs.shape)
        self.assertEqual(info_valids.shape, info_inputs.shape)

        # Check data types
        self.assertEqual(board_valids.dtype, board_inputs.dtype)
        self.assertEqual(mask_valids.dtype, mask_inputs.dtype)
        self.assertEqual(info_valids.dtype, info_inputs.dtype)

        # Check board features
        board_string = str(board)

        for i in range(board_valids.shape[0]):
            self.assertTrue(
                np.allclose(board_valids[i], board_inputs[i]),
                f'\n{board_string}\nerror at layer {i}'
                f'\nboard_valids:\n{board_valids[i]}\nboard_inputs:\n{board_inputs[i]}')

        self.assertTrue(
            np.allclose(mask_valids, mask_inputs),
            f'\nmask_valids:\n{mask_valids}\nmask_inputs:\n{mask_inputs}')

        self.assertTrue(
            np.allclose(info_valids, info_inputs, atol=1 / MODEL_VALUE_SCALE),
            f'\ninfo_valids:\n{info_valids}\ninfo_inputs:\n{info_inputs}')

    def test_copy(self) -> None:
        '''Test board copying.
        Returns:
            None: No return value.
        '''
        for name1, name2 in (
            ('001', '002'), ('002', '003'), ('003', '001'),
            ('004', '005'), ('005', '006'), ('006', '004'),
            ('007', '008'), ('008', '009'), ('009', '007'),
            ('011', '012'), ('012', '013'), ('013', '014'),
            ('014', '015'), ('015', '016'), ('016', '011'),
        ):
            with self.subTest(f'{name1},{name2}'):
                self._test_copy(name1, name2)

    def _test_copy(self, name1: str, name2: str) -> None:
        '''Test board copying.
        Args:
            name1 (str): Name1.
            name2 (str): Name2.
        Returns:
            None: No return value.
        '''
        # Create board objects
        board1 = read_record(name1)[0].create_board()
        board2 = read_record(name2)[0].create_board()

        # Populate internal territory and ladder information
        board2.get_fixed_territories()
        board2.is_shicho((0, 0))

        # Copy board1 into board2
        board2.copy_from(board1)

        # Validate the result
        for y, x in np.ndindex(board1.get_height(), board1.get_width()):
            self.assertEqual(
                board1.get_color((x, y)), board2.get_color((x, y)), f'pos={(x, y)}')
            self.assertEqual(
                board1.is_enabled((x, y), COLOR_BLACK),
                board2.is_enabled((x, y), COLOR_BLACK),
                f'color=black, pos={(x, y)}')
            self.assertEqual(
                board1.is_enabled((x, y), COLOR_WHITE),
                board2.is_enabled((x, y), COLOR_WHITE),
                f'color=white, pos={(x, y)}')
            self.assertEqual(
                board1.get_ren_size((x, y)), board2.get_ren_size((x, y)), f'pos={(x, y)}')
            self.assertEqual(
                board1.get_ren_space((x, y)), board2.get_ren_space((x, y)), f'pos={(x, y)}')

        self.assertEqual(
            list(board1.get_fixed_territories().flatten()),
            list(board2.get_fixed_territories().flatten()))
        self.assertEqual(board1.get_ko(COLOR_BLACK), board2.get_ko(COLOR_BLACK))
        self.assertEqual(board1.get_ko(COLOR_WHITE), board2.get_ko(COLOR_WHITE))
