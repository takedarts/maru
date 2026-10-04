from typing import List, Tuple

import numpy as np

from .config import COLOR_BLACK, COLOR_WHITE, DEFAULT_KOMI, RULE_CH
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
    if color == COLOR_BLACK:
        return 'black'
    elif color == COLOR_WHITE:
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
    if color == COLOR_BLACK:
        return 'X'
    elif color == COLOR_WHITE:
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
    '''Return the standard fixed-handicap coordinates.
    Args:
        width (int): Board width
        height (int): Board height
        handicap (int): Number of handicap stones
    Returns:
        List[Tuple[int, int]]: Result of the operation.
    '''
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
    '''Go board backed by the native rules implementation.
    '''
    def __init__(self, width: int, height: int) -> None:
        '''Initialize the board object.
        Args:
            width (int): Board width
            height (int): Board height
        Returns:
            None: No return value.
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
        Returns:
            None: No return value.
        '''
        for pos in get_handicap_positions(self.get_width(), self.get_height(), handicap):
            self.native.play(pos, COLOR_BLACK)

    def play(
        self,
        pos: tuple[int, int],
        color: int,
    ) -> Tuple[
        Tuple[int, int],
        int,
        int,
        Tuple[bool, bool, bool, bool],
        Tuple[int, int],
    ]:
        '''Place a stone at the specified position.
        Args:
            pos (Tuple[int, int]): Position to place
            color (int): Stone color
        Returns:
            Tuple[Tuple[int, int], int, int, Tuple[bool, bool, bool, bool],
                  Tuple[int, int]]: Move result
        '''
        try:
            return self.native.play(pos, color)
        except ValueError as error:
            raise GoException(
                f'Invalid move: {pos} {get_color_name(color)}: {error}\n'
                f'{get_board_string(self)}') from error

    def undo(
        self,
        result: Tuple[
            Tuple[int, int],
            int,
            int,
            Tuple[bool, bool, bool, bool],
            Tuple[int, int],
        ],
    ) -> None:
        '''Undo the specified move.
        Args:
            result (Tuple[Tuple[int, int], int, int, Tuple[bool, bool, bool, bool],
            Tuple[int, int]]): Result of the move to undo
        Returns:
            None: No return value.
        '''
        try:
            self.native.undo(result)
        except ValueError as error:
            raise GoException(
                f'Invalid undo result: {result}: {error}\n'
                f'{get_board_string(self)}') from error

    def get_ko(self, color: int) -> tuple[int, int]:
        '''Get the ko position.
        Args:
            color (int): Stone color for ko
        Returns:
            Tuple[int, int]: Ko position
        '''
        return self.native.get_ko(color)

    def get_color(self, pos: tuple[int, int]) -> int:
        '''Get the stone color at the specified position.
        Args:
            pos (Tuple[int, int]): Position
        Returns:
            int: Stone color
        '''
        return self.native.get_color(pos)

    def get_colors(self, color: int = COLOR_BLACK) -> np.ndarray:
        '''Get the list of stone colors.
        If WHITE is specified as an argument, returns the board with black and white reversed.
        Args:
            color (int): Stone color
        Returns:
            np.ndarray: List of stone colors
        '''
        return self.native.get_colors(color)

    def get_pattern(self) -> Tuple[int, ...]:
        '''Get values representing the stone arrangement.
        Returns:
            Tuple[int, ...]: Black and white bitboards split into 32-bit words
        '''
        return self.native.get_pattern()

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

    def get_fixed_territories(self) -> np.ndarray:
        '''Get the list of confirmed territories.
        Returns:
            np.ndarray: List of confirmed territories
        '''
        return self.native.get_fixed_territories()

    def get_owners(
        self,
        territories: np.ndarray,
        rule: int = RULE_CH,
    ) -> np.ndarray:
        '''Get the list of owners for each position.
        Args:
            territories (np.ndarray): Territory data
            rule (int): Rule for determining the winner
        Returns:
            np.ndarray: List of owners
        '''
        # Pass territory data with the same shape as the board to C++
        territory_shape = (self.get_height(), self.get_width())

        if territories.shape != territory_shape:
            raise ValueError(
                f'territories shape must be {territory_shape}: {territories.shape}')

        territory_data = np.ascontiguousarray(territories, dtype=np.int32)

        return self.native.get_owners(territory_data, rule)

    def get_score(
        self,
        color: int = COLOR_BLACK,
        rule: int = RULE_CH,
        komi: float = DEFAULT_KOMI,
        territories: np.ndarray | None = None,
    ) -> float:
        '''Get the score for the specified color.
        Args:
            color (int): Stone color to calculate score
            rule (int): Rule for determining the winner
            komi (float): Komi points
            territories (np.ndarray | None): Territory data
        Returns:
            float: Score
        '''
        # Use the board's settled territories if no territory data is supplied
        if territories is None:
            territories = self.get_fixed_territories()

        # Calculate ownership from territory data
        score = self.get_owners(territories, rule).sum() - komi

        if color == COLOR_BLACK:
            return score
        else:
            return -1 * score

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

    def copy_from(self, board: 'Board') -> None:
        '''Copy the board.
        Args:
            board (Board): Source board to copy from
        Returns:
            None: No return value.
        '''
        self.native.copy_from(board.native)

    def __hash__(self) -> int:
        '''Reject hashing because board hashing is not implemented.
        Returns:
            int: Result of the operation.
        '''
        raise NotImplementedError('not implemented yet')

    def __eq__(self, other: object) -> bool:
        '''Compare board types; board-state equality is not implemented.
        Args:
            other (object): Object to compare
        Returns:
            bool: Result of the operation.
        '''
        if not isinstance(other, Board):
            return False

        raise NotImplementedError('not implemented yet')

    def __str__(self) -> str:
        '''Represent the board as a string.
        Returns:
            str: String representation of the board
        '''
        return get_board_string(self)
