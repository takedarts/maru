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


class Candidate(object):
    def __init__(
        self,
        pos: Tuple[int, int],
        color: int,
        visits: int,
        policy: float,
        value: float,
        variations: List[Tuple[int, int]],
    ) -> None:
        '''候補手オブジェクトを初期化する。
        Args:
            pos (Tuple[int, int]): 石を置く座標
            color (int): 石の色
            visits (int): 訪問回数
            policy (float): 予想される着手確率
            value (float): 予測される勝率
            variations (List[Tuple[int, int]]): 予測進行
        '''
        self.pos = pos
        self.color = color
        self.visits = visits
        self.policy = policy
        self.value = value
        self.variations = variations
        self.win_chance = value * color * 0.5 + 0.5
        self.win_chance_lcb = self.win_chance - 1.96 * 0.25 / (visits + 1)**0.5

    def __str__(self) -> str:
        return (
            f'Candidate(pos={self.pos}, color={get_color_name(self.color)},'
            f' visits={self.visits}, policy={self.policy:.2f}, value={self.value:.2f},'
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
    ) -> None:
        '''競技者オブジェクトを初期化する。
        Args:
            processor (Processor): 計算管理オブジェクト
            threads (int): 使用するスレッド数
            width (int): 盤面の幅
            height (int): 盤面の高さ
            komi (float): コミの数
            rule (int): 勝敗の判定ルール
            superko (bool): スーパーコウルールを適用するならTrue
        '''
        self.native = NativePlayer(
            processor=processor.native, threads=threads,
            width=width, height=height, komi=komi, rule=rule, superko=superko)
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
        '''盤面を初期化する。'''
        self.native.initialize()
        self.turn = 0
        self.value = 0.0
        self.moves = [0, 0]
        self.captureds = [0, 0]

    def set_handicap(self, handicap: int) -> None:
        '''置石を設定する。
        Args:
            handicap (int): 置石の数
        '''
        for pos in get_handicap_positions(self.width, self.height, handicap):
            if self.native.get_color() != BLACK:
                self.native.play(PASS)

            self.native.play(pos)
            self.moves[0] += 1

    def is_valid_position(self, pos: Tuple[int, int]) -> bool:
        '''指定した座標が有効かどうかを返す。
        Args:
            pos (Tuple[int, int]): 座標
        Returns:
            bool: 有効な座標ならTrue
        '''
        return is_valid_position(pos, self.width, self.height)

    def is_superko_move(self, pos: Tuple[int, int], color: int) -> bool:
        '''指定された座標がスーパーコウに該当する手かどうかを判定する。
        Args:
            pos (Tuple[int, int]): 座標
            color (int): 石の色
        Returns:
            bool: スーパーコウに該当する手ならTrue
        '''
        if not self.is_valid_position(pos):
            return False

        board = self.get_board()
        board.play(pos, color)

        return tuple(board.get_patterns()) in self.histories

    def get_cleanup_position(self, color: int) -> Tuple[int, int]:
        '''死に石を打ち上げる座標を返す。
        Args:
            color (int): 置く石の色
        Returns:
            Tuple[int, int]: 座標
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
        '''石を置く。
        Args:
            pos (Tuple[int, int]): 石を置く座標
            color (int | None): 石の色（指定しない場合は手番の色）
        Returns:
            int: 取られた石の数
        '''
        # 石の色が指定されていない場合は手番の色を使う
        if color is None:
            color = self.native.get_color()

        # 着手する石の色が異なる場合はパスする
        if self.native.get_color() != color:
            self.native.play(PASS)

        # 着手する
        captured = self.native.play(pos)
        self.turn += 1

        # 着手した石の数と取られた石の数を更新する
        move_count = int(self.is_valid_position(pos) and captured >= 0)

        if color == BLACK:
            self.moves[0] += move_count
            self.captureds[1] += captured
        elif color == WHITE:
            self.moves[1] += move_count
            self.captureds[0] += captured

        # 盤面の状態を履歴に登録する
        self.histories.add(tuple(self.get_board().get_patterns()))

        return captured

    def get_pass(self) -> Candidate:
        '''パスの候補手を返す。
        Returns:
            Candidate: パスの候補手
        '''
        self.native.start_evaluation(1, False, False, 0)
        self.native.wait_evaluation(120.0)
        self.native.stop_evaluation()

        return Candidate(*self.native.get_pass())

    def get_random(self, temperature: float = 0.0, allow_outermost: bool = True) -> Candidate:
        '''ランダムな着手を返す。
        Args:
            temperature (float): 温度パラメータ
            allow_outermost (bool): 端の着手を許可するならTrue
        Returns:
            Candidate: 候補手
        '''
        self.native.start_evaluation(False, False, 0, 1.0, 0.0)
        self.native.wait_evaluation(1, 0, 120.0, True)

        for _ in range(10):
            # ランダムに着手を選択する
            candidate = Candidate(*self.native.get_random(temperature))

            # ログを出力する
            LOGGER.debug(candidate)

            # 着手が有効ならループを抜ける
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
    ) -> List[Candidate]:
        '''盤面を評価する。
        Args:
            visits (int): 訪問数の目標値
            playouts (int): プレイアウト数の目標値
            timelimit (float): 制限時間（秒）
            equally (bool): 探索回数を均等にする場合はTrue、UCB1かPUCBを使う場合はFalse
            use_ucb1 (bool): 探索先の基準としてUCB1を使う場合はTrue、PUCBを使う場合はFalse
            width (int): 探索幅（この探索幅までの候補手について探索を実行する）
        Returns:
            List[Candidate]: 候補手の一覧
        '''
        width = width if width is not None else 0

        # 盤面を評価する
        self.native.start_evaluation(equally, use_ucb1, width, temperature, noise)
        self.native.wait_evaluation(visits, playouts, timelimit, not ponder)

        # 候補手の一覧を作成する
        candidates = [Candidate(*c) for c in self.native.get_candidates()]

        # スーパーコウに該当する候補手を削除する
        if self.superko:
            candidates = [
                c for c in candidates if not self.is_superko_move(c.pos, c.color)]

        # 候補手がない場合はパスを候補手に追加する
        if len(candidates) == 0:
            candidates.append(self.get_pass())

        # COMルールで死に石の打ち上げが残っている場合はパス座標を打ち上げ座標に置き換える
        if self.rule == RULE_COM:
            for i in range(len(candidates)):
                if not self.is_valid_position(candidates[i].pos):
                    candidates[i].pos = self.get_cleanup_position(candidates[i].color)

        # 候補手をLCBでソートする
        candidates.sort(key=lambda cand: cand.win_chance_lcb, reverse=True)

        # ログを出力する
        if LOGGER.isEnabledFor(logging.DEBUG):
            LOGGER.debug(
                'Evaluation: %d visits, %d playouts',
                sum(c.visits for c in candidates),
                sum(c.playouts for c in candidates))
            for candidate in candidates:
                LOGGER.debug(candidate)

        # 候補手の一覧を返す
        return candidates

    def get_territories(
        self,
        pos: Tuple[int, int] | None = None,
        color: int | None = None,
        raw: bool = False,
    ) -> np.ndarray:
        '''最終的な領域を予測する。
        着手座標を指定した場合は、その座標に石を置いた場合の領域を返す。
        Args:
            pos (Tuple[int, int]): 着手座標
            color (int): 着手色
            raw (bool): 生データを返すならTrue
        Returns:
            np.ndarray: 予測領域
        '''
        # 盤面データを取得する
        board = self.get_board()

        # 次の着手色を取得する
        if color is None:
            color = self.get_color()

        # 着手座標が指定されている場合は、その座標に石を置いた場合の領域を返す
        if pos is not None:
            board.play(pos, color)
            color = get_opposite_color(color)

        # 入力値を作成する
        x = board.get_inputs(color, self.komi, self.rule, self.superko)
        x = x.reshape(1, -1)

        # 領域評価を実行する
        y = self.processor.execute(x)

        # 出力値を整形する
        model_length = MODEL_SIZE * MODEL_SIZE
        begin_x = (MODEL_SIZE - self.width) // 2
        end_x = begin_x + self.width
        begin_y = (MODEL_SIZE - self.height) // 2
        end_y = begin_y + self.height

        t = y[0, 2 * model_length:5 * model_length]
        t = t.reshape(3, MODEL_SIZE, MODEL_SIZE)
        t = t[:, begin_y:end_y, begin_x:end_x]

        # 白番の場合は色を反転させる
        if color != BLACK:
            t = t[::-1]

        # すでに確定している領域データを反映する
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
        '''盤面の評価値を取得する。
        Returns:
            np.ndarray: 盤面の評価値
        '''
        board = self.get_board()

        # 入力値を作成する
        x = board.get_inputs(BLACK, self.komi, self.rule, self.superko)
        x = x.reshape(1, -1)

        # 評価値を取得する
        y = self.processor.execute(x)

        # 出力値を整形する
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
        '''次に置く石の色を返す。
        Returns:
            int: 石の色
        '''
        return self.native.get_color()

    def get_board(self) -> Board:
        '''盤面データを返す。
        Returns:
            Board: 盤面データ
        '''
        board = Board(self.width, self.height)
        board.load_state(self.native.get_board_state())
        return board

    def get_board_state(self) -> List[int]:
        '''盤面の状態を取得する。
        Returns:
            List[int]: 盤面の状態
        '''
        return self.native.get_board_state()

    def get_captured(self, color: int) -> int:
        '''指定した色の取られた石の数を返す。
        Args:
            color (int): 石の色
        Returns:
            int: 取られた石の数
        '''
        if color == BLACK:
            return self.captureds[0]
        elif color == WHITE:
            return self.captureds[1]
        else:
            return 0

    def get_final_score(self) -> float:
        '''最終的な得点を取得する。
        Returns:
            float: 得点
        '''
        # 石の色と領域予測を取得する
        colors = self.get_board().get_colors()
        territories = self.get_territories()

        # セキを反映する
        territories += (territories == 0).astype(np.int32) * colors

        # 目数を計算する
        result = float(territories.sum()) - self.komi

        if self.rule == RULE_JP:
            result -= self.moves[0] - self.moves[1]

        return result
