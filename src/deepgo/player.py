import logging
from typing import List, Set, Tuple

import numpy as np

from .board import (Board, get_color_name, get_handicap_positions,
                    get_opposite_color, is_valid_position)
from .config import (BLACK, DEFAULT_KOMI, DEFAULT_SIZE, EMPTY, MODEL_SIZE,
                     PASS, RULE_CH, RULE_COM, RULE_JP, WHITE)
from .native import NativePlayer
from .processor import Processor

LOGGER = logging.getLogger(__name__)


def get_area_color(
    territories: np.ndarray,
    position: Tuple[int, int],
) -> Tuple[int, List[Tuple[int, int]]]:
    '''Get the color of an area.
    Args:
        territories (np.ndarray): Area data
        position (Tuple[int, int]): Position to check
    Returns:
        Tuple[int, List[Tuple[int, int]]]: Area color and list of positions
    '''
    if territories[position] != EMPTY:
        return territories[position], []

    # Perform depth-first search to get area color
    stack = [position]
    checked: Set[Tuple[int, int]] = set()
    positions: List[Tuple[int, int]] = []
    color = EMPTY
    single = True

    while len(stack) > 0:
        y, x = stack.pop()
        positions.append((y, x))

        for pos in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
            if pos in checked:
                continue

            checked.add(pos)

            if not is_valid_position(pos, territories.shape[1], territories.shape[0]):
                continue
            elif territories[pos] == EMPTY:
                stack.append(pos)
            elif color == EMPTY or territories[pos] == color:
                color = territories[pos]
            else:
                single = False

    # If the area color is single, return that color and list of positions
    if single:
        return color, positions
    # If the area color is not single, return empty color and list of positions
    else:
        return EMPTY, positions


class Candidate(object):
    def __init__(
        self,
        pos: Tuple[int, int],
        color: int,
        visits: int,
        playouts: int,
        policy: float,
        value: float,
        variations: List[Tuple[int, int]],
    ) -> None:
        '''Initialize candidate move object.
        Args:
            pos (Tuple[int, int]): Coordinates to place stone
            color (int): Stone color
            visits (int): Number of visits
            playouts (int): Number of playouts
            policy (float): Predicted move probability
            value (float): Predicted win rate
            variations (List[Tuple[int, int]]): Predicted sequence
        '''
        self.pos = pos
        self.color = color
        self.visits = visits
        self.playouts = playouts
        self.policy = policy
        self.value = value
        self.variations = variations
        self.win_chance = value * color * 0.5 + 0.5
        self.win_chance_lcb = self.win_chance - 1.96 * 0.25 / (visits + 1)**0.5

    def __str__(self) -> str:
        return (
            f'Candidate(pos={self.pos}, color={get_color_name(self.color)},'
            f' visits={self.visits}, playouts={self.playouts},'
            f' policy={self.policy:.2f}, value={self.value:.2f},'
            f' win_chance={self.win_chance:.2f}, win_chance_lcb={self.win_chance_lcb:.2f}')

    def __repr__(self) -> str:
        return str(self)


