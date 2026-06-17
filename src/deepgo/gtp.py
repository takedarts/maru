import json
import logging
import re
import struct
import subprocess
import sys
from pathlib import Path
from threading import Thread
from typing import Any, Callable, Dict, List, TextIO, Tuple

import numpy as np
from deepgo.record import Record

from .board import (Board, get_array_string, get_color_name,
                    get_handicap_positions, get_opposite_color, is_valid_position)
from .config import (BLACK, DEFAULT_KOMI, DEFAULT_PUCB_CONSTANT_BASE,
                     DEFAULT_PUCB_CONSTANT_INIT, DEFAULT_SIZE, EMPTY,
                     MODEL_SIZE, NAME, PASS, RULE_CH, RULE_COM, RULE_JP, VERSION, WHITE)
from .exception import GoException
from .player import Candidate, Player
from .processor import Processor

LOGGER = logging.getLogger(__name__)


def gtp_string_to_position(s: str, width: int, height: int) -> Tuple[int, int] | None:
    '''Convert GTP string representation to coordinate values.
    Returns (-1, -1) for pass, and None for resign.
    Args:
        s (str): GTP string representation
        width (int): Board width
        height (int): Board height
    Returns:
        Tuple[int, int]: Coordinate values
    '''
    if s.lower() == 'pass':
        return PASS
    elif s.lower() == 'resign':
        return None

    if not re.match(r'^[a-tA-T]\d+$', s):
        raise GoException('{} is not vertex'.format(s))

    x = s[0].upper().encode('ascii')[0] - 65
    x = x - 1 if x > 8 else x
    y = height - int(s[1:])

    if not is_valid_position((x, y), width, height):
        raise GoException(f'{s} is not vertex')

    return (x, y)


def gtp_string_to_color(s: str) -> int | None:
    '''Convert string representation to stone color.
    Args:
        s (str): String representation
    Returns:
        int | None: Stone color
    '''
    s = s.lower().strip()

    if s[0] == 'b':
        return BLACK
    elif s[0] == 'w':
        return WHITE
    else:
        return None


def gtp_position_to_string(p: Tuple[int, int] | None, width: int, height: int) -> str:
    '''Convert coordinate values to GTP string representation.
    Args:
        p (Tuple[int, int] | None): Coordinate values
        width (int): Board width
        height (int): Board height
    Returns:
        str: GTP string representation
    '''
    if p is None:
        return 'resign'

    x, y = p

    if not is_valid_position(p, width, height):
        return 'PASS'

    x = x + 1 if x >= 8 else x
    x_char = struct.pack('B', 65 + x).decode('ascii')
    y_char = str(height - y)

    return f'{x_char}{y_char}'


def gtp_color_to_string(c: int) -> str:
    '''Convert stone color to string representation.
    Args:
        c (int): Stone color
    Returns:
        str: String representation
    '''
    if c == BLACK:
        return 'black'
    elif c == WHITE:
        return 'white'
    else:
        raise GoException(f'{c} is not color')


def gtp_args_to_color(args: List[str]) -> int | None:
    '''Get color from argument list.
    Args:
        args (List[str]): Argument list
    Returns:
        int | None: Stone color
    '''
    for arg in args:
        color = gtp_string_to_color(arg)

        if color is not None:
            return color

    return None


def lz_candidate_to_string(
    order: int,
    candidate: Candidate,
    width: int,
    height: int,
) -> str:
    '''Convert candidate move to LeelaZero string representation.
    Args:
        order (int): Priority
        candidate (Candidate): Candidate move
        width (int): Board width
        height (int): Board height
    Returns:
        str: LeelaZero string representation
    '''
    candidate_text = (
        f'info move {gtp_position_to_string(candidate.pos, width, height)}'
        f' visits {candidate.visits}'
        f' winrate {int(candidate.win_chance * 10000)}'
        f' lcb {int(candidate.win_chance_lcb * 10000)}'
        f' prior {int(candidate.policy * 10000)}'
        f' order {order}')

    if len(candidate.variations) != 0:
        variations = ' '.join(
            gtp_position_to_string(p[0], width, height) for p in candidate.variations)
        candidate_text += f' pv {variations}'

    return candidate_text.strip()


def lz_candidates_to_string(
    candidates: List[Candidate],
    territories: np.ndarray,
    score: float,
    width: int,
    height: int,
    rootinfo: bool,
    ownership: bool,
) -> str:
    '''Convert list of candidate moves to LeelaZero string representation.
    Args:
        candidates (List[Candidate]): List of candidate moves
        territories (np.ndarray): Territory data (not used, ignored)
        score (float): Predicted score difference
        width (int): Board width
        height (int): Board height
        rootinfo (bool): True if root info should be output (not used, always disabled)
        ownership (bool): True if ownership should be output (not used, always disabled)
    Returns:
        str: LeelaZero string representation
    '''
    return ' '.join(
        lz_candidate_to_string(o, c, width, height)
        for o, c in enumerate(candidates))


def kata_candidate_to_string(
    order: int,
    candidate: Candidate,
    width: int,
    height: int,
) -> str:
    '''Convert candidate move to KataGo string representation.
    Args:
        order (int): Priority
        candidate (Candidate): Candidate move
        width (int): Board width
        height (int): Board height
    Returns:
        str: KataGo string representation
    '''
    candidate_text = (
        f'info move {gtp_position_to_string(candidate.pos, width, height)}'
        f' visits {candidate.visits}'
        f' winrate {candidate.win_chance:.4f}'
        f' lcb {candidate.win_chance_lcb:.4f}'
        f' prior {candidate.policy:.4f}'
        f' order {order}')

    if len(candidate.variations) != 0:
        variations = ' '.join(
            gtp_position_to_string(p[0], width, height) for p in candidate.variations)
        candidate_text += f' pv {variations}'

    return candidate_text.strip()


