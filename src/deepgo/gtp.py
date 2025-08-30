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
                    get_handicap_positions, is_valid_position)
from .config import (BLACK, DEFAULT_KOMI, DEFAULT_SIZE, EMPTY, MODEL_SIZE,
                     NAME, PASS, RULE_CH, RULE_JP, VERSION, WHITE)
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
    height: int
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
            gtp_position_to_string(p, width, height) for p in candidate.variations)
        candidate_text += f' pv {variations}'

    return candidate_text.strip()


def lz_candidates_to_string(
    candidates: List[Candidate],
    territories: np.ndarray,
    score: float,
    width: int,
    height: int,
) -> str:
    '''Convert list of candidate moves to LeelaZero string representation.
    Args:
        candidates (List[Candidate]): List of candidate moves
        territories (np.ndarray): Territory data (not used here)
        score (float): Predicted score difference
        width (int): Board width
        height (int): Board height
    Returns:
        str: LeelaZero string representation
    '''
    return ' '.join(
        lz_candidate_to_string(o, c, width, height) for o, c in enumerate(candidates))


def kata_candidate_to_string(
    order: int,
    candidate: Candidate,
    width: int,
    height: int
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
            gtp_position_to_string(p, width, height) for p in candidate.variations)
        candidate_text += f' pv {variations}'

    return candidate_text.strip()


def kata_candidates_to_string(
    candidates: List[Candidate],
    territories: np.ndarray,
    score: float,
    width: int,
    height: int
) -> str:
    '''Convert list of candidate moves to KataGo string representation.
    Args:
        candidates (List[Candidate]): List of candidate moves
        territories (np.ndarray): Territory data
        score (float): Predicted score difference
        width (int): Board width
        height (int): Board height
    Returns:
        str: KataGo string representation
    '''
    # Create candidate move string
    candidates_text = ' '.join(
        kata_candidate_to_string(o, c, width, height) for o, c in enumerate(candidates))

    # Create rootInfo string
    win_chance = candidates[0].win_chance
    visits = sum(c.visits for c in candidates)
    score = score if candidates[0].color == BLACK else -score
    root_text = (f'rootInfo winrate {win_chance:.4f} visits {visits} scoreLead {score:.1f}')

    # Create territory string
    def territory_to_string(t: np.ndarray) -> str:
        v = max(float(t[2] - t[1]), 0) - max(float(t[0] - t[1]), 0)
        return f'{v:.2f}' if candidates[0].color == BLACK else f'{-v:.2f}'

    ownership_values = ' '.join(
        territory_to_string(territories[:, y, x]) for y, x in np.ndindex(height, width))
    ownership_text = f'ownership {ownership_values}'

    return f'{candidates_text} {root_text} {ownership_text}'


