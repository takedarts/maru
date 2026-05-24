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
        '''プレイヤオブジェクトを初期化する。
        Args:
            processor (NativeInferenceProcessor): 推論プロセッサオブジェクト
            threads (int): スレッド数
            max_visits (int): 最大訪問数
            width (int): 盤面の幅
            height (int): 盤面の高さ
            komi (float): コミの目数
            rule (int): 勝敗の判定ルール
            superko (bool): スーパーコウルールを適用するならTrue
            pucb_constant_init (float): PUCBの信頼上限に掛ける定数の初期値
            pucb_constant_base (float): PUCBの信頼上限に掛ける定数の変化値
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
        '''対戦の状態を初期状態に戻す。'''
        self.player.initialize()

    def play(self, pos: Tuple[int, int], color: int) -> int:
        '''指定された座標に石を打つ。
        Args:
            pos (Tuple[int, int]): 石を打つ座標
            color (int): 石の色
        Returns:
            int: 打ち上げた石の数
        '''
        return self.player.play(Move(pos[0], pos[1], color))

    def get_pass(
        self,
    ) -> Tuple[
            Tuple[int, int], int, int, int, float, float,
            List[Tuple[Tuple[int, int], int]], np.ndarray]:
        '''パスの候補手を取得する。
        Returns:
            Tuple[Tuple[int, int], int, int, int, float, float,
                  List[Tuple[Tuple[int, int], int]], np.ndarray]: 候補手
        '''
        cdef vector[Candidate] candidates
        cdef Candidate candidate
        cdef np.ndarray[np.float32_t, ndim=1, mode='c'] territories

        with nogil:
            candidates = self.player.getPass()
            candidate = candidates[0]

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
        '''評価を開始する。
        Args:
            equally (bool): 探索回数を均等にするならTrue
            candidate_width (int): 候補手の探索幅
            temperature (float): 探索の温度パラメータ
            noise (float): 探索のガンベルノイズの強さ
        '''
        self.player.startEvaluation(equally, candidate_width, temperature, noise)

    def wait_evaluation(self, visits: int, playouts: int, timelimit: float, stop: bool) -> None:
        '''指定された訪問数とプレイアウト数になるまで待機する。
        Args:
            visits (int): 訪問数
            playouts (int): プレイアウト数
            timelimit (float): 時間制限（秒）
            stop (bool): 探索を停止するならばTrue
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
        '''候補手の一覧を取得する。
        Returns:
            List[Tuple[Tuple[int, int], int, int, int, float, float,
                 List[Tuple[Tuple[int, int], int]], np.ndarray]]: 候補手
        '''
        cdef vector[Candidate] candidates
        cdef np.ndarray[np.float32_t, ndim=1, mode='c'] territories

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
        '''次に打つ石の色を取得する。
        Returns:
            int: 石の色
        '''
        return self.player.getColor()

    def get_board_state(self) -> List[int]:
        '''盤面の状態を取得する。
        Returns:
            List[int]: 盤面の状態
        '''
        return self.player.getBoardState()

    def to_string(self) -> str:
        '''探索木の文字列表現を取得する。
        Returns:
            str: 文字列表現
        '''
        return self.player.toString().decode('utf-8')
