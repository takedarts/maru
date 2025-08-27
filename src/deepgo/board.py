import functools
import struct
from typing import List, Tuple

import numpy as np

from .config import BLACK, DEFAULT_KOMI, RULE_CH, WHITE
from .exception import GoException
from .native import NativeBoard


def get_opposite_color(color: int) -> int:
    '''Get the opposite color of the specified color.
    Args:
        color (int): Stone color
    Returns:
        int: Opponent's stone color
    '''
    return color * -1


def get_color_name(color: int) -> str:
    '''Get the string representing the stone color.
    Args:
        color (int): Stone color
    Returns:
        str: String representing the stone color
    '''
    if color == BLACK:
        return 'black'
    elif color == WHITE:
        return 'white'
    else:
        return 'empty'


def get_color_mark(color: int) -> str:
    '''Get the symbol representing the stone color.
    Args:
        color (int): Stone color
    Returns:
        str: Symbol representing the stone color
    '''
    if color == BLACK:
        return 'X'
    elif color == WHITE:
        return 'O'
    else:
        return '.'


def is_valid_position(pos: tuple[int, int], width: int, height: int) -> bool:
    '''Determine whether the specified coordinates are within the board.
    Args:
        pos (Tuple[int, int]): Coordinates
        width (int): Board width
        height (int): Board height
    Returns:
        bool: True if within the board
    '''
    return 0 <= pos[0] < width and 0 <= pos[1] < height


def _get_array_string(
    values: np.ndarray,
    pos: Tuple[int, int] | None = None,
) -> List[str]:
    '''Return the string representation of the given data.
    BLACK is represented as X, WHITE as O, and empty as .
    Args:
        values (np.ndarray): Data
        pos (Tuple[int, int]): Highlighted position
    Returns:
        List[str]: Board as string
    '''
    text = []
    text.append('   ' + ''.join(f'{i:2d}' for i in range(values.shape[1])) + '  ')
    text.append('  +' + ('-' * (values.shape[1] * 2 + 1)) + '+')

    for y in range(values.shape[0]):
        line = f'{y:2d}|'

        if pos is not None and pos[0] == 0 and pos[1] == y:
            line += '['
        else:
            line += ' '

        for x in range(values.shape[1]):
            line += get_color_mark(values[y, x])

            if pos is not None and pos[0] == x and pos[1] == y:
                line += ']'
            elif pos is not None and pos[0] == x + 1 and pos[1] == y:
                line += '['
            else:
                line += ' '

        line += '|'
        text.append(line)

    text.append('  +' + ('-' * (values.shape[1] * 2 + 1)) + '+')

    return text


def get_array_string(
    *values: np.ndarray,
    pos: Tuple[int, int] | None = None,
) -> str:
    '''Return the string representation of the given board data.
    Args:
        *values (np.ndarray): Board data
        pos (Tuple[int, int]): Highlighted position
    Returns:
        str: Board as string
    '''
    return "\n".join([' '.join(s) for s in zip(*[_get_array_string(v, pos) for v in values])])


def get_board_string(
    *boards: 'Board',
    pos: Tuple[int, int] | None = None,
) -> str:
    '''Return the string representation of the given board data.
    Args:
        *boards (Board): Board
        pos (Tuple[int, int]): Highlighted position
    Returns:
        str: Board as string
    '''
    return get_array_string(*[v.get_colors() for v in boards], pos=pos)


