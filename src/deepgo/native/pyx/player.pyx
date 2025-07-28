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
        '''プレイヤオブジェクトを初期化する。
        Args:
            processor (NativeProcessor): プロセッサオブジェクト
            threads (int): スレッド数
            width (int): 盤面の幅
            height (int): 盤面の高さ
            komi (float): コミの目数
            rule (int): 勝敗の判定ルール
            superko (bool): スーパーコウルールを適用するならTrue
            eval_leaf_only (bool): 葉ノードのみを評価対象とするならTrue
        '''
        self.player = new Player(
            processor.processor, threads,
            width, height, komi, rule, superko, eval_leaf_only)

    def __dealloc__(self):
        del self.player

    def initialize(self) -> None:
        '''対戦の状態を初期状態に戻す。
        '''
        self.player.initialize()

    def play(self, pos: Tuple[int, int]) -> int:
        '''指定された座標に石を打つ。
        Args:
            pos (Tuple[int, int]): 石を打つ座標
        Returns:
            int: 打ち上げた石の数
        '''
        return self.player.play(pos[0], pos[1])

    def get_pass(
        self,
    ) -> Tuple[Tuple[int, int], int, int, int, float, float, List[Tuple[int, int]]]:
        '''パスの候補手を取得する。
        Returns:
            Tuple[Tuple[int, int], int, int, int, float, float, List[Tuple[int, int]]]: 候補手
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
        '''ランダムに候補手を選択する。
        Args:
            temperature (float): 温度
        Returns:
            Tuple[Tuple[int, int], int, float, float, float, List[Tuple[int, int]]]: 候補手
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
        '''評価を開始する。
        Args:
            equality (int): 探索回数を均等にするならTrue、UCB1かPUCBを使用するならFalse
            use_ucb1 (int): 探索先の基準としてUCB1を使用するならTrue、PUCBを使用するならFalse
            width (int): 探索幅(0ならば探索幅を制限しない)
            temperature (float): 探索の温度パラメータ
            noise (float): 探索のガンベルノイズの強さ
        '''
        self.player.startEvaluation(equally, use_ucb1, width, temperature, noise)

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
    ) -> List[Tuple[Tuple[int, int], int, float, float, List[Tuple[int, int]]]]:
        '''候補手の一覧を取得する。
        Returns:
            List[Tuple[Tuple[int, int], int, float, float, float, List[Tuple[int, int]]]]: 候補手の一覧
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
