import logging
import math
import random
from typing import List, Tuple

import numpy as np
from deepgo.exception import GoException

from .board import (Board, get_color_name, get_handicap_positions,
                    is_valid_position)
from .config import (COLOR_BLACK, COLOR_EMPTY, COLOR_WHITE, DEFAULT_KOMI,
                     DEFAULT_MAX_VISITS, DEFAULT_PUCB_CONSTANT_BASE,
                     DEFAULT_PUCB_CONSTANT_INIT, DEFAULT_PUCB_MIN_VISITS_RATE,
                     DEFAULT_SIZE, MOVE_PASS, RULE_CH)
from .native import NativePlayer
from .processor import Processor

LOGGER = logging.getLogger(__name__)


class Candidate(object):
    '''Search candidate with probability, value, score, and principal variation.
    '''
    def __init__(
        self,
        pos: Tuple[int, int],
        color: int,
        visits: int,
        policy: float,
        value: float,
        score: float,
        variations: List[Tuple[Tuple[int, int], int]],
        territories: np.ndarray,
    ) -> None:
        '''Initialize candidate move object.
        Args:
            pos (Tuple[int, int]): Coordinates to place stone
            color (int): Stone color
            visits (int): Number of visits
            policy (float): Predicted move probability
            value (float): Predicted win rate
            score (float): Predicted score difference from Black's perspective
            variations (List[Tuple[Tuple[int, int], int]]): Predicted sequence
            territories (np.ndarray): Predicted territory
        Returns:
            None: No return value.
        '''
        self.pos = pos
        self.color = color
        self.visits = visits
        self.policy = policy
        self.value = value
        self.score = score
        self.variations = variations
        self.territories = territories

        if math.isnan(self.policy):
            raise GoException('Policy is NaN')

        if math.isnan(self.value):
            raise GoException('Value is NaN')

        if math.isnan(self.score):
            raise GoException('Score is NaN')

        self.win_chance = self.value * self.color * 0.5 + 0.5

        self.value_lcb = value - color * 1.96 * 0.5 / (visits + 1)**0.5
        self.win_chance_lcb = self.value_lcb * self.color * 0.5 + 0.5

    def __str__(self) -> str:
        '''Return a readable string representation.
        Returns:
            str: Result of the operation.
        '''
        return (
            f'Candidate(pos={self.pos}, color={get_color_name(self.color)},'
            f' visits={self.visits}, policy={self.policy:.2f},'
            f' value={self.value: .3f}, value_lcb={self.value_lcb: .3f},'
            f' win_chance={self.win_chance: .3f}, win_chance_lcb={self.win_chance_lcb: .3f},'
            f' score={self.score: .2f}, variations={self.variations})')

    def __repr__(self) -> str:
        '''Return the diagnostic string representation.
        Returns:
            str: Result of the operation.
        '''
        return str(self)