def get_handicap_positions(width: int, height: int, handicap: int) -> List[Tuple[int, int]]:
    positions: List[Tuple[int, int]] = []
    ver_line = 3 if width >= 13 else 2
    hor_line = 3 if height >= 13 else 2

    if handicap >= 2:
        positions.append((width - ver_line - 1, hor_line))
        positions.append((ver_line, height - hor_line - 1))

    if handicap >= 3:
        positions.append((ver_line, hor_line))

    if handicap >= 4:
        positions.append((width - ver_line - 1, height - hor_line - 1))

    if handicap == 5:
        positions.append((width // 2, height // 2))

    if handicap >= 6:
        positions.append((ver_line, height // 2))
        positions.append((width - ver_line - 1, height // 2))

    if handicap == 7:
        positions.append((width // 2, height // 2))

    if handicap >= 8:
        positions.append((width // 2, hor_line))
        positions.append((width // 2, height - hor_line - 1))

    if handicap == 9:
        positions.append((width // 2, height // 2))

    return positions


class Board(object):
    def __init__(self, width: int, height: int) -> None:
        '''Initialize the board object.
        Args:
            width (int): Board width
            height (int): Board height
        '''
        self.native = NativeBoard(width, height)

    def get_width(self) -> int:
        '''Get the width of the board.
        Returns:
            int: Board width
        '''
        return self.native.get_width()

    def get_height(self) -> int:
        '''Get the height of the board.
        Returns:
            int: Board height
        '''
        return self.native.get_height()

    def is_valid_position(self, pos: tuple[int, int]) -> bool:
        '''Determine whether the specified coordinates are within the board.
        Args:
            pos (Tuple[int, int]): Coordinates
        Returns:
            bool: True if within the board
        '''
        return is_valid_position(pos, self.get_width(), self.get_height())

    def set_handicap(self, handicap: int) -> None:
        '''Set handicap stones.
        Args:
            handicap (int): Number of handicap stones
        '''
        for pos in get_handicap_positions(self.get_width(), self.get_height(), handicap):
            self.native.play(pos, BLACK)

    def play(self, pos: tuple[int, int], color: int) -> int:
        '''Place a stone at the specified position.
        Args:
            pos (Tuple[int, int]): Position to place
            color (int): Stone color
        Returns:
            int: Number of captured stones
        '''
        captured = self.native.play(pos, color)

        if captured < 0:
            raise GoException(f'Invalid move: {pos} {get_color_name(color)}')

        return captured

    def get_ko(self, color: int) -> tuple[int, int]:
        '''Get the ko position.
        Args:
            color (int): Stone color for ko
        Returns:
            Tuple[int, int]: Ko position
        '''
        return self.native.get_ko(color)

    def get_histories(self, color: int) -> List[tuple[int, int]]:
        '''Get move history.
        Args:
            color (int): Move color
        Returns:
            List[Tuple[int, int]]: List of moves
        '''
        return self.native.get_histories(color)

    def get_color(self, pos: tuple[int, int]) -> int:
        '''Get the stone color at the specified position.
        Args:
            pos (Tuple[int, int]): Position
        Returns:
            int: Stone color
        '''
        return self.native.get_color(pos)

    def get_colors(self, color: int = BLACK) -> np.ndarray:
        '''Get the list of stone colors.
        If WHITE is specified as an argument, returns the board with black and white reversed.
        Args:
            color (int): Stone color
        Returns:
            np.ndarray: List of stone colors
        '''
        return self.native.get_colors(color)

    def get_ren_size(self, pos: tuple[int, int]) -> int:
        '''Get the size of the group at the specified position.
        Args:
            pos (Tuple[int, int]): Position
        Returns:
            int: Size of the group
        '''
        return self.native.get_ren_size(pos)

    def get_ren_space(self, pos: tuple[int, int]) -> int:
        '''Get the number of liberties of the group at the specified position.
        Args:
            pos (Tuple[int, int]): Position
        Returns:
            int: Number of liberties of the group
        '''
        return self.native.get_ren_space(pos)

    def is_shicho(self, pos: tuple[int, int]) -> bool:
        '''Determine whether the specified position is a ladder (shicho).
        Args:
            pos (Tuple[int, int]): Position
        Returns:
            bool: True if it is a ladder
        '''
        return self.native.is_shicho(pos)

    def is_enabled(
        self,
        pos: tuple[int, int],
        color: int,
        check_seki: bool = False,
    ) -> bool:
        '''Determine whether a stone can be placed at the specified position.
        Args:
            pos (Tuple[int, int]): Position
            color (int): Stone color
            check_seki (bool): Whether to consider seki
        Returns:
            bool: True if a stone can be placed
        '''
        return self.native.is_enabled(pos, color, check_seki)

    def get_enableds(
        self,
        color: int,
        check_seki: bool = False,
    ) -> np.ndarray:
        '''Get the positions where a stone can be placed.
        Args:
            color (int): Stone color
            check_seki (bool): Whether to consider seki
        Returns:
            np.ndarray: Positions where a stone can be placed
        '''
        return self.native.get_enableds(color, check_seki)

    def get_territories(self, color: int = BLACK) -> np.ndarray:
        '''Get the list of confirmed territories.
        Args:
            color (int): Reference stone color (if WHITE, returns with black and white reversed)
        Returns:
            np.ndarray: List of confirmed territories
        '''
        return self.native.get_territories(color)

    def get_owners(self, color: int = BLACK, rule: int = RULE_CH) -> np.ndarray:
        '''Get the list of owners for each position.
        Args:
            color (int): Reference stone color (if WHITE, returns with black and white reversed)
            rule (int): Rule for determining the winner
        Returns:
            np.ndarray: List of owners
        '''
        return self.native.get_owners(color, rule)

    def get_score(
        self,
        color: int = BLACK,
        rule: int = RULE_CH,
        komi: float = DEFAULT_KOMI,
    ) -> float:
        '''Get the score for the specified color.
        Args:
            color (int): Stone color to calculate score
            rule (int): Rule for determining the winner
            komi (float): Komi points
        Returns:
            float: Score
        '''
        if color == BLACK:
            return self.get_owners(color, rule).sum() - komi
        else:
            return self.get_owners(color, rule).sum() + komi

    def get_patterns(self) -> List[int]:
        '''Get values representing the arrangement of stones.
        Returns:
            List[int]: Values representing the arrangement of stones
        '''
        return self.native.get_patterns()

    def get_inputs(
        self,
        color: int,
        komi: float,
        rule: int,
        superko: bool,
    ) -> np.ndarray:
        '''Get the data to input to the inference model.
        Args:
            color (int): Stone color to play
            komi (float): Komi points
            rule (int): Rule for determining the winner
            superko (bool): Whether to apply superko rule
        Returns:
            np.ndarray: Input data
        '''
        return self.native.get_inputs(color, komi, rule, superko)

    def get_state(self) -> List[int]:
        '''Return the serialized value of the board state.
        :return: Board state value
        '''
        return self.native.get_state()

    def load_state(self, state: List[int]) -> None:
        '''Deserialize the board state.
        :param state: Board state value
        '''
        self.native.load_state(state)

    def copy_from(self, board: 'Board') -> None:
        '''Copy the board.
        Args:
            board (Board): Source board to copy from
        '''
        self.native.copy_from(board.native)

    def __getstate__(self) -> bytes:
        width = self.get_width()
        height = self.get_height()
        values = self.native.get_state()
        return struct.pack(f'>bb{len(values)}i', width, height, *values)

    def __setstate__(self, state: bytes) -> None:
        values = list(struct.unpack(f'>bb{(len(state) - 2) // 4}i', state))
        width = values[0]
        height = values[1]
        self.native = NativeBoard(width, height)
        self.native.load_state(values[2:])

    def __hash__(self) -> int:
        return int(functools.reduce(lambda x, y: x ^ y, self.native.get_state()))

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Board):
            return False

        return (self.native.get_state() == other.native.get_state()).all()

    def __str__(self) -> str:
        '''Represent the board as a string.
        Returns:
            str: String representation of the board
        '''
        return get_board_string(self)