def kata_candidates_to_string(
    candidates: List[Candidate],
    territories: np.ndarray,
    score: float,
    width: int,
    height: int,
    rootinfo: bool,
    ownership: bool,
) -> str:
    '''Convert list of candidate moves to KataGo string representation.
    Args:
        candidates (List[Candidate]): List of candidate moves
        territories (np.ndarray): Territory data
        score (float): Predicted score difference
        width (int): Board width
        height (int): Board height
        rootinfo (bool): True if root info should be output
        ownership (bool): True if ownership should be output
    Returns:
        str: KataGo string representation
    '''
    output_texts = []

    # Create candidate move string
    candidates_text = ' '.join(
        kata_candidate_to_string(o, c, width, height)
        for o, c in enumerate(candidates))
    output_texts.append(candidates_text)

    # Create rootInfo string
    if rootinfo:
        win_chance = candidates[0].win_chance
        visits = sum(c.visits for c in candidates)
        score = score if candidates[0].color == BLACK else -score
        root_text = (f'rootInfo winrate {win_chance:.4f} visits {visits} scoreLead {score:.1f}')
        output_texts.append(root_text)

    # Create territory string
    if ownership:
        def territory_to_string(t: np.ndarray) -> str:
            v = max(float(t[2] - t[1]), 0) - max(float(t[0] - t[1]), 0)
            return f'{v:.2f}' if candidates[0].color == BLACK else f'{-v:.2f}'

        ownership_values = ' '.join(
            territory_to_string(territories[:, y, x]) for y, x in np.ndindex(height, width))
        ownership_text = f'ownership {ownership_values}'
        output_texts.append(ownership_text)

    return ' '.join(output_texts)


def cgos_candidates_to_string(
    candidates: List[Candidate],
    territories: np.ndarray,
    score: float,
    width: int,
    height: int,
    rootinfo: bool,
    ownership: bool,
) -> str:
    '''Convert list of candidate moves to CGOS string representation.
    Args:
        candidates (List[Candidate]): List of candidate moves
        territories (np.ndarray): Territory data
        score (float): Predicted score difference
        width (int): Board width
        height (int): Board height
        rootinfo (bool): True if root info should be output (not used, always enabled)
        ownership (bool): True if ownership should be output (not used, always enabled)
    Returns:
        str: CGOS string representation
    '''
    # Variable to store candidate move data
    root_values: Dict[str, Any] = {}

    # Set rootInfo values
    root_values['winrate'] = candidates[0].win_chance
    root_values['score'] = score if candidates[0].color == BLACK else -score
    root_values['visits'] = sum(c.visits for c in candidates)

    # Create candidate move string
    move_values: List[Dict[str, Any]] = []

    for candidate in candidates:
        variations = ' '.join(
            gtp_position_to_string(p[0], width, height) for p in candidate.variations)

        move_values.append({
            'move': gtp_position_to_string(candidate.pos, width, height),
            'winrate': candidate.win_chance,
            'prior': candidate.policy,
            'pv': variations,
            'visits': candidate.visits,
        })

    root_values['moves'] = move_values

    # Set territory values
    def territory_to_string(t: np.ndarray) -> str:
        c = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+'
        v = max(float(t[2] - t[1]), 0) - max(float(t[0] - t[1]), 0)
        v = v if candidates[0].color == BLACK else -v
        return c[round(max(0, min((v + 1) / 2, 1)) * 62)]

    root_values['ownership'] = ''.join(
        territory_to_string(territories[:, y, x]) for y, x in np.ndindex(height, width))

    return json.dumps(root_values)


