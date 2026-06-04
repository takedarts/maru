from typing import List, Tuple

from libc.stdint cimport int32_t
from libcpp.pair cimport pair
from libcpp.vector cimport vector

import numpy
cimport numpy

from deepgo.config import MODEL_INPUT_PACK_SIZE
from pyx.board cimport Board
from pyx.move cimport Move


cdef class NativeBoard:
    cdef Board *board

    def __cinit__(self, width: int, height: int) -> None:
        '''Create a board object.
        Args:
            width (int): Width of the board
            height (int): Height of the board
        '''
        self.board = new Board(width, height)

    def __dealloc__(self) -> None:
        '''Destroy the board object.'''
        del self.board

    def get_width(self) -> int:
        '''Get the width of the board.
        Returns:
            int: Width of the board
        '''
        return self.board.getWidth()

    def get_height(self) -> int:
        '''Get the height of the board.
        Returns:
            int: Height of the board
        '''
        return self.board.getHeight()

    def play(self, pos: Tuple[int, int], color: int) -> int:
        '''Place a stone at the specified position.
        Args:
            pos (Tuple[int, int]): Position to place the stone
            color (int): Color of the stone
        Returns:
            int: Number of captured stones (-1 if the move is invalid)
        '''
        return self.board.play(Move(pos[0], pos[1], color))

    def get_ko(self, color: int) -> Tuple[int, int]:
        '''Get the ko position.
        Args:
            color (int): Color of the stone subject to ko
        Returns:
            Tuple[int, int]: Ko position
        '''
        cdef pair[int32_t, int32_t] ko = self.board.getKo(color)
        return (ko.first, ko.second)

    def get_histories(self, color: int) -> List[Tuple[int, int]]:
        '''Get the move history for the specified color.
        Args:
            color (int): Color of the stone
        Returns:
            List[Tuple[int, int]]: Move history
        '''
        cdef vector[Move] moves = self.board.getHistories(color)
        return [(move.getX(), move.getY()) for move in moves]

    def get_color(self, pos: Tuple[int, int]) -> int:
        '''Get the color of the stone at the specified position.
        Args:
            pos (Tuple[int, int]): Position to get the stone color from
        Returns:
            int: Color of the stone
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
            color (int): Color of the stone
            check_seki (bool): Whether to check for seki
        Returns:
            bool: True if a stone can be placed
        '''
        return self.board.isEnabled(pos[0], pos[1], color, check_seki)

    def get_enableds(self, color: int, check_seki: bool) -> numpy.ndarray:
        '''Get whether a stone can be placed at each position on the board.
        Args:
            color (int): Color of the stone
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

    def get_territories(self, color: int) -> numpy.ndarray:
        '''Get the list of secured territories.
        Args:
            color (int): Reference stone color (specifying WHITE inverts the colors)
        Returns:
            numpy.ndarray: List of secured territories
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode='c'] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getTerritories(<int32_t*> &data[0], color)

        return data.reshape((height, width))

    def get_owners(self, color: int, rule: int) -> numpy.ndarray:
        '''Get the list of owners for each coordinate.
        Args:
            color (int): Reference stone color (specifying WHITE inverts the colors)
            rule (int): Scoring rule
        Returns:
            numpy.ndarray: List of owners for each coordinate
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode='c'] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getOwners(<int32_t*> &data[0], color, rule)

        return data.reshape((height, width))

    def get_patterns(self) -> List[int]:
        '''Get values representing the stone arrangement pattern.
        Returns:
            List[int]: Values representing the stone arrangement pattern
        '''
        return self.board.getPatterns()

    def get_inputs(self, color: int, komi: float, rule: int, superko: bool) -> numpy.ndarray:
        '''Get the board data to be fed into the inference model.
        Args:
            color (int): Color of the stone to play
            komi (float): Komi value
            rule (int): Scoring rule
            superko (bool): True to apply the superko rule
        Returns:
            numpy.ndarray: Board input data
        '''
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode='c'] inputs = numpy.zeros(
            (MODEL_INPUT_PACK_SIZE,), dtype=numpy.int32)

        self.board.getInputs(<int32_t*> &inputs[0], color, komi, rule, superko)

        return inputs

    def get_state(self) -> List[int]:
        '''Get the board state.
        Returns:
            List[int]: Board state
        '''
        return self.board.getState()

    def load_state(self, state: List[int]) -> None:
        '''Load the board state.
        Args:
            state (List[int]): Board state
        '''
        self.board.loadState(state)

    def copy_from(self, board: NativeBoard) -> None:
        '''Copy the board from another board.
        Args:
            board (NativeBoard): Source board to copy from
        '''
        self.board.copyFrom(board.board)
