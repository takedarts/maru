from typing import List, Tuple

import numpy as np

cimport numpy as np
from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.vector cimport vector
from pyx.candidate cimport Candidate
from pyx.move cimport Move
from pyx.player cimport Player

from deepgo.config import MODEL_BOARD_SIZE, MODEL_TERRITORY_NUM


cdef class NativePlayer:
    '''Expose native MCTS search to Python.'''
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
        pucb_min_visits_rate: float,
    )->None:
        '''Initialize the player object.
        Args:
            processor (NativeInferenceProcessor): Inference processor object
            threads (int): Number of threads
            max_visits (int): Maximum number of visits
            width (int): Board width
            height (int): Board height
            komi (float): Komi points
            rule (int): Rule for determining the winner
            superko (bool): True to apply superko rule
            pucb_constant_init (float): Initial value applied to PUCB upper confidence bound
            pucb_constant_base (float): Base value applied to PUCB upper confidence bound
            pucb_min_visits_rate (float): Minimum child visit ratio prioritized by PUCB
        '''
        self.player = new Player(
            processor.processor, threads, max_visits,
            width, height, komi, rule, superko,
            pucb_constant_init, pucb_constant_base, pucb_min_visits_rate)
        self.width = width
        self.height = height

    def __dealloc__(self) -> None:
        '''Release the native object; return None.'''
        del self.player

    def initialize(self) -> None:
        '''Reset the game state to the initial state.'''
        self.player.initialize()

    def play(self, pos: Tuple[int, int], color: int) -> None:
        '''Play a stone at the specified coordinate.
        Args:
            pos (Tuple[int, int]): Coordinate to play the stone
            color (int): Stone color
        '''
        cdef Move move = Move(pos[0], pos[1], color)

        with nogil:
            self.player.play(move)

    def get_captured(self, color: int) -> int:
        '''Get the number of captured stones of the specified color.
        Args:
            color (int): Stone color
        Returns:
            int: Number of captured stones
        '''
        return self.player.getCaptured(color)

    def get_pass_candidate(
        self,
    ) -> Tuple[
            Tuple[int, int], int, int, float, float, float,
            List[Tuple[Tuple[int, int], int]], np.ndarray]:
        '''Get the pass candidate move.
        Returns:
            Tuple[Tuple[int, int], int, int, float, float, float,
                  List[Tuple[Tuple[int, int], int]], np.ndarray]: Candidate move
        '''
        cdef vector[Candidate] candidates
        cdef Candidate candidate
        cdef np.ndarray[np.float32_t, ndim=1, mode='c'] territories

        with nogil:
            candidate = self.player.getPassCandidate()

        territories = np.zeros(
            (MODEL_TERRITORY_NUM * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE,), dtype=np.float32)
        candidate.getTerritories(<float*> &territories[0])
        x_begin = (MODEL_BOARD_SIZE - self.width) // 2
        x_end = x_begin + self.width
        y_begin = (MODEL_BOARD_SIZE - self.height) // 2
        y_end = y_begin + self.height

        return (
            (candidate.getMove().getX(), candidate.getMove().getY()),
             candidate.getMove().getColor(),
             candidate.getVisits(),
             candidate.getPolicy(),
             candidate.getValue(),
             candidate.getScore(),
             [((variation.getX(), variation.getY()), variation.getColor())
              for variation in candidate.getVariations()],
             territories.reshape(
                (MODEL_TERRITORY_NUM, MODEL_BOARD_SIZE, MODEL_BOARD_SIZE)
             )[:, y_begin:y_end, x_begin:x_end],
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

    def wait_evaluation(
        self,
        visits: int,
        timelimit: float,
        stop: bool,
    ) -> None:
        '''Wait until the specified search conditions are satisfied.
        Args:
            visits (int): Number of visits
            timelimit (float): Time limit (seconds)
            stop (bool): True to stop the search
        '''
        cdef int32_t visits_int = visits
        cdef float timelimit_float = timelimit
        cdef bool stop_bool = stop

        with nogil:
            self.player.waitEvaluation(
                visits_int, timelimit_float, stop_bool)

    def get_candidates(
        self,
    ) -> List[Tuple[
            Tuple[int, int], int, int, float, float, float,
            List[Tuple[Tuple[int, int], int]], np.ndarray]]:
        '''Get the list of candidate moves.
        Returns:
            List[Tuple[Tuple[int, int], int, int, float, float, float,
                 List[Tuple[Tuple[int, int], int]], np.ndarray]]: Candidate moves
        '''
        cdef vector[Candidate] candidates
        cdef np.ndarray[np.float32_t, ndim=1, mode='c'] territories

        with nogil:
            candidates = self.player.getCandidates()

        x_begin = (MODEL_BOARD_SIZE - self.width) // 2
        x_end = x_begin + self.width
        y_begin = (MODEL_BOARD_SIZE - self.height) // 2
        y_end = y_begin + self.height

        results = []

        for candidate in candidates:
            territories = np.zeros(
                (MODEL_TERRITORY_NUM * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE,), dtype=np.float32)
            candidate.getTerritories(<float*> &territories[0])

            results.append((
                (candidate.getMove().getX(), candidate.getMove().getY()),
                candidate.getMove().getColor(),
                candidate.getVisits(),
                candidate.getPolicy(),
                candidate.getValue(),
                candidate.getScore(),
                [((variation.getX(), variation.getY()), variation.getColor())
                 for variation in candidate.getVariations()],
                territories.reshape(
                    (MODEL_TERRITORY_NUM, MODEL_BOARD_SIZE, MODEL_BOARD_SIZE)
                )[:, y_begin:y_end, x_begin:x_end],
            ))

        return results

    def get_color(self) -> int:
        '''Get the color of the next stone to play.
        Returns:
            int: Stone color
        '''
        return self.player.getColor()

    def get_predicted_territories(self) -> np.ndarray:
        '''Get the root node's predicted territories.
        Returns:
            np.ndarray: Predicted territories from Black's perspective
        '''
        cdef np.ndarray[np.float32_t, ndim=1, mode='c'] territories

        # Create an array holding predicted territories for the full model board
        territories = np.zeros(
            (MODEL_TERRITORY_NUM * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE,), dtype=np.float32)

        # Get the root node's predicted territories from the C++ Player object
        with nogil:
            self.player.getPredictedTerritories(<float*> &territories[0])

        # Crop to the actual board dimensions
        x_begin = (MODEL_BOARD_SIZE - self.width) // 2
        x_end = x_begin + self.width
        y_begin = (MODEL_BOARD_SIZE - self.height) // 2
        y_end = y_begin + self.height
        shaped_territories = territories.reshape(
            (MODEL_TERRITORY_NUM, MODEL_BOARD_SIZE, MODEL_BOARD_SIZE)
        )[:, y_begin:y_end, x_begin:x_end]

        # Convert the most probable class into black, neutral, and white territory values
        return shaped_territories.argmax(axis=0) - 1

    def get_predicted_score(self) -> float:
        '''Get the root node's predicted score difference.
        Returns:
            float: Predicted score difference from Black's perspective
        '''
        cdef float score

        with nogil:
            score = self.player.getPredictedScore()

        return score

    def copy_board_to(self, NativeBoard board) -> None:
        '''Copy the board state to the specified board object.
        Args:
            board (NativeBoard): Destination board
        '''
        self.player.copyBoardTo(board.board)

    def to_string(self) -> str:
        '''Get the string representation of the search tree.
        Returns:
            str: String representation
        '''
        return self.player.toString().decode('utf-8')