class Player(object):
    '''Manage the native search tree and play moves.
    '''
    def __init__(
        self,
        processor: Processor,
        threads: int = 1,
        width: int = DEFAULT_SIZE,
        height: int = DEFAULT_SIZE,
        komi: float = DEFAULT_KOMI,
        rule: int = RULE_CH,
        superko: bool = False,
        pucb_constant_init: float = DEFAULT_PUCB_CONSTANT_INIT,
        pucb_constant_base: float = DEFAULT_PUCB_CONSTANT_BASE,
        pucb_min_visits_rate: float = DEFAULT_PUCB_MIN_VISITS_RATE,
        max_visits: int = DEFAULT_MAX_VISITS,
    ) -> None:
        '''Initialize player object.
        Args:
            processor (Processor): Computation management object
            threads (int): Number of threads to use
            width (int): Board width
            height (int): Board height
            komi (float): Komi value
            rule (int): Rule for determining the winner
            superko (bool): True to apply superko rule
            pucb_constant_init (float): Initial value applied to PUCB upper confidence bound
            pucb_constant_base (float): Base value applied to PUCB upper confidence bound
            pucb_min_visits_rate (float): Minimum child visit ratio prioritized by PUCB
            max_visits (int): Maximum number of visits
        Returns:
            None: No return value.
        '''
        self.native = NativePlayer(
            processor=processor.native, threads=threads, max_visits=max_visits,
            width=width, height=height, komi=komi, rule=rule, superko=superko,
            pucb_constant_init=pucb_constant_init,
            pucb_constant_base=pucb_constant_base,
            pucb_min_visits_rate=pucb_min_visits_rate)
        self.processor = processor
        self.width = width
        self.height = height
        self.turn = 0

    def initialize(self) -> None:
        '''Initialize the board.
        Returns:
            None: No return value.
        '''
        self.native.initialize()
        self.turn = 0

    def set_handicap(self, handicap: int) -> None:
        '''Set handicap stones.
        Args:
            handicap (int): Number of handicap stones
        Returns:
            None: No return value.
        '''
        for pos in get_handicap_positions(self.width, self.height, handicap):
            self.native.play(pos, COLOR_BLACK)

    def is_valid_position(self, pos: Tuple[int, int]) -> bool:
        '''Return whether the specified coordinates are valid.
        Args:
            pos (Tuple[int, int]): Coordinates
        Returns:
            bool: True if valid coordinates
        '''
        return is_valid_position(pos, self.width, self.height)

    def get_cleanup_position(self, color: int) -> Tuple[int, int]:
        '''Return the coordinates to remove dead stones.
        Args:
            color (int): Color of the stone to place
        Returns:
            Tuple[int, int]: Coordinates
        '''
        board = self.get_board()
        colors = board.get_colors(color)
        # Convert settled territories from Black's perspective to the playing color's perspective
        territories = board.get_fixed_territories() * color
        enableds = board.get_enableds(color)

        territories_black = (territories == COLOR_BLACK)
        colors_white = (colors == COLOR_WHITE)
        deads = (territories_black & colors_white).astype(np.int32)

        targets = np.zeros_like(deads)
        targets[1:, :] += deads[:-1, :]
        targets[:-1, :] += deads[1:, :]
        targets[:, 1:] += deads[:, :-1]
        targets[:, :-1] += deads[:, 1:]

        positions = (targets != 0) & enableds
        ys, xs = np.where(positions)

        if len(xs) == 0:
            return MOVE_PASS

        return xs[0], ys[0]

    def play(self, pos: Tuple[int, int], color: int | None = None) -> None:
        '''Place a stone.
        Args:
            pos (Tuple[int, int]): Coordinates to place stone
            color (int | None): Stone color (if not specified, use the current turn's color)
        Returns:
            None: No return value.
        '''
        # If the stone color is not specified, use the current turn's color
        if color is None:
            color = self.native.get_color()

        # Play the move
        self.native.play(pos, color)
        self.turn += 1

    def get_pass_candidate(self) -> Candidate:
        '''Return a candidate for pass.
        Returns:
            Candidate: Pass candidate
        '''
        return Candidate(*self.native.get_pass_candidate())

    def get_random_candidate(
        self,
        width: int = 16,
        timelimit: float = 120.0,
        temperature: float = 1.0,
        noise: float = 0.0,
        delta: float = 0.1,
        ponder: bool = False,
    ) -> Candidate:
        '''Return a random move.
        Args:
            width (int): Number of candidate moves
            timelimit (float): Time limit in seconds
            temperature (float): Temperature parameter
            noise (float): Strength of Gumbel noise for search
            delta (float): Maximum acceptable win rate decrease
            ponder (bool): True to continue searching
        Returns:
            Candidate: Candidate move
        '''
        # Use 1.0 for search temperature and the requested temperature for final sampling
        self.native.start_evaluation(True, width, 1.0, noise)
        self.native.wait_evaluation(width + 1, timelimit, not ponder)

        # Create a list of candidate moves
        candidates = [Candidate(*c) for c in self.native.get_candidates()]

        # Keep all searched candidates for log statistics
        evaluated_candidates = candidates

        # Get the maximum expected win rate
        max_win_chance = max(c.win_chance for c in candidates)

        # Exclude candidate moves with expected win rate less than max_win_chance - delta
        candidates = [
            c for c in candidates if c.win_chance >= max_win_chance - delta]

        # Convert policy values to selection probabilities
        probs = [c.policy**(1 / max(temperature, 1e-3)) for c in candidates]

        # Return a randomly selected candidate move
        candidate = random.choices(candidates, weights=probs, k=1)[0]

        # Log the search results and the selected candidate
        if LOGGER.isEnabledFor(logging.DEBUG):
            LOGGER.debug(
                'Evaluation: %d visits (batch fill rate=%.2f, cache hit rate=%.2f)',
                sum(c.visits for c in evaluated_candidates),
                self.processor.get_batch_fill_rate(),
                self.processor.get_cache_hit_rate())
            LOGGER.debug(candidate)

        # Return the randomly selected candidate
        return candidate

    def evaluate(
        self,
        visits: int,
        timelimit: float = 120.0,
        equally: bool = False,
        criterion: str = 'value',
        width: int | None = None,
        temperature: float = 1.0,
        noise: float = 0.0,
        ponder: bool = False,
    ) -> List[Candidate]:
        '''Evaluate the board.
        Args:
            visits (int): Target number of visits
            timelimit (float): Time limit in seconds
            equally (bool): True to make the number of searches equal, False to use UCB or PUCB
            criterion (str): Candidate priority criterion ('value' or 'visits')
            width (int): Search width (number of candidate moves to search; 0 for auto adjustment)
            temperature (float): Temperature parameter for search
            noise (float): Strength of Gumbel noise for search
            ponder (bool): True to continue searching
        Returns:
            List[Candidate]: List of candidate moves
        '''
        width = width if width is not None else 0

        # Evaluate the board
        self.native.start_evaluation(equally, width, temperature, noise)
        self.native.wait_evaluation(visits, timelimit, not ponder)

        # Create a list of candidate moves
        candidates = [Candidate(*c) for c in self.native.get_candidates()]

        # Add a pass candidate when no candidates are available
        if len(candidates) == 0:
            candidates.append(self.get_pass_candidate())

        # Sort candidates
        if criterion == 'visits':
            candidates.sort(key=lambda cand: cand.visits, reverse=True)
        else:
            candidates.sort(key=lambda cand: cand.win_chance_lcb, reverse=True)

        # Output log
        if LOGGER.isEnabledFor(logging.DEBUG):
            LOGGER.debug(
                'Evaluation: %d visits (batch fill rate=%.2f, cache hit rate=%.2f)',
                sum(c.visits for c in candidates),
                self.processor.get_batch_fill_rate(),
                self.processor.get_cache_hit_rate())
            for candidate in candidates:
                LOGGER.debug(candidate)

        # Return the list of candidates
        return candidates

    def stop_evaluation(self) -> None:
        '''Stop if pondering.
        Returns:
            None: No return value.
        '''
        self.native.wait_evaluation(0, 0.0, True)

    def get_predicted_territories(self) -> np.ndarray:
        '''Predict the final territories of the current board.
        Returns:
            np.ndarray: Predicted territories from Black's perspective
        '''
        # Get the current board's settled territories before computing territories in C++
        board_territories = self.get_board().get_fixed_territories()

        # Get predicted territories from the C++ root node
        predicted_territories = self.native.get_predicted_territories()

        # Apply the board's settled territories to the predictions
        fixed_positions = board_territories != COLOR_EMPTY
        predicted_territories[fixed_positions] = board_territories[fixed_positions]

        return predicted_territories

    def get_color(self) -> int:
        '''Return the color of the next stone to be placed.
        Returns:
            int: Stone color
        '''
        return self.native.get_color()

    def get_board(self) -> Board:
        '''Return board data.
        Returns:
            Board: Board data
        '''
        board = Board(self.width, self.height)
        self.native.copy_board_to(board.native)
        return board

    def get_captured(self, color: int) -> int:
        '''Return the number of captured stones of the specified color.
        Args:
            color (int): Stone color
        Returns:
            int: Number of captured stones
        '''
        return self.native.get_captured(color)

    def get_predicted_score(self) -> float:
        '''Get the predicted score difference of the current board.
        Returns:
            float: Predicted score difference from Black's perspective
        '''
        return self.native.get_predicted_score()

    def __str__(self) -> str:
        '''Get the string representation of the search tree.
        Returns:
            str: String representation
        '''
        return self.native.to_string()
