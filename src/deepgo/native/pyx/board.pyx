from typing import List, Tuple

from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.pair cimport pair
from libcpp.vector cimport vector

import numpy
cimport numpy

from deepgo.config import MODEL_INPUT_SIZE


cdef extern from "cpp/Board.h" namespace "deepgo":
    cdef cppclass Board:
        Board(int32_t, int32_t) except +
        int32_t getWidth()
        int32_t getHeight()
        int32_t play(int32_t, int32_t, int32_t)
        pair[int32_t, int32_t] getKo(int32_t)
        vector[pair[int32_t, int32_t]] getHistories(int32_t)
        int32_t getColor(int32_t, int32_t)
        void getColors(int32_t*, int32_t)
        int32_t getRenSize(int32_t, int32_t)
        int32_t getRenSpace(int32_t, int32_t)
        bool isShicho(int32_t, int32_t)
        bool isEnabled(int32_t, int32_t, int32_t, bool)
        void getEnableds(int32_t*, int32_t, bool)
        void getTerritories(int32_t*, int32_t)
        void getOwners(int32_t*, int32_t, int32_t)
        vector[int32_t] getPatterns()
        void getInputs(float*, int32_t, float, int32_t, bool)
        vector[int32_t] getState()
        void loadState(vector[int32_t])
        void copyFrom(Board*)


cdef class NativeBoard:
    cdef Board *board

    def __cinit__(self, width: int, height: int) -> None:
        '''Create a board object.
        Args:
            width (int): Board width
            height (int): Board height
        '''
        self.board = new Board(width, height)

    def __dealloc__(self) -> None:
        '''Destroy the board object.'''
        del self.board

    def get_width(self) -> int:
        '''Get the board width.
        Returns:
            int: Board width
        '''
        return self.board.getWidth()

    def get_height(self) -> int:
        '''Get the board height.
        Returns:
            int: Board height
        '''
        return self.board.getHeight()

    def play(self, pos: Tuple[int, int], color: int) -> int:
        '''Place a stone at the specified position.
        Args:
            pos (Tuple[int, int]): Position to place the stone
            color (int): Stone color
        Returns:
            int: Number of captured stones (-1 if not allowed)
        '''
        return self.board.play(pos[0], pos[1], color)

    def get_ko(self, color: int) -> Tuple[int, int]:
        '''Get the ko position.
        Args:
            color (int): Color of the stone for ko
        Returns:
            Tuple[int, int]: Ko position
        '''
        cdef pair[int32_t, int32_t] ko = self.board.getKo(color)
        return (ko.first, ko.second)

    def get_histories(self, color: int) -> List[Tuple[int, int]]:
        '''Get the move history for the specified color.
        Args:
            color (int): Stone color
        Returns:
            List[Tuple[int, int]]: Move history
        '''
        cdef vector[pair[int32_t, int32_t]] moves = self.board.getHistories(color)
        return [(move.first, move.second) for move in moves]

    def get_color(self, pos: Tuple[int, int]) -> int:
        '''Get the color of the stone at the specified position.
        Args:
            pos (Tuple[int, int]): Position to get the stone color
        Returns:
            int: Stone color
        '''
        return self.board.getColor(pos[0], pos[1])

    def get_colors(self, color: int) -> numpy.ndarray:
        '''Get the color of all stones.
        Args:
            color (int): Reference stone color (specify WHITE to invert colors)
        Returns:
            numpy.ndarray: All stone colors
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode="c"] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getColors(<int32_t*> &data[0], color)

        return data.reshape((height, width))

    def get_ren_size(self, pos: Tuple[int, int]) -> int:
        '''Get the size of the group at the specified position.
        Args:
            pos (Tuple[int, int]): Position to get the group size
        Returns:
            int: Group size
        '''
        return self.board.getRenSize(pos[0], pos[1])

    def get_ren_space(self, pos: Tuple[int, int]) -> int:
        '''Get the number of liberties of the group at the specified position.
        Args:
            pos (Tuple[int, int]): Position to get the group liberties
        Returns:
            int: Number of liberties
        '''
        return self.board.getRenSpace(pos[0], pos[1])

    def is_shicho(self, pos: Tuple[int, int]) -> bool:
        '''Check if the specified position is a ladder (shicho).
        Args:
            pos (Tuple[int, int]): Position to check for ladder
        Returns:
            bool: True if ladder
        '''
        return self.board.isShicho(pos[0], pos[1])

    def is_enabled(
        self,
        pos: Tuple[int, int],
        color: int,
        check_seki: bool,
    ) -> bool:
        '''Check if a stone can be placed at the specified position.
        Args:
            pos (Tuple[int, int]): Position to place the stone
            color (int): Stone color
            check_seki (bool): Whether to check for seki
        Returns:
            bool: True if stone can be placed
        '''
        return self.board.isEnabled(pos[0], pos[1], color, check_seki)

    def get_enableds(self, color: int, check_seki: bool) -> numpy.ndarray:
        '''Get whether stones can be placed at all positions.
        Args:
            color (int): Stone color
            check_seki (bool): Whether to check for seki
        Returns:
            numpy.ndarray: Whether stones can be placed at all positions
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode="c"] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getEnableds(<int32_t*> &data[0], color, check_seki)

        return data.reshape((height, width))

    def get_territories(self, color: int) -> numpy.ndarray:
        '''Get the list of confirmed territories.
        Args:
            color (int): Reference stone color (specify WHITE to invert colors)
        Returns:
            numpy.ndarray: List of confirmed territories
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode="c"] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getTerritories(<int32_t*> &data[0], color)

        return data.reshape((height, width))

    def get_owners(self, color: int, rule: int) -> numpy.ndarray:
        '''Get the list of owners for each coordinate.
        Args:
            color (int): Reference stone color (specify WHITE to invert colors)
            rule (int): Calculation rule
        Returns:
            numpy.ndarray: List of owners
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode="c"] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getOwners(<int32_t*> &data[0], color, rule)

        return data.reshape((height, width))

    def get_patterns(self) -> List[int]:
        '''Get values representing the arrangement of stones.
        Returns:
            List[int]: Values representing the arrangement of stones
        '''
        return self.board.getPatterns()

    def get_inputs(self, color: int, komi: float, rule: int, superko: bool) -> numpy.ndarray:
        '''Get board data to input to the inference model.
        Args:
            color (int): Stone color to play
            komi (float): Komi value
            rule (int): Rule for determining winner
            superko (bool): True to apply superko rule
        Returns:
            numpy.ndarray: Board data
        '''
        cdef numpy.ndarray[numpy.float32_t, ndim=1, mode="c"] inputs = numpy.zeros(
            (MODEL_INPUT_SIZE,), dtype=numpy.float32)

        self.board.getInputs(<float*> &inputs[0], color, komi, rule, superko)

        return inputs

    def get_state(self) -> List[int]:
        '''Get the board state.
        Returns:
            List[int]: Board state
        '''
        return self.board.getState()

    def load_state(self, state: List[int]) -> None:
        '''Set the board state.
        Args:
            state (List[int]): Board state
        '''
        self.board.loadState(state)

    def copy_from(self, board: NativeBoard) -> None:
        '''
        Copy the board.
        Args:
            board (NativeBoard): Source board to copy from
        '''
        self.board.copyFrom(board.board)