def cgos_candidates_to_string(
    candidates: List[Candidate],
    territories: np.ndarray,
    score: float,
    width: int,
    height: int
) -> str:
    '''Convert list of candidate moves to CGOS string representation.
    Args:
        candidates (List[Candidate]): List of candidate moves
        territories (np.ndarray): Territory data
        score (float): Predicted score difference
        width (int): Board width
        height (int): Board height
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
            gtp_position_to_string(p, width, height) for p in candidate.variations)

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
        use_ucb1: bool = False,
        temperature: float = 1.0,
        randomness: float = 0.0,
        criterion: str = 'lcb',
        rule: int = RULE_CH,
        boardsize: int = DEFAULT_SIZE,
        komi: float = DEFAULT_KOMI,
        superko: bool = False,
        eval_leaf_only: bool = False,
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
            use_ucb1 (bool): True to use UCB1, False to use PUCB
            criterion (str): Candidate move priority criterion ('lcb' or 'visits')
            temperature (float): Search temperature parameter
            randomness (float): Randomness of search visits
            rule (int): Game rule
            komi: float: Komi value
            superko (bool): True to apply superko rule
            eval_leaf_only (bool): True to evaluate only leaf nodes
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
        self.use_ucb1 = use_ucb1
        self.temperature = temperature
        self.randomness = randomness
        self.criterion = criterion

        self.rule = rule
        self.size = boardsize
        self.komi = komi
        self.superko = superko
        self.eval_leaf_only = eval_leaf_only
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
            'width=%d, height=%d, komi=%.1f, rule=%d, superko=%s, eval_leaf_only=%s',
            self.size, self.size, self.komi, self.rule, self.superko, self.eval_leaf_only)

        return Player(
            processor=self.processor,
            threads=self.threads,
            width=self.size,
            height=self.size,
            komi=self.komi,
            rule=self.rule,
            superko=self.superko,
            eval_leaf_only=self.eval_leaf_only
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
        candiate = self.player.get_random()

        # Return candidate move
        return candiate

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
            use_ucb1=self.use_ucb1,
            timelimit=timelimit,
            temperature=self.temperature,
            criterion=self.criterion,
            ponder=self.ponder)

        # Return list of candidate moves
        return candidates

    def _get_move(
        self,
        candidate: Candidate,
    ) -> Tuple[Tuple[int, int] | None, float, np.ndarray]:
        '''Get move coordinates.
        Returns None as coordinates in case of resignation.
        Args:
            candidate (Candidate): Candidate move
        Returns:
            Tuple[int, int]: Move coordinates, predicted score difference, predicted territory
        '''
        # playerオブジェクトを確認する
        # Check player object
        if self.player is None:
            raise GoException('Game has not started yet')

        # 着手した後の予想領域を取得する
        # Get predicted territory after move
        board = self.player.get_board()
        territories = self.player.get_territories(
            pos=candidate.pos, color=candidate.color, raw=True)
        score = territories[2].sum() - territories[0].sum()
        score += (board.get_colors() * territories[1]).sum()
        score -= self.komi

        # In Japanese rule, consider passing if all boundary territories have been fixed
        # If predicted score difference does not change between passing and playing, pass
        # However, if a pass has already occurred, do not check boundary territories
        if self.rule == RULE_JP:
            boundary_fixed = True

            # If no pass has occurred, check if all boundary territories are fixed
            if PASS not in [m[0] for m in self.moves]:
                owners = territories.argmax(axis=0) - 1
                colors = board.get_colors()

                for x, y in np.ndindex(self.size, self.size):
                    if colors[y, x] != EMPTY:
                        continue

                    for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                        nx = x + dx
                        ny = y + dy

                        if not is_valid_position((nx, ny), self.size, self.size):
                            continue
                        elif owners[y, x] != owners[ny, nx]:
                            boundary_fixed = False
                            break

                    if not boundary_fixed:
                        break

            # If all boundary territories are fixed, check predicted score difference for passing
            # If predicted score difference is below threshold (0.8), pass
            # (stop search if pondering)
            if boundary_fixed:
                pass_territories = self.player.get_territories(
                    pos=PASS, color=candidate.color, raw=True)
                pass_score = pass_territories[2].sum() - pass_territories[0].sum()
                pass_score += (board.get_colors() * pass_territories[1]).sum()
                pass_score -= self.komi

                score_diff = score - pass_score
                score_diff = score_diff if candidate.color == BLACK else -score_diff

                if score_diff < 0.8:
                    self.player.stop_evaluation()
                    return PASS, pass_score, pass_territories

        # Do not resign before specified turn
        if self.player.turn < self.resign_turn:
            return candidate.pos, score, territories

        # Do not resign if score difference is below threshold
        if abs(score) < self.resign_score:
            return candidate.pos, score, territories

        # Resign if win rate is below threshold (stop search if pondering)
        if candidate.win_chance < self.resign_threshold:
            self.player.stop_evaluation()
            return None, score, territories

        # If not resigning, return move coordinates
        return candidate.pos, score, territories

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
            territories = self.player.get_territories()
            LOGGER.debug(
                'Played: color=%s, pos=%s\n%s',
                gtp_color_to_string(color), pos,
                get_array_string(colors, territories, pos=pos))

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
            candidate = self._random_move(color)
        else:
            candidate = self._evaluate(color)[0]

        # Get move coordinates
        pos, score, territories = self._get_move(candidate)

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
            territories = self.player.get_territories()
            LOGGER.debug(
                'Played: color=%s, pos=%s, score=%.1f\n%s',
                gtp_color_to_string(color), pos, score,
                get_array_string(colors, territories, pos=pos))

        # Return string representing move coordinates
        return (True, gtp_position_to_string(pos, self.size, self.size), False)

    def _perform_command_reg_genmove(self, args: List[str]) -> Tuple[bool, str, bool]:
        return self._perform_command_genmove(args, play=False)

    def _perform_command_lz_genmove_analyze(
        self,
        args: List[str],
        analyze_func: Callable[
            [List[Candidate], np.ndarray, float, int, int], str] = lz_candidates_to_string,
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

        for arg in args:
            if arg.lower()[0] == 'b':
                color = BLACK
            elif arg.lower()[0] == 'w':
                color = WHITE
            elif arg.isdigit():
                interval = float(arg) / 100

        # Get list of candidate moves
        if play:
            if len(self.moves) < self.initial_turn:
                candidates = [self._random_move(color)]
            else:
                candidates = self._evaluate(color)
        else:
            candidates = self._evaluate(color, visits=100_000, timelimit=interval)

        # Create move coordinates
        pos, score, territories = self._get_move(candidates[0])

        # If pass, set final predicted score
        if pos == PASS:
            score = self.player.get_final_score()

        # Create analysis result string
        analyze_line = analyze_func(candidates, territories, score, self.size, self.size)

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