class Player(object):
    def __init__(
        self,
        processor: Processor,
        threads: int = 1,
        width: int = DEFAULT_SIZE,
        height: int = DEFAULT_SIZE,
        komi: float = DEFAULT_KOMI,
        rule: int = RULE_CH,
        superko: bool = False,
        eval_leaf_only: bool = False,
    ) -> None:
        '''Initialize player object.
        Args:
            processor (Processor): Computation management object
            threads (int): Number of threads to use
            width (int): Board width
            height (int): Board height
            komi (float): Komi value
            rule (int): Rule for determining winner
            superko (bool): True to apply superko rule
            eval_leaf_only (bool): True to evaluate only leaf nodes
        '''
        self.native = NativePlayer(
            processor=processor.native, threads=threads,
            width=width, height=height, komi=komi, rule=rule, superko=superko,
            eval_leaf_only=eval_leaf_only)
        self.processor = processor
        self.width = width
        self.height = height
        self.komi = komi
        self.rule = rule
        self.superko = superko
        self.turn = 0
        self.value = 0.0
        self.moves = [0, 0]
        self.captureds = [0, 0]
        self.histories: Set[Tuple] = set()

    def initialize(self) -> None:
        '''Initialize the board.'''
        self.native.initialize()
        self.turn = 0
        self.value = 0.0
        self.moves = [0, 0]
        self.captureds = [0, 0]

    def set_handicap(self, handicap: int) -> None:
        '''Set handicap stones.
        Args:
            handicap (int): Number of handicap stones
        '''
        for pos in get_handicap_positions(self.width, self.height, handicap):
            if self.native.get_color() != BLACK:
                self.native.play(PASS)

            self.native.play(pos)
            self.moves[0] += 1

    def is_valid_position(self, pos: Tuple[int, int]) -> bool:
        '''Return whether the specified coordinates are valid.
        Args:
            pos (Tuple[int, int]): Coordinates
        Returns:
            bool: True if valid coordinates
        '''
        return is_valid_position(pos, self.width, self.height)

    def is_superko_move(self, pos: Tuple[int, int], color: int) -> bool:
        '''Determine whether the specified coordinates are a superko move.
        Args:
            pos (Tuple[int, int]): Coordinates
            color (int): Stone color
        Returns:
            bool: True if it is a superko move
        '''
        if not self.is_valid_position(pos):
            return False

        board = self.get_board()
        board.play(pos, color)

        return tuple(board.get_patterns()) in self.histories

    def get_cleanup_position(self, color: int) -> Tuple[int, int]:
        '''Return the coordinates to remove dead stones.
        Args:
            color (int): Color of the stone to place
        Returns:
            Tuple[int, int]: Coordinates
        '''
        board = self.get_board()
        colors = board.get_colors(color)
        territories = board.get_territories(color)
        enableds = board.get_enableds(color)

        deads = ((territories == BLACK) & (colors == WHITE)).astype(np.int32)
        targets = np.zeros_like(deads)
        targets[1:, :] += deads[:-1, :]
        targets[:-1, :] += deads[1:, :]
        targets[:, 1:] += deads[:, :-1]
        targets[:, :-1] += deads[:, 1:]

        positions = (targets != 0) & enableds
        ys, xs = np.where(positions)

        if len(xs) == 0:
            return PASS

        return xs[0], ys[0]

    def play(self, pos: Tuple[int, int], color: int | None = None) -> int:
        '''Place a stone.
        Args:
            pos (Tuple[int, int]): Coordinates to place the stone
            color (int | None): Stone color (if not specified, use the current turn's color)
        Returns:
            int: Number of captured stones
        '''
        # If the stone color is not specified, use the current turn's color
        if color is None:
            color = self.native.get_color()

        # If the color to play is different, pass
        if self.native.get_color() != color:
            self.native.play(PASS)

        # Play the move
        captured = self.native.play(pos)
        self.turn += 1

        # Update the number of moves and captured stones
        move_count = int(self.is_valid_position(pos) and captured >= 0)

        if color == BLACK:
            self.moves[0] += move_count
            self.captureds[1] += captured
        elif color == WHITE:
            self.moves[1] += move_count
            self.captureds[0] += captured

        # Register the board state in the history
        self.histories.add(tuple(self.get_board().get_patterns()))

        return captured

    def get_pass(self) -> Candidate:
        '''Return a candidate for pass.
        Returns:
            Candidate: Pass candidate
        '''
        self.native.start_evaluation(False, False, 0, 1.0, 0.0)
        self.native.wait_evaluation(1, 0, 120.0, True)

        return Candidate(*self.native.get_pass())

    def get_random(self, temperature: float = 0.0, allow_outermost: bool = True) -> Candidate:
        '''Return a random move.
        Args:
            temperature (float): Temperature parameter
            allow_outermost (bool): True to allow moves on the edge
        Returns:
            Candidate: Candidate move
        '''
        self.native.start_evaluation(False, False, 0, 1.0, 0.0)
        self.native.wait_evaluation(1, 0, 120.0, True)

        for _ in range(10):
            # Select a move randomly
            candidate = Candidate(*self.native.get_random(temperature))

            # Output log
            LOGGER.debug(candidate)

            # If the move is valid, exit the loop
            if not self.is_valid_position(candidate.pos):
                break
            elif self.superko and self.is_superko_move(candidate.pos, candidate.color):
                candidate = self.get_pass()
                continue
            elif not allow_outermost and (
                    candidate.pos[0] == 0 or candidate.pos[0] == self.width - 1
                    or candidate.pos[1] == 0 or candidate.pos[1] == self.height - 1):
                continue
            else:
                break

        return candidate

    def evaluate(
        self,
        visits: int,
        playouts: int = 0,
        timelimit: float = 120.0,
        equally: bool = False,
        use_ucb1: bool = False,
        width: int | None = None,
        temperature: float = 1.0,
        noise: float = 0.0,
        criterion: str = 'lcb',
        ponder: bool = False,
    ) -> List[Candidate]:
        '''Evaluate the board.
        Args:
            visits (int): Target number of visits
            playouts (int): Target number of playouts
            timelimit (float): Time limit (seconds)
            equally (bool): True to make the number of searches equal, False to use UCB1 or PUCB
            use_ucb1 (bool): True to use UCB1 as the search criterion, False to use PUCB
            width (int): Search width (number of candidate moves to search; 0 for auto adjustment)
            temperature (float): Temperature parameter for search
            noise (float): Strength of Gumbel noise for search
            criterion (str): Candidate priority criterion ('lcb' or 'visits')
            ponder (bool): True to continue searching
        Returns:
            List[Candidate]: List of candidate moves
        '''
        width = width if width is not None else 0

        # Evaluate the board
        self.native.start_evaluation(equally, use_ucb1, width, temperature, noise)
        self.native.wait_evaluation(visits, playouts, timelimit, not ponder)

        # Create a list of candidate moves
        candidates = [Candidate(*c) for c in self.native.get_candidates()]

        # Remove candidates that are superko moves
        if self.superko:
            candidates = [
                c for c in candidates if not self.is_superko_move(c.pos, c.color)]

        # If there are no candidates, add pass as a candidate
        if len(candidates) == 0:
            candidates.append(self.get_pass())

        # In COM rule, if there are dead stones to be removed, replace pass
        # coordinates with cleanup coordinates
        if self.rule == RULE_COM:
            for i in range(len(candidates)):
                if not self.is_valid_position(candidates[i].pos):
                    candidates[i].pos = self.get_cleanup_position(candidates[i].color)

        # Sort candidates
        if criterion == 'visits':
            candidates.sort(key=lambda cand: cand.visits, reverse=True)
        elif criterion == 'lcb':
            candidates.sort(key=lambda cand: cand.win_chance_lcb, reverse=True)
        else:
            raise ValueError(f'Unknown criterion: {criterion}')

        # Output log
        if LOGGER.isEnabledFor(logging.DEBUG):
            LOGGER.debug(
                'Evaluation: %d visits, %d playouts',
                sum(c.visits for c in candidates),
                sum(c.playouts for c in candidates))
            for candidate in candidates:
                LOGGER.debug(candidate)

        # Return the list of candidates
        return candidates

    def stop_evaluation(self) -> None:
        '''Stop if pondering.'''
        self.native.wait_evaluation(0, 0, 0.0, True)

    def get_territories(
        self,
        pos: Tuple[int, int] | None = None,
        color: int | None = None,
        raw: bool = False,
    ) -> np.ndarray:
        '''Predict the final territory.
        If a move coordinate is specified,
        returns the territory after placing a stone at that coordinate.
        Args:
            pos (Tuple[int, int]): Move coordinate
            color (int): Move color
            raw (bool): True to return raw data
        Returns:
            np.ndarray: Predicted territory
        '''
        # Get board data
        board = self.get_board()

        # Get the next move color
        if color is None:
            color = self.get_color()

        # If a move coordinate is specified,
        # return the territory after placing a stone at that coordinate
        if pos is not None:
            board.play(pos, color)
            color = get_opposite_color(color)

        # Create input values
        x = board.get_inputs(color, self.komi, self.rule, self.superko)
        x = x.reshape(1, -1)

        # Execute territory evaluation
        y = self.processor.execute(x)

        # Format output values
        model_length = MODEL_SIZE * MODEL_SIZE
        begin_x = (MODEL_SIZE - self.width) // 2
        end_x = begin_x + self.width
        begin_y = (MODEL_SIZE - self.height) // 2
        end_y = begin_y + self.height

        t = y[0, 2 * model_length:5 * model_length]
        t = t.reshape(3, MODEL_SIZE, MODEL_SIZE)
        t = t[:, begin_y:end_y, begin_x:end_x]

        # Reverse colors for white's turn
        if color != BLACK:
            t = t[::-1]

        # Reflect already determined territory data
        territories = board.get_territories()
        black_territories = (territories == BLACK).astype(np.float32)
        white_territories = (territories == WHITE).astype(np.float32)
        empty_territories = (territories == EMPTY).astype(np.float32)

        t = t * np.expand_dims(empty_territories, 0)
        t[0] += white_territories
        t[2] += black_territories

        if raw:
            return t
        else:
            return t.argmax(axis=0) - 1

    def get_values(self) -> np.ndarray:
        '''Get the board evaluation values.
        Returns:
            np.ndarray: Board evaluation values
        '''
        board = self.get_board()

        # Create input values
        x = board.get_inputs(BLACK, self.komi, self.rule, self.superko)
        x = x.reshape(1, -1)

        # Get evaluation values
        y = self.processor.execute(x)

        # Format output values
        model_length = MODEL_SIZE * MODEL_SIZE
        begin_x = (MODEL_SIZE - self.width) // 2
        end_x = begin_x + self.width
        begin_y = (MODEL_SIZE - self.height) // 2
        end_y = begin_y + self.height

        v = y[0, 5 * model_length:6 * model_length]
        v = v.reshape(MODEL_SIZE, MODEL_SIZE)
        v = v[begin_y:end_y, begin_x:end_x] * 2.0 - 1.0

        return v

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
        board.load_state(self.native.get_board_state())
        return board

    def get_board_state(self) -> List[int]:
        '''Get the board state.
        Returns:
            List[int]: Board state
        '''
        return self.native.get_board_state()

    def get_captured(self, color: int) -> int:
        '''Return the number of captured stones of the specified color.
        Args:
            color (int): Stone color
        Returns:
            int: Number of captured stones
        '''
        if color == BLACK:
            return self.captureds[0]
        elif color == WHITE:
            return self.captureds[1]
        else:
            return 0

    def get_final_score(self) -> float:
        '''Get the final score.
        Returns:
            float: Score
        '''
        # Get stone colors and territory prediction
        colors = self.get_board().get_colors()
        territories = self.get_territories()

        # Reflect seki
        territories += (territories == 0).astype(np.int32) * colors

        # For Chinese rules, replace surrounded empty points with the surrounding color
        if self.rule == RULE_CH:
            checked_positions = np.zeros_like(territories, dtype=np.bool_)

            for y, x in np.ndindex(territories.shape):
                if checked_positions[y, x] or territories[y, x] != EMPTY:
                    continue

                color, positions = get_area_color(territories, (y, x))

                for pos in positions:
                    territories[pos] = color
                    checked_positions[pos] = True

        # Calculate the score
        result = float(territories.sum()) - self.komi

        if self.rule == RULE_JP:
            result -= self.moves[0] - self.moves[1]

        return result
