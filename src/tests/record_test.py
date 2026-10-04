import io
import re
import unittest
from pathlib import Path
from typing import Dict, Tuple

from deepgo.board import is_valid_position
from deepgo.config import COLOR_BLACK, COLOR_WHITE, MOVE_PASS
from deepgo.record import Record


class RecordTest(unittest.TestCase):
    '''Regression tests for the Record class.'''

    def test_load(self) -> None:
        '''Test loading game records.
        Returns:
            None: No return value.
        '''
        for name in ('001', '002', '003', '004', '005', '006', '007', '008', '009'):
            with self.subTest(name):
                self._test_load(name)

    def _test_load(self, name: str) -> None:
        '''Test loading game records.
        Args:
            @param name Name of the fixture file
            name (str): Name.
        Returns:
            None: No return value.
        '''
        path = Path(__file__).parent / 'records' / f'{name}.sgf'

        # Parse SGF independently to obtain expected data
        text = path.read_text().replace('\n', '')[2:-1]
        values = [
            dict((m.group(1).lower(), m.group(2))
                 for m in re.finditer(r'(\w+)\[([^\]]*)\]', token))
            for token in text.split(';')]

        # Property data
        props = values[0]
        size = int(props['sz'])

        # Coordinate data
        def get_idx(s: int) -> int:
            '''Decode an SGF coordinate character.
            Args:
                s (int): s
            Returns:
                int: Result of the operation.
            '''
            return s - 97 if 97 <= s < 97 + size else -1

        def get_pos(s: str) -> Tuple[int, int]:
            '''Decode an SGF move coordinate.
            Args:
                s (str): s
            Returns:
                Tuple[int, int]: Result of the operation.
            '''
            if len(s) == 2:
                p = get_idx(ord(s[0])), get_idx(ord(s[1]))
                if is_valid_position(p, size, size):
                    return p
                else:
                    return MOVE_PASS
            else:
                return MOVE_PASS

        def get_move(v: Dict[str, str]) -> Tuple[Tuple[int, int], int]:
            '''Extract a move from an SGF node.
            Args:
                v (Dict[str, str]): v
            Returns:
                Tuple[Tuple[int, int], int]: Result of the operation.
            '''
            if 'b' in v:
                return get_pos(v['b']), COLOR_BLACK
            else:
                return get_pos(v['w']), COLOR_WHITE

        def get_message(v: Dict[str, str]) -> str | None:
            '''Extract an SGF comment.
            Args:
                v (Dict[str, str]): v
            Returns:
                str | None: Result of the operation.
            '''
            return v['c'] if 'c' in v else None

        moves = [get_move(vals) for vals in values[1:] if 'b' in vals or 'w' in vals]
        messages = [get_message(vals) for vals in values[1:] if 'b' in vals or 'w' in vals]

        # Load the SGF file
        record = Record(path)

        # Check properties
        self.assertEqual(props['km'], record.properties['km'])
        self.assertEqual(props['pb'], record.properties['pb'])
        self.assertEqual(props['pw'], record.properties['pw'])
        self.assertEqual(props['re'], record.properties['re'])

        # Check coordinates
        self.assertEqual(len(moves), len(record.moves))

        for move1, move2 in zip(moves, record.moves):
            self.assertEqual(move1, move2[:2])

        for message1, move2 in zip(messages, record.moves):
            self.assertEqual(message1, move2[2])

    def test_dump(self) -> None:
        '''Test writing game records.
        Returns:
            None: No return value.
        '''
        for name in ('001', '002', '003', '004', '005', '006', '007', '008', '009'):
            with self.subTest(name):
                self._test_dump(name)

    def _test_dump(self, name: str) -> None:
        '''Test writing game records.
        Args:
            @param name Name of the fixture file
            name (str): Name.
        Returns:
            None: No return value.
        '''
        path = Path(__file__).parent / 'records' / f'{name}.sgf'

        # Load the SGF file
        record1 = Record(path)

        # Write to a stream
        file = io.StringIO()

        record1.dump(file)

        # Read the stream
        record2 = Record(io.StringIO(file.getvalue()))

        # Validation
        self.assertEqual(record1.properties, record2.properties)
        self.assertEqual(record1.moves, record2.moves)

    def test_escape(self) -> None:
        '''Test escaping SGF comments.
        Returns:
            None: No return value.
        '''
        path = Path(__file__).parent / 'records' / '001.sgf'

        # Load the SGF file
        record1 = Record(path)

        # Set a comment requiring escaping
        record1.moves[0] = record1.moves[0][:2] + ('([test]);\\',)

        # Write to a stream
        file = io.StringIO()

        record1.dump(file)

        # Read the stream
        record2 = Record(io.StringIO(file.getvalue()))

        # Validation
        self.assertEqual(record1.moves, record2.moves)
