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
    '''GTPの文字列表現を座標値に変換する
    パスの場合は(-1, -1)を返し、投了の場合はNoneを返す。
    Args:
        s (str): GTPの文字列表現
        width (int): 盤面の幅
        height (int): 盤面の高さ
    Returns:
        Tuple[int, int]: 座標値
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
    '''文字列表現を石の色に変換する
    Args:
        s (str): 文字列表現
    Returns:
        int | None: 石の色
    '''
    s = s.lower().strip()

    if s[0] == 'b':
        return BLACK
    elif s[0] == 'w':
        return WHITE
    else:
        return None


def gtp_position_to_string(p: Tuple[int, int] | None, width: int, height: int) -> str:
    '''座標値をGTPの文字列表現に変換する
    Args:
        p (Tuple[int, int] | None): 座標値
        width (int): 盤面の幅
        height (int): 盤面の高さ
    Returns:
        str: GTPの文字列表現
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
    '''石の色を文字列表現に変換する
    Args:
        c (int): 石の色
    Returns:
        str: 文字列表現
    '''
    if c == BLACK:
        return 'black'
    elif c == WHITE:
        return 'white'
    else:
        raise GoException(f'{c} is not color')


def gtp_args_to_color(args: List[str]) -> int | None:
    '''引数リストから色を取得する
    Args:
        args (List[str]): 引数リスト
    Returns:
        int | None: 石の色
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
    '''候補手をLeelaZeroの文字列表現に変換する
    Args:
        order (int): 優先順位
        candidate (Candidate): 候補手
        width (int): 盤面の幅
        height (int): 盤面の高さ
    Returns:
        str: LeelaZeroの文字列表現
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
    '''候補手の一覧をLeelaZeroの文字列表現に変換する
    Args:
        candidates (List[Candidate]): 候補手の一覧
        territories (np.ndarray): 領域のデータ（ここでは使用しない）
        score (float): 予想目数差
        width (int): 盤面の幅
        height (int): 盤面の高さ
    Returns:
        str: LeelaZeroの文字列表現
    '''
    return ' '.join(
        lz_candidate_to_string(o, c, width, height) for o, c in enumerate(candidates))


