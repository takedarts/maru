from typing import List, Tuple

from libc.stdint cimport int32_t, uint32_t
from libcpp.pair cimport pair
from libcpp.vector cimport vector

import numpy
cimport numpy

from deepgo.config import MODEL_INPUT_PACK_SIZE
from pyx.board cimport Board
from pyx.move cimport Move, MoveResult


cdef class NativeBoard:
    '''Expose native board rules to Python.'''
    cdef Board *board

    def __cinit__(self, width: int, height: int) -> None:
        '''Create board object.
        Args:
            width (int): Board width
            height (int): Board height
        '''
        self.board = new Board(width, height)

    def __dealloc__(self) -> None:
        '''Destroy the board object.'''
        del self.board

    def get_width(self) -> int:
        '''Get the width of the board.
        Returns:
            int: Board width
        '''
        return self.board.getWidth()

    def get_height(self) -> int:
        '''Get the height of the board.
        Returns:
            int: Board height
        '''
        return self.board.getHeight()

    def play(
        self,
        pos: Tuple[int, int],
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
            pos (Tuple[int, int]): Position to place the stone
            color (int): Stone color
        Returns:
            Tuple[Tuple[int, int], int, int, Tuple[bool, bool, bool, bool],
                  Tuple[int, int]]: Move result
        '''
        cdef MoveResult result = self.board.play(Move(pos[0], pos[1], color))
        cdef pair[int32_t, int32_t] previous_ko = result.getPreviousKo()

        return (
            (result.getX(), result.getY()),
            result.getColor(),
            result.getCaptured(),
            (
                result.getCapturedDirection(0),
                result.getCapturedDirection(1),
                result.getCapturedDirection(2),
                result.getCapturedDirection(3),
            ),
            (previous_ko.first, previous_ko.second),
        )

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
        '''
        cdef Move move = Move(result[0][0], result[0][1], result[1])
        cdef pair[int32_t, int32_t] previous_ko = pair[int32_t, int32_t](
            result[4][0], result[4][1])
        cdef MoveResult native_result = MoveResult(
            move,
            result[2],
            result[3][0],
            result[3][1],
            result[3][2],
            result[3][3],
            previous_ko,
        )

        self.board.undo(native_result)

    def get_ko(self, color: int) -> Tuple[int, int]:
        '''Get the ko position.
        Args:
            color (int): Stone color for ko
        Returns:
            Tuple[int, int]: Ko position
        '''
        cdef pair[int32_t, int32_t] ko = self.board.getKo(color)
        return (ko.first, ko.second)

    def get_color(self, pos: Tuple[int, int]) -> int:
        '''Get the color of the stone at the specified position.
        Args:
            pos (Tuple[int, int]): Position to get the stone color from
        Returns:
            int: Stone color
        '''
        return self.board.getColor(pos[0], pos[1])

    def get_colors(self, color: int) -> numpy.ndarray:
        '''Get the colors of all stones on the board.
        Args:
            color (int): Reference stone color (specifying WHITE inverts the colors)
        Returns:
            numpy.ndarray: Colors of all stones
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode='c'] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getColors(<int32_t*> &data[0], color)

        return data.reshape((height, width))

    def get_pattern(self) -> Tuple[int, ...]:
        '''Get values representing the stone arrangement.
        Returns:
            Tuple[int, ...]: Black and white bitboards split into 32-bit words
        '''
        cdef vector[uint32_t] pattern = self.board.getPattern()

        # Convert the C++ array to a Python tuple
        return tuple(pattern)

    def get_ren_size(self, pos: Tuple[int, int]) -> int:
        '''Get the size of the group at the specified position.
        Args:
            pos (Tuple[int, int]): Position to get the group size from
        Returns:
            int: Size of the group
        '''
        return self.board.getRenSize(pos[0], pos[1])

    def get_ren_space(self, pos: Tuple[int, int]) -> int:
        '''Get the number of liberties of the group at the specified position.
        Args:
            pos (Tuple[int, int]): Position to get the group liberties from
        Returns:
            int: Number of liberties of the group
        '''
        return self.board.getRenSpace(pos[0], pos[1])

    def is_shicho(self, pos: Tuple[int, int]) -> bool:
        '''Check whether the specified position is in a ladder (shicho).
        Args:
            pos (Tuple[int, int]): Position to check for ladder
        Returns:
            bool: True if the position is in a ladder
        '''
        return self.board.isShicho(pos[0], pos[1])

    def is_enabled(
        self,
        pos: Tuple[int, int],
        color: int,
        check_seki: bool,
    ) -> bool:
        '''Check whether a stone can be placed at the specified position.
        Args:
            pos (Tuple[int, int]): Position to place the stone
            color (int): Stone color
            check_seki (bool): Whether to check for seki
        Returns:
            bool: True if a stone can be placed
        '''
        return self.board.isEnabled(pos[0], pos[1], color, check_seki)

    def get_enableds(self, color: int, check_seki: bool) -> numpy.ndarray:
        '''Get whether a stone can be placed at each position on the board.
        Args:
            color (int): Stone color
            check_seki (bool): Whether to check for seki
        Returns:
            numpy.ndarray: Placement validity for each position
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode='c'] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getEnableds(<int32_t*> &data[0], color, check_seki)

        return data.reshape((height, width))

    def get_fixed_territories(self) -> numpy.ndarray:
        '''Get the list of confirmed territories.
        Returns:
            numpy.ndarray: List of secured territories
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode='c'] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getFixedTerritories(<int32_t*> &data[0])

        return data.reshape((height, width))

    def get_owners(
        self,
        numpy.ndarray[numpy.int32_t, ndim=2, mode='c'] territories,
        rule: int,
    ) -> numpy.ndarray:
        '''Get the list of owners for each position.
        Args:
            territories (numpy.ndarray): Territory data from Black's perspective
            rule (int): Scoring rule
        Returns:
            numpy.ndarray: List of owners for each coordinate
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode='c'] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getOwners(
            <int32_t*> &data[0], <const int32_t*> &territories[0, 0], rule)

        return data.reshape((height, width))

    def get_inputs(self, color: int, komi: float, rule: int, superko: bool) -> numpy.ndarray:
        '''Get the board data to be fed into the inference model.
        Args:
            color (int): Stone color to play
            komi (float): Komi points
            rule (int): Rule for determining the winner
            superko (bool): True to apply superko rule
        Returns:
            numpy.ndarray: Board input data
        '''
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode='c'] inputs = numpy.zeros(
            (MODEL_INPUT_PACK_SIZE,), dtype=numpy.int32)

        self.board.getInputs(<int32_t*> &inputs[0], color, komi, rule, superko)

        return inputs

    def copy_from(self, board: NativeBoard) -> None:
        '''
        Copy the board.
        Args:
            board (NativeBoard): Source board to copy from
        '''
        self.board.copyFrom(board.board)