class Display(object):
    '''Class for displaying the board.'''

    def __init__(self, cmd: str) -> None:
        '''Initialize board display object.
        Args:
            cmd (str): Command to run board display application
        '''
        self._pipe = subprocess.Popen(
            cmd.split(), stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self.send('clear_board')

    def wait(self) -> None:
        '''Wait until display process finishes.'''
        self._pipe.wait()

    def close(self) -> None:
        '''Terminate display process.'''
        self._pipe.terminate()

    def play(self, pos: Tuple[int, int], color: int) -> None:
        '''Display a stone on the board.
        Args:
            pos (Tuple[int, int]): Coordinates to place the stone
            color (int): Color of the stone to place
        '''
        pos_str = gtp_position_to_string(pos, 19, 19)
        color_str = 'b' if color == BLACK else 'w'

        self.send(f'play {color_str} {pos_str}')

    def send(self, message: str) -> None:
        '''Send display command.
        Args:
            message (str): Display command
        '''
        assert self._pipe.stdin is not None
        assert self._pipe.stdout is not None

        self._pipe.stdin.write(message.encode())
        self._pipe.stdin.write(b'\n')
        self._pipe.stdin.flush()
        LOGGER.debug('Display: send=%s', message)

        line = b''

        while True:
            c = self._pipe.stdout.read(1)

            if c != b'\n':
                line += c
                continue

            line_bytes = line.decode().strip()

            if len(line_bytes) == 0:
                break

            LOGGER.debug('Display: recv=%s', line_bytes)
            line = b''


class GTPEngine(object):
    '''Class for running Go AI as a GTP engine.'''

    def __init__(
        self,
        processor: Processor,
        threads: int,
        visits: int,
        playouts: int = 0,
        criterion: str = 'value',
        temperature: float = 1.0,
        randomness: float = 0.0,
        rule: int = RULE_CH,
        boardsize: int = DEFAULT_SIZE,
        komi: float = DEFAULT_KOMI,
        superko: bool = False,
        pucb_constant_init: float = DEFAULT_PUCB_CONSTANT_INIT,
        pucb_constant_base: float = DEFAULT_PUCB_CONSTANT_BASE,
        timelimit: float = 10,
        ponder: bool = False,
        resign_threshold: float = 0.0,
        resign_score: float = 0.0,
        resign_turn: int = 0,
        initial_turn: int = 0,
        client_name: str = NAME,
        client_version: str = VERSION,
        reader: TextIO = sys.stdin,
        writer: TextIO = sys.stdout,
        display: str | None = None,
    ):
        '''Create a GTP engine.
        Args:
            processor (Processor): Inference execution object
            threads (int): Number of threads to use
            visits (int): Target number of visits
            playouts (int): Target number of playouts
            criterion (str): Candidate move priority criterion ('value' or 'visits')
            temperature (float): Search temperature parameter
            randomness (float): Randomness of search visits
            rule (int): Game rule
            komi: float: Komi value
            superko (bool): True to apply superko rule
            pucb_constant_init (float): Initial value applied to PUCB upper confidence bound
            pucb_constant_base (float): Base value applied to PUCB upper confidence bound
            timelimit (float): Maximum thinking time
            ponder (bool): True to continue analysis during opponent's turn
            resign_threshold (float): Win rate for resignation
            resign_score (float): Score difference for resignation
            resign_turn (int): Minimum number of turns before resignation
            initial_turn (int): Number of initial random moves
            client_name (str): Client display name
            client_version (str): Client display version
            reader (TextIO): Stream to input commands
            writer (TextIO): Stream to output results
            display (str | None): Display command
        '''
        self.processor = processor
        self.threads = threads
        self.player: Player | None = None
        self.moves: List[Tuple[Tuple[int, int], int]] = []

        self.visits = visits
        self.playouts = playouts
        self.criterion = criterion
        self.temperature = temperature
        self.randomness = randomness

        self.rule = rule
        self.size = boardsize
        self.komi = komi
        self.superko = superko
        self.pucb_constant_init = pucb_constant_init
        self.pucb_constant_base = pucb_constant_base
        self.timelimit = timelimit
        self.ponder = ponder
        self.remain_times = [-1, -1]

        self.resign_threshold = resign_threshold
        self.resign_score = resign_score
        self.resign_turn = resign_turn

        self.initial_turn = initial_turn

        self.client_name = client_name
        self.client_version = client_version

        self.reader = reader
        self.writer = writer

        self.display = Display(display) if display is not None else None

        self.thread: Thread | None = None
        self.terminated = False

    def load(self, path: str | Path) -> None:
        '''Load SGF file.
        Args:
            sgf (str | Path): Path to SGF file
        '''
        record = Record(path)
        board_size = int(record.properties.get('sz', str(DEFAULT_SIZE)))
        komi = float(record.properties.get('km', str(DEFAULT_KOMI)))
        handicap = int(record.properties.get('ha', '0'))

        self._perform_command_boardsize([str(board_size)])
        self._perform_command_komi([str(komi)])

        for pos in get_handicap_positions(board_size, board_size, handicap):
            gtp_pos = gtp_position_to_string(pos, board_size, board_size)
            self._perform_command_play(['black', gtp_pos])

        for pos, color, _ in record.moves:
            gto_color = get_color_name(color)
            gtp_pos = gtp_position_to_string(pos, board_size, board_size)
            self._perform_command_play([gto_color, gtp_pos])

    def run(self) -> None:
        '''Run the engine in GTP mode.'''
        regex = re.compile(r'^(\d+)\s+(.*)$')

        while not self.reader.closed and not self.writer.closed:
            try:
                # Read command
                command = self.reader.readline()

                # If no characters (no data to read), exit
                if len(command) == 0:
                    break

                # Remove newline characters
                command = command.strip()

                # If empty string, continue reading
                if len(command) == 0:
                    continue

                # If a thread is running, terminate it
                if self.thread is not None:
                    self.terminated = True
                    self.thread.join()

                # Output log
                LOGGER.debug('GTP command: %s', command)

                # Parse command
                number = ''

                if match := regex.match(command):
                    number = match.group(1).strip()
                    command = match.group(2).strip()

                # If quit command, exit loop
                if command.lower().startswith('quit'):
                    self.thread = None
                    self.writer.write(f'={number}\n\n')
                    break

                # Execute command in a separate thread
                self.terminated = False
                self.thread = Thread(target=self._perform, args=(number, command))
                self.thread.start()
            except BaseException as e:
                LOGGER.error('GTP error: %s', str(e))
                break

    def _get_timelimit(self, color: int) -> float:
        if color == BLACK:
            remain_time = self.remain_times[0]
        else:
            remain_time = self.remain_times[1]

        if remain_time < 0:
            return self.timelimit
        else:
            return max(min(self.timelimit, (remain_time - 20) * 0.02), 0)

    def _create_player(self) -> Player:
        '''Create player object
        Returns:
            Player: Player object
        '''
        LOGGER.debug(
            'Create player: '
            'width=%d, height=%d, komi=%.1f, rule=%d, superko=%s',
            self.size, self.size, self.komi, self.rule, self.superko)

        return Player(
            processor=self.processor,
            threads=self.threads,
            width=self.size,
            height=self.size,
            komi=self.komi,
            rule=self.rule,
            superko=self.superko,
            pucb_constant_init=self.pucb_constant_init,
            pucb_constant_base=self.pucb_constant_base,
        )

    def _random_move(self, color: int) -> Candidate:
        '''Make a random move based on policy.
        Args:
            color (int): Color to play
        Returns:
            Candidate: Candidate move
        '''
        # Create player object if not present
        if self.player is None:
            self.player = self._create_player()

        # Match the color to play
        if self.player.get_color() != color:
            self.player.play(PASS)

        # Get candidate move
        LOGGER.debug('Random: color=%s', gtp_color_to_string(color))
        candidate = self.player.get_random()

        # Return candidate move
        return candidate

    def _evaluate(
        self,
        color: int,
        visits: int | None = None,
        playouts: int | None = None,
        timelimit: float | None = None,
    ) -> List[Candidate]:
        '''Evaluate the board.
        Args:
            color (int): Color to play
            visits (int | None): Target number of visits
            playouts (int | None): Target number of playouts
            timelimit (float | None): Maximum thinking time
        Returns:
            List[Candidate]: List of candidate moves
        '''
        # Create player object if not present
        if self.player is None:
            self.player = self._create_player()

        # Match the color to play
        if self.player.get_color() != color:
            self.player.play(PASS)

        # Set number of visits
        if visits is None:
            rand = (1 - self.randomness / 2) + (np.random.rand() * self.randomness)
            visits = max(int(self.visits * rand), 1)

        # Set number of playouts
        if playouts is None:
            rand = (1 - self.randomness / 2) + (np.random.rand() * self.randomness)
            playouts = max(int(self.playouts * rand), 0)

        # Set thinking time
        if timelimit is None:
            timelimit = self._get_timelimit(color)

        # Evaluate the board
        LOGGER.debug(
            'Evaluate: color=%s, visits=%d, playouts=%d, timelimit=%.1f',
            gtp_color_to_string(color), visits, playouts, timelimit)

        candidates = self.player.evaluate(
            visits=visits,
            playouts=playouts,
            criterion=self.criterion,
            timelimit=timelimit,
            temperature=self.temperature,
            ponder=self.ponder)

        # Return list of candidate moves
        return candidates

    def _get_move(
        self,
        candidates: List[Candidate],
    ) -> Tuple[Tuple[int, int] | None, float, np.ndarray]:
        '''Get the move coordinate.
        Candidate moves are assumed to be sorted in order of priority.
        If resigning, returns None as the coordinate.
        Args:
            candidates (List[Candidate]): List of candidate moves
        Returns:
            Tuple[Tuple[int, int] | None, float, np.ndarray]: Move, score, territory
        '''
        # Check player object
        if self.player is None:
            raise GoException('Game has not started yet')

        # Check number of candidate moves
        if len(candidates) == 0:
            raise GoException('No candidates')

        # Get move under Japanese rules
        if self.rule == RULE_JP:
            pos = self._get_move_in_jp_rule(candidates)
        # Get move under COM rules
        elif self.rule == RULE_COM:
            pos = self._get_move_in_com_rule(candidates)
        # Get move under Chinese rules
        else:
            pos = self._get_move_in_ch_rule(candidates)

        # If the move is included in the candidate moves,
        #  use the score and territory of that candidate.
        # If the move is not included in the candidate moves,
        # use the score and territory of the first candidate.
        selected_candidates = [c for c in candidates if c.pos == pos]

        if len(selected_candidates) > 0:
            candidate = selected_candidates[0]
        else:
            candidate = candidates[0]

        score = candidate.get_score(self.player.get_board()) - self.komi
        territories = candidate.territories
        win_chance = candidate.win_chance

        # If pass, calculate score with all territories confirmed
        if pos == PASS:
            board = self.player.get_board()
            fixed_territories = territories.argmax(axis=0) - 1
            fixed_territories += (fixed_territories == EMPTY) * board.get_owners()
            score = fixed_territories.sum() - self.komi

            return pos, score, territories

        # Do not resign if turn is less than specified
        if self.player.turn < self.resign_turn:
            return pos, score, territories

        # Do not resign if score difference is less than threshold
        if abs(score) < self.resign_score:
            return pos, score, territories

        # Resign if win chance is less than threshold (stop search if pondering)
        if win_chance < self.resign_threshold:
            self.player.stop_evaluation()
            return None, score, territories

        # If not resigning, return move coordinates
        return pos, score, territories

    def _get_move_in_ch_rule(self, candidates: List[Candidate]) -> Tuple[int, int]:
        '''Get the move according to Chinese rules.
        Returns the first candidate as the move.
        Args:
            candidates (List[Candidate]): List of candidate moves
        Returns:
            Tuple[int, int]: Move coordinates
        '''
        return candidates[0].pos

    def _get_move_in_com_rule(self, candidates: List[Candidate]) -> Tuple[int, int]:
        '''Get the move according to COM rules.
        Returns the candidate move if there is a non-pass candidate.
        Otherwise, returns a move that captures the opponent's stones.
        If there is no move to capture opponent's stones, returns a pass.
        Args:
            candidates (List[Candidate]): List of candidate moves
        Returns:
            Tuple[int, int]: Move coordinates
        '''
        # If there is a non-pass candidate move, return that candidate
        for candidate in candidates:
            if candidate.pos != PASS:
                return candidate.pos

        # Search for a move to capture opponent's stones
        my_color = candidates[0].color
        op_color = get_opposite_color(my_color)

        assert self.player is not None
        board = self.player.get_board()
        board_enableds = board.get_enableds(my_color)
        board_targets = (
            (board.get_territories() == my_color)
            & (board.get_colors() == op_color))

        for y, x in np.argwhere(board_targets):
            for ny, nx in [(y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)]:
                if not board.is_valid_position((nx, ny)):
                    continue
                elif board_enableds[ny, nx]:
                    return (nx, ny)

        # If there is no move to capture opponent's stones, return a pass
        return PASS

    def _get_move_in_jp_rule(self, candidates: List[Candidate]) -> Tuple[int, int]:
        '''Get a move in Japanese rules.
        Args:
            candidates (List[Candidate]): List of candidate moves
        Returns:
            Tuple[int, int]: Move coordinates
        '''
        assert self.player is not None

        # Search for a pass candidate
        pass_candidate = None

        for candidate in candidates:
            if candidate.pos == PASS:
                pass_candidate = candidate
                break

        # If no pass candidate exists, create a new pass candidate
        if pass_candidate is None:
            pass_candidate = self.player.get_pass()

        # Get non-pass candidates
        candidates = [c for c in candidates if c.pos != PASS]

        # If no non-pass candidates exist, return pass
        if len(candidates) == 0:
            return PASS

        # Examine candidates
        board = self.player.get_board()
        colors = board.get_colors()
        pass_value = pass_candidate.value
        pass_score = pass_candidate.get_score(board) - self.komi
        pass_territories = pass_candidate.territories.argmax(axis=0) - 1

        # Calculate boundaries of predicted territory
        vertical_borders = np.abs(pass_territories[:-1, :] - pass_territories[1:, :]) == 2
        horizontal_borders = np.abs(pass_territories[:, :-1] - pass_territories[:, 1:]) == 2
        borders = np.zeros_like(pass_territories)

        borders[:-1, :] |= vertical_borders
        borders[1:, :] |= vertical_borders
        borders[:, :-1] |= horizontal_borders
        borders[:, 1:] |= horizontal_borders

        # Select a move if it meets the following conditions:
        #  [Condition 1] Move value is 0.1 or higher than pass value
        #  [Condition 2] Move territory is larger than pass territory
        #  [Condition 3] Move position is in an area predicted as seki
        #  [Condition 4] Own chain with size 1 and 1 liberty in border area exists near move position
        # Exclude moves if they meet the following conditions:
        #  [Condition 1] Move value is 0.05 or lower than pass value
        #  [Condition 2] Move territory is smaller than pass territory
        #  [Condition 3] Move score is 0.5 or worse than pass score
        excludes = []

        if LOGGER.isEnabledFor(logging.DEBUG):
            LOGGER.debug(
                'JP rule: pass candidate=%s, value=%.4f, score=%.1f',
                pass_candidate.pos, pass_value, pass_score)

        def is_near_atari_ren_in_border(pos: Tuple[int, int], color: int) -> bool:
            for dy, dx in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                ny, nx = pos[1] + dy, pos[0] + dx

                if (board.is_valid_position((nx, ny))
                        and board.get_ren_size((nx, ny)) == 1
                        and board.get_ren_space((nx, ny)) == 1
                        and colors[ny, nx] == color
                        and borders[ny, nx]):
                    return True

            return False

        for candidate in candidates:
            value = candidate.value
            score = candidate.get_score(board) - self.komi
            territories = candidate.territories.argmax(axis=0) - 1

            if LOGGER.isEnabledFor(logging.DEBUG):
                LOGGER.debug(
                    'JP rule: candidate=%s, value=%.4f, score=%.1f',
                    candidate.pos, value, score)

            # Move value is 0.1 or higher than pass value
            if (value - pass_value) * candidate.color >= 0.1:
                if LOGGER.isEnabledFor(logging.DEBUG):
                    LOGGER.debug(
                        'JP rule: select move=%s, value=%.4f',
                        candidate.pos, value - pass_value)
                return candidate.pos
            # Move territory is larger than pass territory
            elif np.any((territories - pass_territories) == (2 * candidate.color)):
                if LOGGER.isEnabledFor(logging.DEBUG):
                    LOGGER.debug(
                        'JP rule: select move=%s, territory=%d', candidate.pos,
                        ((territories - pass_territories) == (2 * candidate.color)).sum())
                return candidate.pos
            # Move position is in an area predicted as seki
            elif pass_territories[candidate.pos[1], candidate.pos[0]] == EMPTY:
                LOGGER.debug('JP rule: select move=%s, in seki territory', candidate.pos)
                return candidate.pos
            # Own chain with size 1 and 1 liberty in border area exists near move position
            elif is_near_atari_ren_in_border(candidate.pos, candidate.color):
                LOGGER.debug('JP rule: select move=%s, near atari ren', candidate.pos)
                return candidate.pos
            # Move value is 0.05 or lower than pass value
            elif (value - pass_value) * candidate.color <= -0.05:
                if LOGGER.isEnabledFor(logging.DEBUG):
                    LOGGER.debug(
                        'JP rule: exclude move=%s, value=%.4f',
                        candidate.pos, value - pass_value)
                excludes.append(True)
            # Move territory is smaller than pass territory
            elif np.any((territories - pass_territories) == (-2 * candidate.color)):
                if LOGGER.isEnabledFor(logging.DEBUG):
                    LOGGER.debug(
                        'JP rule: exclude move=%s, territory=%d', candidate.pos,
                        ((territories - pass_territories) == (2 * candidate.color)).sum())
                excludes.append(True)
            # Move score is 0.5 or worse than pass score
            elif (score - pass_score) * candidate.color <= -0.5:
                if LOGGER.isEnabledFor(logging.DEBUG):
                    LOGGER.debug(
                        'JP rule: exclude move=%s, score=%.1f',
                        candidate.pos, score - pass_score)
                excludes.append(True)
            # Otherwise, do not exclude
            else:
                LOGGER.debug('JP rule: not exclude move=%s', candidate.pos)
                excludes.append(False)

        # If all borders of the predicted territory are occupied when passing, select pass
        if not np.any(borders & (colors == EMPTY)):
            LOGGER.debug('JP rule: select move=PASS, all borders have stones')
            return PASS

        # Look for dame moves (moves to the border)
        for candidate, exclude in zip(candidates, excludes):
            if exclude:
                continue

            if borders[candidate.pos[1], candidate.pos[0]]:
                LOGGER.debug('JP rule: select move=%s, in dame zone', candidate.pos)
                return candidate.pos

        # If no dame moves are available, select pass
        return PASS

    def _perform(self, number: str, command: str) -> None:
        '''Execute command.
        Args:
            number (str): Command number
            command (str): Command
        '''
        first_response = True

        while True:
            try:
                # Execute process
                stat, message, cont = self._perform_command(command)

                # For the first response, output response mark
                if first_response:
                    first_response = False
                    mark = '=' if stat else '?'
                    header = f'{mark}{number} '
                else:
                    header = ''

                # Output response
                response = f'{header}{message}'
                LOGGER.debug('GTP response: %s', response)
                self.writer.write(response)
                self.writer.flush()

                # If not continuing, output newline and exit loop
                if not cont or self.terminated:
                    self.writer.write('\n\n')
                    self.writer.flush()
                    break
            except BaseException as e:
                LOGGER.error('GTP error: %s', str(e))
                return

    def _perform_command(self, command: str) -> Tuple[bool, str, bool]:
        '''Execute command.
        Args:
            command (str): Command string (including arguments)
        Returns:
            Tuple[bool, str, bool]: (True if successful, message, True to continue execution)
        '''
        tokens = [s for s in command.split() if len(s) != 0]
        command = tokens[0].lower().replace('-', '_')
        args = tokens[1:]
        method_name = '_perform_command_{}'.format(command)

        if hasattr(self, method_name):
            try:
                return getattr(self, method_name)(args)
            except GoException as e:
                return (False, str(e), False)
        else:
            return (False, 'unknown command', False)

    def _perform_command_protocol_version(self, args: List[str]) -> Tuple[bool, str, bool]:
        return (True, '2', False)

    def _perform_command_name(self, args: List[str]) -> Tuple[bool, str, bool]:
        return (True, self.client_name, False)

    def _perform_command_version(self, args: List[str]) -> Tuple[bool, str, bool]:
        return (True, self.client_version, False)

    def _perform_command_known_command(self, args: List[str]) -> Tuple[bool, str, bool]:
        if len(args) < 1:
            return (False, 'syntax error', False)
        elif hasattr(self, '_perform_command_{}'.format(args[0].lower())):
            return (True, 'true', False)
        else:
            return (True, 'false', False)

    def _perform_command_list_commands(self, args: List[str]) -> Tuple[bool, str, bool]:
        commands = [v[17:] for v in dir(self) if v.startswith('_perform_command_')]
        commands = [f'lz-{v[3:]}' if v.startswith('lz_') else v for v in commands]
        commands = [f'kata-{v[5:]}' if v.startswith('kata_') else v for v in commands]
        commands = [f'cgos-{v[5:]}' if v.startswith('cgos_') else v for v in commands]
        commands.append('quit')

        return (True, '\n'.join(sorted(commands)), False)

    def _perform_command_boardsize(self, args: List[str]) -> Tuple[bool, str, bool]:
        if len(args) < 1 or not re.match(r'^\d+$', args[0]):
            return (False, 'syntax error', False)

        if int(args[0]) > MODEL_SIZE:
            return (False, 'boardsize is too large', False)

        if int(args[0]) != self.size and self.player is not None:
            return (False, 'can not change boardsize after the game starts', False)

        self.size = int(args[0])
        return (True, '', False)

    def _perform_command_clear_board(self, args: List[str]) -> Tuple[bool, str, bool]:
        if self.player is not None:
            self.player.stop_evaluation()

        self.player = None
        self.moves.clear()

        return (True, '', False)

    def _perform_command_komi(self, args: List[str]) -> Tuple[bool, str, bool]:
        if len(args) < 1 or not re.match(r'^\d+(\.\d+)?$', args[0]):
            return (False, 'syntax error', False)

        new_komi = float(args[0])

        if new_komi == self.komi:
            return (True, '', False)

        if self.player is not None:
            self.player.komi = new_komi

        self.komi = new_komi

        return (True, '', False)

    def _perform_command_fixed_handicap(self, args: List[str]) -> Tuple[bool, str, bool]:
        '''Execute fixed_handicap command.
        Args:
            args (List[str]): Argument list
        Returns:
            Tuple[bool, str, bool]: (True if successful, message, True to continue)
        '''
        # Check arguments
        if len(args) < 1:
            return (False, 'syntax error', False)

        handicap = int(args[0])

        if handicap < 2 or handicap > 9:
            return (False, 'handicap must be between 2 and 9', False)

        # Create player object if not present
        if self.player is None:
            self.player = self._create_player()

        # Set handicap stones
        self.player.set_handicap(handicap)

        # Return coordinates where handicap stones were placed
        positions = [
            gtp_position_to_string(p, self.size, self.size)
            for p in get_handicap_positions(self.size, self.size, handicap)]

        return (True, ' '.join(positions), False)

    def _perform_command_play(self, args: List[str]) -> Tuple[bool, str, bool]:
        '''Execute play command.
        Args:
            args (List[str]): Argument list
        Returns:
            Tuple[bool, str, bool]: (True if successful, message, True to continue)
        '''
        # Check arguments
        if len(args) < 2:
            return (False, 'syntax error', False)

        color = gtp_string_to_color(args[0])

        if color is None:
            return (False, 'syntax error', False)

        pos = gtp_string_to_position(args[1], self.size, self.size)

        if pos is None:
            return (False, 'syntax error', False)

        # Create player object if not present
        if self.player is None:
            self.player = self._create_player()

        # If not pass, check if move is legal and position is valid
        if (is_valid_position(pos, self.size, self.size)
                and not self.player.get_board().is_enabled(pos, color)):
            return (False, 'illegal move', False)

        # Execute move
        try:
            self.player.play(pos, color)
            self.moves.append((pos, color))
        except GoException:
            return (False, 'illegal move', False)

        # Update display
        if self.display:
            self.display.play(pos, color)

        # Output log
        if LOGGER.isEnabledFor(logging.DEBUG):
            colors = self.player.get_board().get_colors()
            LOGGER.debug(
                'Played: color=%s, pos=%s\n%s',
                gtp_color_to_string(color), pos,
                get_array_string(colors, pos=pos))

        return (True, '', False)

    def _perform_command_undo(self, args: List[str],) -> Tuple[bool, str, bool]:
        if self.player is None:
            return (False, 'game has not started yet', False)
        elif self.moves == []:
            return (False, 'cannot undo', False)

        self.moves = self.moves[:-1]
        self.player.initialize()

        for pos, color in self.moves:
            self.player.play(pos, color)

        return (True, '', False)

    def _perform_command_genmove(
        self,
        args: List[str],
        play: bool = True,
    ) -> Tuple[bool, str, bool]:
        '''Execute genmove command.
        Args:
            args (List[str]): Argument list
            play (bool): True to execute move
        Returns:
            Tuple[bool, str, bool]: (True if successful, message, True to continue)
        '''
        # Create player object if not present
        if self.player is None:
            self.player = self._create_player()

        # Get color
        if (color := gtp_args_to_color(args)) is None:
            color = self.player.get_color()

        # Calculate move
        if len(self.moves) < self.initial_turn:
            candidates = [self._random_move(color)]
        else:
            candidates = self._evaluate(color)

        # Get move coordinates
        pos, score, territories = self._get_move(candidates)

        # If resigning or not advancing the board, return response
        if pos is None or not play:
            return (True, gtp_position_to_string(pos, self.size, self.size), False)

        # Move to the next board
        self.player.play(pos, color)
        self.moves.append((pos, color))

        if self.display:
            self.display.play(pos, color)

        # Output log
        if LOGGER.isEnabledFor(logging.DEBUG):
            colors = self.player.get_board().get_colors()
            LOGGER.debug(
                'Played: color=%s, pos=%s, score=%.1f\n%s',
                gtp_color_to_string(color), pos, score,
                get_array_string(colors, territories.argmax(axis=0) - 1, pos=pos))

        # Return string representing move coordinates
        return (True, gtp_position_to_string(pos, self.size, self.size), False)

    def _perform_command_reg_genmove(self, args: List[str]) -> Tuple[bool, str, bool]:
        return self._perform_command_genmove(args, play=False)

    def _perform_command_lz_genmove_analyze(
        self,
        args: List[str],
        analyze_func: Callable[
            [List[Candidate], np.ndarray, float, int, int, bool, bool], str,
        ] = lz_candidates_to_string,
        play: bool = True,
    ) -> Tuple[bool, str, bool]:
        '''Execute genmove_analyze command.
        Args:
            args (List[str]): Argument list
            analyze_func (Callable): Analysis function
            play (bool): True to execute move
        Returns:
            Tuple[bool, str, bool]: (True if successful, message, True to continue)
        '''
        # Create player object if not present
        if self.player is None:
            self.player = self._create_player()

        # Parse arguments
        color = self.player.get_color()
        interval = 1.0
        rootinfo = False
        ownership = False
        arg_idx = 0

        while arg_idx < len(args):
            arg = args[arg_idx]
            arg_idx += 1

            if arg.lower()[0] == 'b':
                color = BLACK
            elif arg.lower()[0] == 'w':
                color = WHITE
            elif arg.isdigit():
                interval = float(arg) / 100
            elif arg.lower() == 'rootinfo' and args[arg_idx].lower() == 'true':
                rootinfo = True
                arg_idx += 1
            elif arg.lower() == 'ownership' and args[arg_idx].lower() == 'true':
                ownership = True
                arg_idx += 1

        # Get list of candidate moves
        if play:
            if len(self.moves) < self.initial_turn:
                candidates = [self._random_move(color)]
            else:
                candidates = self._evaluate(color)
        else:
            candidates = self._evaluate(color, visits=100_000, timelimit=interval)

        # Create move coordinates
        pos, score, territories = self._get_move(candidates)

        # If pass, set final predicted score
        if pos == PASS:
            score = self.player.get_final_score()

        # Create analysis result string
        analyze_line = analyze_func(
            candidates, territories, score, self.size, self.size, rootinfo, ownership)

        # If not advancing the board, return only analysis result
        if not play:
            return (True, f'\n{analyze_line}', True)

        # If not resigning, advance the board
        if pos is not None:
            self.player.play(pos, color)
            self.moves.append((pos, color))

            if self.display:
                self.display.play(pos, color)

        # Return string representing move coordinates
        gtp_pos = gtp_position_to_string(pos, self.size, self.size)

        return (True, f'\n{analyze_line}\nplay {gtp_pos}', False)

    def _perform_command_lz_analyze(self, args: List[str]) -> Tuple[bool, str, bool]:
        return self._perform_command_lz_genmove_analyze(args, play=False)

    def _perform_command_kata_genmove_analyze(
        self, args: List[str], play: bool = True
    ) -> Tuple[bool, str, bool]:
        return self._perform_command_lz_genmove_analyze(
            args, kata_candidates_to_string, play=play)

    def _perform_command_kata_analyze(self, args: List[str]) -> Tuple[bool, str, bool]:
        return self._perform_command_kata_genmove_analyze(args, play=False)

    def _perform_command_cgos_genmove_analyze(
        self, args: List[str], play: bool = True,
    ) -> Tuple[bool, str, bool]:
        return self._perform_command_lz_genmove_analyze(
            args, cgos_candidates_to_string, play=play)

    def _perform_command_cgos_analyze(self, args: List[str]) -> Tuple[bool, str, bool]:
        return self._perform_command_cgos_genmove_analyze(args, play=False)

    def _perform_command_showboard(self, args: List[str]) -> Tuple[bool, str, bool]:
        return (True, '\n' + str(self), False)

    def _perform_command_time_settings(self, args: List[str]) -> Tuple[bool, str, bool]:
        if len(args) < 3:
            return (False, 'syntax error', False)

        self.remain_times[0] = int(args[0])
        self.remain_times[1] = int(args[0])

        return (True, '', False)

    def _perform_command_time_left(self, args: List[str]) -> Tuple[bool, str, bool]:
        if len(args) < 3:
            return (False, 'syntax error', False)

        color = gtp_string_to_color(args[0])
        remain_time = int(args[1])

        if color == BLACK:
            self.remain_times[0] = remain_time
        else:
            self.remain_times[1] = remain_time

        return (True, '', False)

    def _perform_command_final_status_list(self, args: List[str]) -> Tuple[bool, str, bool]:
        if len(args) < 1:
            return (False, 'syntax error', False)

        if self.player is None:
            return (False, 'game has not started yet', False)

        board = self.player.get_board()
        territory = self.player.get_territories()
        colors = board.get_colors()

        if args[0] == 'alive':
            values = (territory * colors) == 1
        elif args[0] == 'dead':
            values = (territory * colors) == -1
        elif args[0] == 'seki':
            values = ((territory == EMPTY) * colors) != 0
        else:
            return (False, 'invalid status string', False)

        positions = [
            gtp_position_to_string((x, y), self.size, self.size)
            for x, y in np.ndindex(self.size, self.size) if values[y, x]]
        texts = [' '.join(positions[i:i + 20]) for i in range(0, len(positions), 20)]

        return (True, '\n'.join(texts), False)

    def _perform_command_final_score(self, args: List[str]) -> Tuple[bool, str, bool]:
        if self.player is None:
            return (False, 'game has not started yet', False)

        score = self.player.get_final_score()

        if score >= 0:
            message = f'B+{score:.1f}'
        else:
            message = f'W+{-score:.1f}'

        return (True, message, False)

    def _perform_command_gogui_analyze_commands(self, args: List[str]) -> Tuple[bool, str, bool]:
        commands = [
            'bwboard/Analyze Territories/gogui_analyze_territory',
            'cboard/Analyze Values/gogui_analyze_values',
            'string/Analyze Value/gogui_analyze_value',
            'string/Name/name',
            'string/Version/version',
            'string/Protocol Version/protocol_version',
            'varc/Reg GenMove/reg_genmove %c',
            'string/Final Score/final_score',
        ]

        return (True, '\n'.join(sorted(commands)), False)

    def _perform_command_gogui_analyze_territory(self, args: List[str]) -> Tuple[bool, str, bool]:
        if self.player is None:
            return (False, 'game has not started yet', False)

        lines = [
            ' '.join(('N', 'B', 'W')[w] for w in v)
            for v in self.player.get_territories()]

        return (True, '\n' + '\n'.join(lines), False)

    def _perform_command_gogui_analyze_values(self, args: List[str]) -> Tuple[bool, str, bool]:
        if self.player is None:
            return (False, 'game has not started yet', False)

        values = self.player.get_values()
        text = ['']

        for vs in values:
            vs = [(int(max(v, 0) * 255), int(max(-v, 0) * 255)) for v in vs]
            text.append(' '.join('#{:02x}00{:02x}'.format(r, b) for r, b in vs))

        return (True, '\n'.join(text), False)

    def _perform_command_gogui_analyze_value(self, args: List[str]) -> Tuple[bool, str, bool]:
        if self.player is None:
            return (False, 'game has not started yet', False)

        value = self.player.get_pass().value

        return (True, '{:.4f}'.format(value), False)

    def __str__(self) -> str:
        if self.player is not None:
            board = self.player.get_board()
            captured_black = self.player.get_captured(BLACK)
            captured_white = self.player.get_captured(WHITE)
        else:
            board = Board(self.size, self.size)
            captured_black = 0
            captured_white = 0

        colors = board.get_colors()
        chars = [
            gtp_position_to_string((i, i), self.size, self.size) for i in range(self.size)]

        def mark(x: int, y: int, c: int) -> str:
            if c == BLACK:
                return 'X'
            elif c == WHITE:
                return 'O'
            elif (x - 3) % 6 == 0 and (y - 3) % 6 == 0:
                return '+'
            else:
                return '.'

        texts = ['   ' + ' '.join(chars[x][0] for x in range(self.size))]

        for y in range(self.size):
            lnum = '{:>2s} '.format(chars[y][1:])
            rnum = ' {:<2s}'.format(chars[y][1:])
            line = ' '.join(mark(x, y, colors[y, x]) for x in range(self.size))
            texts.append(lnum + line + rnum)

        texts[-2] += f'    WHITE (O) has captured {captured_black} stones'
        texts[-1] += f'    BLACK (X) has captured {captured_white} stones'

        texts.append('   ' + ' '.join(chars[x][0] for x in range(self.size)))

        return '\n'.join(texts)