def kata_candidate_to_string(
    order: int,
    candidate: Candidate,
    width: int,
    height: int
) -> str:
    '''候補手をKataGoの文字列表現に変換する
    Args:
        order (int): 優先順位
        candidate (Candidate): 候補手
        width (int): 盤面の幅
        height (int): 盤面の高さ
    Returns:
        str: KataGoの文字列表現
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
    '''候補手の一覧をKataGoの文字列表現に変換する
    Args:
        candidates (List[Candidate]): 候補手の一覧
        territories (np.ndarray): 領域のデータ
        score (float): 予想目数差
        width (int): 盤面の幅
        height (int): 盤面の高さ
    Returns:
        str: KataGoの文字列表現
    '''
    # 候補手の文字列を作成する
    candidates_text = ' '.join(
        kata_candidate_to_string(o, c, width, height) for o, c in enumerate(candidates))

    # rootInfoの文字列を作成する
    win_chance = candidates[0].win_chance
    visits = sum(c.visits for c in candidates)
    score = score if candidates[0].color == BLACK else -score
    root_text = (f'rootInfo winrate {win_chance:.4f} visits {visits} scoreLead {score:.1f}')

    # 領域の文字列を作成する
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
    '''候補手の一覧をCGOSの文字列表現に変換する
    Args:
        candidates (List[Candidate]): 候補手の一覧
        territories (np.ndarray): 領域のデータ
        score (float): 予想目数差
        width (int): 盤面の幅
        height (int): 盤面の高さ
    Returns:
        str: CGOSの文字列表現
    '''
    # 候補手のデータを格納する変数
    root_values: Dict[str, Any] = {}

    # rootInfoの値を設定する
    root_values['winrate'] = candidates[0].win_chance
    root_values['score'] = score if candidates[0].color == BLACK else -score
    root_values['visits'] = sum(c.visits for c in candidates)

    # 候補手の文字列を作成する
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

    # 領域の値を設定する
    def territory_to_string(t: np.ndarray) -> str:
        c = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+'
        v = max(float(t[2] - t[1]), 0) - max(float(t[0] - t[1]), 0)
        v = v if candidates[0].color == BLACK else -v
        return c[round(max(0, min((v + 1) / 2, 1)) * 62)]

    root_values['ownership'] = ''.join(
        territory_to_string(territories[:, y, x]) for y, x in np.ndindex(height, width))

    return json.dumps(root_values)


class Display(object):
    '''盤面を表示するクラス。'''

    def __init__(self, cmd: str) -> None:
        '''盤面表示オブジェクトを初期化する。
        Args:
            cmd (str): 盤面表示アプリケーションを実行するコマンド
        '''
        self._pipe = subprocess.Popen(
            cmd.split(), stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self.send('clear_board')

    def wait(self) -> None:
        '''表示処理が終了するまで待機する。'''
        self._pipe.wait()

    def close(self) -> None:
        '''表示処理を終了する。'''
        self._pipe.terminate()

    def play(self, pos: Tuple[int, int], color: int) -> None:
        '''盤面に石を表示する。
        Args:
            pos (Tuple[int, int]): 石を置く座標
            color (int): 置く石の色
        '''
        pos_str = gtp_position_to_string(pos, 19, 19)
        color_str = 'b' if color == BLACK else 'w'

        self.send(f'play {pos_str} {color_str}')

    def send(self, message: str) -> None:
        '''表示命令を送信する。
        Args:
            message (str): 表示命令
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
    '''GTPエンジンとして囲碁AIを実行するためのクラス'''

    def __init__(
        self,
        processor: Processor,
        threads: int,
        visits: int,
        use_ucb1: bool,
        rule: int = RULE_CH,
        boardsize: int = DEFAULT_SIZE,
        komi: float = DEFAULT_KOMI,
        superko: bool = False,
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
        '''GTPエンジンを作成する。
        Args:
            processor (Processor): 推論実行オブジェクト
            threads (int): 使用するスレッドの数
            visits (int): 訪問数の目標値
            use_ucb1 (bool): UCB1を使用するならTrue、PUCBを使用するならFalse
            rule (int): ゲームルール
            komi: float: コミの値
            superko (bool): スーパーコウルールを適用するならTrue
            timelimit (float): 思考時間の上限
            ponder (bool): 相手の思考中に解析を継続するならTrue
            resign_threshold (float): 投了するときの勝率
            resign_score (float): 投了するときの目数差
            resign_turn (int): 投了するまでの最低ターン数
            initial_turn (int): ランダム着手する初期ターン数
            client_name (str): クライアントの表示名
            client_version (str): クライアントの表示バージョン
            reader (TextIO): 命令を入力するストリーム
            writer (TextIO): 結果を出力するストリーム
            display (str | None): 表示コマンド
        '''
        self.processor = processor
        self.threads = threads
        self.player: Player | None = None
        self.moves: List[Tuple[Tuple[int, int], int]] = []

        self.visits = visits
        self.use_ucb1 = use_ucb1

        self.size = boardsize
        self.komi = komi
        self.rule = rule
        self.superko = superko
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
        '''SGFファイルを読み込む。
        Args:
            sgf (str | Path): SGFファイルのパス
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
        '''GTPモードでエンジンを実行する。'''
        regex = re.compile(r'^(\d+)\s+(.*)$')

        while not self.reader.closed and not self.writer.closed:
            try:
                # コマンドを読み込む
                command = self.reader.readline()

                # 文字がない（読み込むデータが存在しない）場合は終了する
                if len(command) == 0:
                    break

                # 改行文字を削除する
                command = command.strip()

                # 空文字列だった場合は読み込みを継続する
                if len(command) == 0:
                    continue

                # 実行中のスレッドがあれば終了する
                if self.thread is not None:
                    self.terminated = True
                    self.thread.join()

                # ログを出力する
                LOGGER.debug('GTP command: %s', command)

                # コマンドを解析する
                number = ''

                if match := regex.match(command):
                    number = match.group(1).strip()
                    command = match.group(2).strip()

                # 終了命令ならループを抜ける
                if command.lower().startswith('quit'):
                    self.thread = None
                    self.writer.write(f'={number}\n')
                    break

                # 別スレッドでコマンドを実行する
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
        '''プレイヤオブジェクトを作成する
        Returns:
            Player: プレイヤオブジェクト
        '''
        LOGGER.debug(
            'Create player: width=%d, height=%d, komi=%.1f, rule=%d, superko=%s',
            self.size, self.size, self.komi, self.rule, self.superko)

        return Player(
            processor=self.processor,
            threads=self.threads,
            width=self.size,
            height=self.size,
            komi=self.komi,
            rule=self.rule,
            superko=self.superko)

    def _random_move(self, color: int) -> Candidate:
        '''Policyに基づいてランダムに着手する。
        Args:
            color (int): 着手する色
        Returns:
            Candidate: 候補手
        '''
        # プレイヤオブジェクトがない場合は作成する
        if self.player is None:
            self.player = self._create_player()

        # 着手する色をあわせる
        if self.player.get_color() != color:
            self.player.play(PASS)

        # 候補手を取得する
        LOGGER.debug('Random: color=%s', gtp_color_to_string(color))
        candiate = self.player.get_random()

        # 候補手を返す
        return candiate

    def _evaluate(
        self,
        color: int,
        visits: int | None = None,
        timelimit: float | None = None,
    ) -> List[Candidate]:
        '''盤面の評価を実行する。
        Args:
            color (int): 着手する色
            visits (int | None): 訪問数の目標値
            timelimit (float | None): 思考時間の上限
        Returns:
            List[Candidate]: 候補手の一覧
        '''
        # プレイヤオブジェクトがない場合は作成する
        if self.player is None:
            self.player = self._create_player()

        # 着手する色をあわせる
        if self.player.get_color() != color:
            self.player.play(PASS)

        # 訪問数と思考時間を設定する
        if visits is None:
            visits = self.visits

        if timelimit is None:
            timelimit = self._get_timelimit(color)

        # 盤面を評価する
        LOGGER.debug(
            'Evaluate: color=%s, visits=%d, timelimit=%.1f',
            gtp_color_to_string(color), visits, timelimit)

        candidates = self.player.evaluate(
            visits=visits,
            use_ucb1=self.use_ucb1,
            timelimit=timelimit)

        # 候補手の一覧を返す
        return candidates

    def _get_move(
        self,
        candidate: Candidate,
    ) -> Tuple[Tuple[int, int] | None, float, np.ndarray]:
        '''着手座標を取得する。
        投了の場合は座標としてNoneを返す。
        Args:
            candidate (Candidate): 候補手
        Returns:
            Tuple[int, int]: 着手座標, 予想目数差, 予想領域
        '''
        # playerオブジェクトを確認する
        if self.player is None:
            raise GoException('Game has not started yet')

        # 着手した後の予想領域を取得する
        board = self.player.get_board()
        territories = self.player.get_territories(
            pos=candidate.pos, color=candidate.color, raw=True)
        score = territories[2].sum() - territories[0].sum()
        score += (board.get_colors() * territories[1]).sum()
        score -= self.komi

        # 日本ルールですべての境界領域に着手している場合はパスを検討する
        # パスしたときの予測目数差と着手時の予測目数差に変化がない場合はパスする
        # ただし、それまでにパスが存在している場合は境界領域の確認は行わない
        if self.rule == RULE_JP:
            boundary_fixed = True

            # それまでの着手にパスが存在しない場合、すべての境界領域が確定しているかを確認する
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

            # すべての境界領域が確定している場合はパスした場合の予測目数差を確認する
            # 予測目数差が閾値(0.8)未満の場合はパスする
            if boundary_fixed:
                pass_territories = self.player.get_territories(
                    pos=PASS, color=candidate.color, raw=True)
                pass_score = pass_territories[2].sum() - pass_territories[0].sum()
                pass_score += (board.get_colors() * pass_territories[1]).sum()
                pass_score -= self.komi

                score_diff = score - pass_score
                score_diff = score_diff if candidate.color == BLACK else -score_diff

                if score_diff < 0.8:
                    return PASS, pass_score, pass_territories

        # 指定ターン未満なら投了しない
        if self.player.turn < self.resign_turn:
            return candidate.pos, score, territories

        # 目数差が閾値未満なら投了しない
        if abs(score) < self.resign_score:
            return candidate.pos, score, territories

        # 勝率が閾値未満なら投了する
        if candidate.win_chance < self.resign_threshold:
            return None, score, territories

        # 投了しない場合は着手座標を返す
        return candidate.pos, score, territories

    def _perform(self, number: str, command: str) -> None:
        '''命令を実行する。
        Args:
            number (str): 命令番号
            command (str): 命令
        '''
        first_response = True

        while True:
            try:
                # 処理を実行する
                stat, message, cont = self._perform_command(command)

                # 最初の応答の場合は応答マークを出力する
                if first_response:
                    first_response = False
                    mark = '=' if stat else '?'
                    header = f'{mark}{number} '
                else:
                    header = ''

                # 応答を出力する
                response = f'{header}{message}'
                LOGGER.debug('GTP response: %s', response)
                self.writer.write(response)
                self.writer.flush()

                # 実行を継続しない場合は改行を出力してループを抜ける
                if not cont or self.terminated:
                    self.writer.write('\n\n')
                    self.writer.flush()
                    break
            except BaseException as e:
                LOGGER.error('GTP error: %s', str(e))
                return

    def _perform_command(self, command: str) -> Tuple[bool, str, bool]:
        '''コマンドを実行する。
        Args:
            command (str): コマンド文字列(引数を含む)
        Returns:
            Tuple[bool, str, bool]: (成功ならTrue, メッセージ, 実行を継続するならTrue)
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
            return (False, 'can not change komi after the game starts', False)

        self.komi = new_komi

        return (True, '', False)

    def _perform_command_play(self, args: List[str]) -> Tuple[bool, str, bool]:
        '''playコマンドを実行する。
        Args:
            args (List[str]): 引数リスト
        Returns:
            Tuple[bool, str, bool]: (成功ならTrue, メッセージ, 継続するならTrue)
        '''
        # 引数を確認する
        if len(args) < 2:
            return (False, 'syntax error', False)

        color = gtp_string_to_color(args[0])

        if color is None:
            return (False, 'syntax error', False)

        pos = gtp_string_to_position(args[1], self.size, self.size)

        if pos is None:
            return (False, 'syntax error', False)

        # プレイヤオブジェクトがない場合は作成する
        if self.player is None:
            self.player = self._create_player()

        # パスでない場合は着手できるか座標であることを確認する
        if (is_valid_position(pos, self.size, self.size)
                and not self.player.get_board().is_enabled(pos, color)):
            return (False, 'illegal move', False)

        # 着手を実行する
        try:
            self.player.play(pos, color)
            self.moves.append((pos, color))
        except GoException:
            return (False, 'illegal move', False)

        # 表示を更新する
        if self.display:
            self.display.play(pos, color)

        # ログを出力する
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
        self.player.clear()

        for pos, color in self.moves:
            self.player.play(pos, color)

        return (True, '', False)

    def _perform_command_genmove(
        self,
        args: List[str],
        play: bool = True,
    ) -> Tuple[bool, str, bool]:
        '''genmoveコマンドを実行する。
        Args:
            args (List[str]): 引数リスト
            play (bool): 着手を実行するならTrue
        Returns:
            Tuple[bool, str, bool]: (成功ならTrue, メッセージ, 実行を継続するならTrue)
        '''
        # プレイヤオブジェクトがない場合は作成する
        if self.player is None:
            self.player = self._create_player()

        # 色を取得する
        if (color := gtp_args_to_color(args)) is None:
            color = self.player.get_color()

        # 着手を計算する
        if len(self.moves) < self.initial_turn:
            candidate = self._random_move(color)
        else:
            candidate = self._evaluate(color)[0]

        # 着手座標を取得する
        pos, score, territories = self._get_move(candidate)

        # 投了の場合と盤面を進めない場合は着手座標を返す
        if pos is None or not play:
            return (True, gtp_position_to_string(pos, self.size, self.size), False)

        # 盤面を進める
        self.player.play(pos, color)
        self.moves.append((pos, color))

        if self.display:
            self.display.play(pos, color)

        # ログを出力する
        if LOGGER.isEnabledFor(logging.DEBUG):
            colors = self.player.get_board().get_colors()
            territories = self.player.get_territories()
            LOGGER.debug(
                'Played: color=%s, pos=%s, score=%.1f\n%s',
                gtp_color_to_string(color), pos, score,
                get_array_string(colors, territories, pos=pos))

        # 着手座標を表す文字列を返す
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
        '''genmove_analyzeコマンドを実行する。
        Args:
            args (List[str]): 引数リスト
            analyze_func (Callable): 解析関数
            play (bool): 着手を実行するならTrue
        Returns:
            Tuple[bool, str, bool]: (成功ならTrue, メッセージ, 実行を継続するならTrue)
        '''
        # プレイヤオブジェクトがない場合は作成する
        if self.player is None:
            self.player = self._create_player()

        # 引数を解析する
        color = self.player.get_color()
        interval = 1.0

        for arg in args:
            if arg.lower()[0] == 'b':
                color = BLACK
            elif arg.lower()[0] == 'w':
                color = WHITE
            elif arg.isdigit():
                interval = float(arg) / 100

        # 候補手の一覧を取得する
        if play:
            if len(self.moves) < self.initial_turn:
                candidates = [self._random_move(color)]
            else:
                candidates = self._evaluate(color)
        else:
            candidates = self._evaluate(color, visits=100_000, timelimit=interval)

        # 着手座標を作成する
        pos, score, territories = self._get_move(candidates[0])

        # パスの場合は最終予想目数を設定する
        if pos == PASS:
            score = self.player.get_final_score()

        # 解析結果の文字列を作成する
        analyze_line = analyze_func(candidates, territories, score, self.size, self.size)

        # 盤面を進めない場合は解析結果のみを返す
        if not play:
            return (True, f'\n{analyze_line}', True)

        # 投了でない場合は盤面を進める
        if pos is not None:
            self.player.play(pos, color)
            self.moves.append((pos, color))

            if self.display:
                self.display.play(pos, color)

        # 着手座標を表す文字列を返す
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
