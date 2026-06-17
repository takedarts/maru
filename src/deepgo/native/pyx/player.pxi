from typing import List, Tuple

import numpy as np
cimport numpy as np

from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.vector cimport vector
from pyx.candidate cimport Candidate
from pyx.move cimport Move
from pyx.player cimport Player

from deepgo.config import MODEL_SIZE

cdef class NativePlayer:
    cdef Player* player
    cdef int width
    cdef int height

    def __cinit__(
        self,
        processor: NativeInferenceProcessor,  # type: ignore
        threads: int,
        max_visits: int,
        width: int,
        height: int,
        komi: float,
        rule: int,
        superko: bool,
        pucb_constant_init: float,
        pucb_constant_base: float,
    )->None:
        '''Initialize the player object.
        Args:
            processor (NativeInferenceProcessor): Inference processor object
            threads (int): Number of threads
            max_visits (int): Maximum number of visits
            width (int): Width of the board
            height (int): Height of the board
            komi (float): Komi value
            rule (int): Scoring rule
            superko (bool): True to apply the superko rule
            pucb_constant_init (float): Initial value of the constant multiplied by the PUCB confidence upper bound
            pucb_constant_base (float): Base value for the change in the constant multiplied by the PUCB confidence upper bound
        '''
        self.player = new Player(
            processor.processor, threads, max_visits,
            width, height, komi, rule, superko,
            pucb_constant_init, pucb_constant_base)
        self.width = width
        self.height = height

    def __dealloc__(self):
        del self.player

    def initialize(self) -> None:
        '''Reset the game state to the initial state.'''
        self.player.initialize()

    def play(self, pos: Tuple[int, int], color: int) -> int:
        '''Play a stone at the specified coordinate.
        Args:
            pos (Tuple[int, int]): Coordinate to play the stone
            color (int): Color of the stone
        Returns:
            int: Number of captured stones
        '''
        cdef Move move = Move(pos[0], pos[1], color)
        cdef int32_t captured

        with nogil:
            captured = self.player.play(move)

        return captured

    def get_pass(
        self,
    ) -> Tuple[
            Tuple[int, int], int, int, int, float, float,
            List[Tuple[Tuple[int, int], int]], np.ndarray]:
        '''Get the pass candidate move.
        Returns:
            Tuple[Tuple[int, int], int, int, int, float, float,
                  List[Tuple[Tuple[int, int], int]], np.ndarray]: Candidate move
        '''
        cdef vector[Candidate] candidates
        cdef Candidate candidate
        cdef np.ndarray[np.float32_t, ndim=1, mode='c'] territories

        with nogil:
            candidate = self.player.getPass()

        territories = np.zeros((3 * MODEL_SIZE * MODEL_SIZE,), dtype=np.float32)
        candidate.getTerritories(<float*> &territories[0])
        x_begin = (MODEL_SIZE - self.width) // 2
        x_end = x_begin + self.width
        y_begin = (MODEL_SIZE - self.height) // 2
        y_end = y_begin + self.height

        return (
            (candidate.getMove().getX(), candidate.getMove().getY()),
             candidate.getMove().getColor(),
             candidate.getVisits(),
             candidate.getPlayouts(),
             candidate.getPolicy(),
             candidate.getValue(),
             [((variation.getX(), variation.getY()), variation.getColor())
              for variation in candidate.getVariations()],
             territories.reshape((3, MODEL_SIZE, MODEL_SIZE))[:, y_begin:y_end, x_begin:x_end],
        )

    def start_evaluation(
        self,
        equally: bool,
        candidate_width: int,
        temperature: float,
        noise: float,
    ) -> None:
        '''Start the evaluation.
        Args:
            equally (bool): True to equalize the number of search visits
            candidate_width (int): Search width for candidate moves
            temperature (float): Temperature parameter for search
            noise (float): Strength of Gumbel noise for search
        '''
        cdef bool equally_bool = equally
        cdef int32_t candidate_width_int = candidate_width
        cdef float temperature_float = temperature
        cdef float noise_float = noise

        with nogil:
            self.player.startEvaluation(
                equally_bool, candidate_width_int, temperature_float, noise_float)

    def wait_evaluation(self, visits: int, playouts: int, timelimit: float, stop: bool) -> None:
        '''Wait until the specified number of visits and playouts is reached.
        Args:
            visits (int): Number of visits
            playouts (int): Number of playouts
            timelimit (float): Time limit (seconds)
            stop (bool): True to stop the search
        '''
        cdef int32_t visits_int = visits
        cdef int32_t playouts_int = playouts
        cdef float timelimit_float = timelimit
        cdef bool stop_bool = stop

        with nogil:
            self.player.waitEvaluation(visits_int, playouts_int, timelimit_float, stop_bool)

    def get_candidates(
        self,
    ) -> List[Tuple[
            Tuple[int, int], int, int, int, float, float,
            List[Tuple[Tuple[int, int], int]], np.ndarray]]:
        '''Get the list of candidate moves.
        Returns:
            List[Tuple[Tuple[int, int], int, int, int, float, float,
                 List[Tuple[Tuple[int, int], int]], np.ndarray]]: Candidate moves
        '''
        cdef vector[Candidate] candidates
        cdef np.ndarray[np.float32_t, ndim=1, mode='c'] territories

        with nogil:
            candidates = self.player.getCandidates()

        x_begin = (MODEL_SIZE - self.width) // 2
        x_end = x_begin + self.width
        y_begin = (MODEL_SIZE - self.height) // 2
        y_end = y_begin + self.height

        results = []

        for candidate in candidates:
            territories = np.zeros((3 * MODEL_SIZE * MODEL_SIZE,), dtype=np.float32)
            candidate.getTerritories(<float*> &territories[0])

            results.append((
                (candidate.getMove().getX(), candidate.getMove().getY()),
                candidate.getMove().getColor(),
                candidate.getVisits(),
                candidate.getPlayouts(),
                candidate.getPolicy(),
                candidate.getValue(),
                [((variation.getX(), variation.getY()), variation.getColor())
                 for variation in candidate.getVariations()],
                territories.reshape((3, MODEL_SIZE, MODEL_SIZE))[:, y_begin:y_end, x_begin:x_end],
            ))

        return results

    def get_color(self) -> int:
        '''Get the color of the next stone to play.
        Returns:
            int: Color of the stone
        '''
        return self.player.getColor()

    def get_board_state(self) -> List[int]:
        '''Get the board state.
        Returns:
            List[int]: Board state
        '''
        return self.player.getBoardState()

    def to_string(self) -> str:
        '''Get the string representation of the search tree.
        Returns:
            str: String representation
        '''
        return self.player.toString().decode('utf-8')
