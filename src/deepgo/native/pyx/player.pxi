from typing import List, Tuple

from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.vector cimport vector
from pyx.candidate cimport Candidate
from pyx.player cimport Player


cdef class NativePlayer:
    cdef Player* player
    def __cinit__(
        self,
        processor: NativeProcessor,  # type: ignore
        threads: int,
        cache_size: int,
        width: int,
        height: int,
        komi: float,
        rule: int,
        superko: bool,
        ucb_constant: float,
        pucb_constant_init: float,
        pucb_constant_base: float,
        eval_leaf_only: bool,
        max_visits: int,
    )->None:
        '''Initialize player object.
        Args:
            processor (NativeProcessor): Processor object
            threads (int): Number of threads
            cache_size (int): Cache size for evaluation results
            width (int): Board width
            height (int): Board height
            komi (float): Komi value
            rule (int): Rule for determining winner
            superko (bool): True to apply superko rule
            ucb_constant (float): Constant multiplied to UCB upper confidence bound
            pucb_constant_init (float): Initial value applied to PUCB upper confidence bound
            pucb_constant_base (float): Base value applied to PUCB upper confidence bound
            eval_leaf_only (bool): True to evaluate only leaf nodes
            max_visits (int): Maximum number of visits
        '''
        self.player = new Player(
            processor.processor, threads, cache_size,
            width, height, komi, rule, superko,
            ucb_constant, pucb_constant_init, pucb_constant_base,
            eval_leaf_only, max_visits)

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
    ) -> Tuple[Tuple[int, int], int, int, int, float, float, float, List[Tuple[int, int]]]:
        '''Get a candidate for pass.
        Returns:
            Tuple[Tuple[int, int], int, int, int, float, float, float, List[Tuple[int, int]]]: Candidate
        '''
        cdef vector[Candidate] candidates

        with nogil:
            candidates = self.player.getPass()

        return (
            (candidates[0].getX(), candidates[0].getY()), candidates[0].getColor(),
             candidates[0].getVisits(), candidates[0].getPlayouts(),
             candidates[0].getPolicy(), candidates[0].getValue(), candidates[0].getMinimax(),
             candidates[0].getVariations(),
        )

    def get_random(
        self,
        temperature: float,
    ) -> Tuple[Tuple[int, int], int, int, int, float, float, float, List[Tuple[int, int]]]:
        '''Select a candidate move randomly.
        Args:
            temperature (float): Temperature
        Returns:
            Tuple[Tuple[int, int], int, int, int, float, float, float, List[Tuple[int, int]]]: Candidate
        '''
        cdef vector[Candidate] candidates

        with nogil:
            candidates = self.player.getRandom(temperature)

        return (
            (candidates[0].getX(), candidates[0].getY()), candidates[0].getColor(),
             candidates[0].getVisits(), candidates[0].getPlayouts(),
             candidates[0].getPolicy(), candidates[0].getValue(), candidates[0].getMinimax(),
             candidates[0].getVariations(),
        )

    def start_evaluation(
        self,
        equally: bool,
        algorithm: int,
        width: int,
        temperature: float,
        noise: float,
    ) -> None:
        '''Start evaluation.
        Args:
            equality (int): True to make the number of searches equal, False to use UCB or PUCB
            algorithm (int): Search algorithm
            width (int): Search width (0 means no restriction)
            temperature (float): Temperature parameter for search
            noise (float): Strength of Gumbel noise for search
        '''
        self.player.startEvaluation(equally, algorithm, width, temperature, noise)

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
    ) -> List[Tuple[Tuple[int, int], int, int, int, float, float, float, List[Tuple[int, int]]]]:
        '''Get the list of candidate moves.
        Returns:
            List[Tuple[Tuple[int, int], int, int, int, float, float, float, List[Tuple[int, int]]]]: List of candidates
        '''
        cdef vector[Candidate] candidates

        candidates = self.player.getCandidates()

        results = [
            ((candidates[i].getX(), candidates[i].getY()), candidates[i].getColor(),
             candidates[i].getVisits(), candidates[i].getPlayouts(),
             candidates[i].getPolicy(), candidates[i].getValue(), candidates[i].getMinimax(),
             candidates[i].getVariations(),
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

    def get_debug_info(self) -> str:
        '''Output debug information of the search tree.
        Returns:
            str: Debug information
        '''
        return self.player.getDebugInfo().decode('utf-8')
