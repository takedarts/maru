from typing import List, Tuple

from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.vector cimport vector
from libcpp.pair cimport pair

include "processor.pyx"

cdef extern from "cpp/Candidate.h" namespace "deepgo":
    cdef cppclass Candidate:
        int32_t getX()
        int32_t getY()
        int32_t getColor()
        int32_t getVisits()
        int32_t getPlayouts()
        float getPolicy()
        float getValue()
        vector[pair[int32_t, int32_t]] getVariations()


cdef extern from "cpp/Player.h" namespace "deepgo":
    cdef cppclass Player:
        Player(Processor*, int32_t, int32_t, int32_t, float, int, bool, bool) except +
        void initialize()
        int32_t play(int32_t, int32_t)
        vector[Candidate] getPass() nogil
        vector[Candidate] getRandom(float) nogil
        void startEvaluation(bool, bool, int32_t, float, float)
        void waitEvaluation(int32_t, int32_t, float, bool) nogil
        vector[Candidate] getCandidates()
        int32_t getColor()
        vector[int32_t] getBoardState()


cdef class NativePlayer:
    cdef Player* player
    def __cinit__(
        self,
        processor: NativeProcessor,
        threads: int,
        width: int,
        height: int,
        komi: float,
        rule: int,
        superko: bool,
        eval_leaf_only: bool
    )->None:
        '''Initialize player object.
        Args:
            processor (NativeProcessor): Processor object
            threads (int): Number of threads
            width (int): Board width
            height (int): Board height
            komi (float): Komi value
            rule (int): Rule for determining winner
            superko (bool): True to apply superko rule
            eval_leaf_only (bool): True to evaluate only leaf nodes
        '''
        self.player = new Player(
            processor.processor, threads,
            width, height, komi, rule, superko, eval_leaf_only)

    def __dealloc__(self):
        del self.player

    def initialize(self) -> None:
        '''Reset the game state to the initial state.'''
        self.player.initialize()

    def play(self, pos: Tuple[int, int]) -> int:
        '''Place a stone at the specified coordinates.
        Args:
            pos (Tuple[int, int]): Coordinates to place the stone
        Returns:
            int: Number of captured stones
        '''
        return self.player.play(pos[0], pos[1])

    def get_pass(
        self,
    ) -> Tuple[Tuple[int, int], int, int, int, float, float, List[Tuple[int, int]]]:
        '''Get a candidate for pass.
        Returns:
            Tuple[Tuple[int, int], int, int, int, float, float, List[Tuple[int, int]]]: Candidate
        '''
        cdef vector[Candidate] candidates

        with nogil:
            candidates = self.player.getPass()

        return (
            (candidates[0].getX(), candidates[0].getY()), candidates[0].getColor(),
             candidates[0].getVisits(), candidates[0].getPlayouts(),
             candidates[0].getPolicy(), candidates[0].getValue(), candidates[0].getVariations(),
        )

    def get_random(
        self,
        temperature: float,
    ) -> Tuple[Tuple[int, int], int, float, float, float, List[Tuple[int, int]]]:
        '''Select a candidate move randomly.
        Args:
            temperature (float): Temperature
        Returns:
            Tuple[Tuple[int, int], int, float, float, float, List[Tuple[int, int]]]: Candidate
        '''
        cdef vector[Candidate] candidates

        with nogil:
            candidates = self.player.getRandom(temperature)

        return (
            (candidates[0].getX(), candidates[0].getY()), candidates[0].getColor(),
             candidates[0].getVisits(), candidates[0].getPlayouts(),
             candidates[0].getPolicy(), candidates[0].getValue(), candidates[0].getVariations(),
        )

    def start_evaluation(
        self,
        equally: bool,
        use_ucb1: bool,
        width: int,
        temperature: float,
        noise: float,
    ) -> None:
        '''Start evaluation.
        Args:
            equality (int): True to make the number of searches equal, False to use UCB1 or PUCB
            use_ucb1 (int): True to use UCB1 as the search criterion, False to use PUCB
            width (int): Search width (0 means no restriction)
            temperature (float): Temperature parameter for search
            noise (float): Strength of Gumbel noise for search
        '''
        self.player.startEvaluation(equally, use_ucb1, width, temperature, noise)

    def wait_evaluation(self, visits: int, playouts: int, timelimit: float, stop: bool) -> None:
        '''Wait until the specified number of visits and playouts is reached.
        Args:
            visits (int): Number of visits
            playouts (int): Number of playouts
            timelimit (float): Time limit (seconds)
            stop (bool): True to stop search
        '''
        cdef int32_t visits_int = visits
        cdef int32_t playouts_int = playouts
        cdef float timelimit_float = timelimit
        cdef bool stop_bool = stop

        with nogil:
            self.player.waitEvaluation(visits_int, playouts_int, timelimit_float, stop_bool)

    def get_candidates(
        self,
    ) -> List[Tuple[Tuple[int, int], int, float, float, List[Tuple[int, int]]]]:
        '''Get the list of candidate moves.
        Returns:
            List[Tuple[Tuple[int, int], int, float, float, float, List[Tuple[int, int]]]]: List of candidates
        '''
        cdef vector[Candidate] candidates

        candidates = self.player.getCandidates()

        results = [
            ((candidates[i].getX(), candidates[i].getY()), candidates[i].getColor(),
             candidates[i].getVisits(), candidates[i].getPlayouts(),
             candidates[i].getPolicy(), candidates[i].getValue(), candidates[i].getVariations(),
            ) for i in range(candidates.size())]

        return results

    def get_color(self) -> int:
        '''Get the color of the next stone to play.
        Returns:
            int: Stone color
        '''
        return self.player.getColor()

    def get_board_state(self) -> List[int]:
        '''Get the board state.
        Returns:
            List[int]: Board state
        '''
        return self.player.getBoardState()
