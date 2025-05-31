from typing import List, Tuple

from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.vector cimport vector
from libcpp.pair cimport pair

include "processor.pyx"

cdef extern from "cpp/Candidate.h" namespace "deepgo":
    cdef cppclass Candidate:
        Candidate(int32_t, int32_t, float, float) except +
        int32_t getX()
        int32_t getY()
        int32_t getColor()
        int32_t getVisits()
        float getPolicy()
        float getValue()
        vector[pair[int32_t, int32_t]] getVariations()


cdef extern from "cpp/Player.h" namespace "deepgo":
    cdef cppclass Player:
        Player(Processor*, int32_t, int32_t, int32_t, float, int, bool) except +
        void clear()
        int32_t play(int32_t, int32_t)
        vector[Candidate] getPass() nogil
        vector[Candidate] getRandom(float) nogil
        void startEvaluation(int32_t, bool, bool, int32_t)
        void stopEvaluation()
        void waitEvaluation(float) nogil
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
        '''
        self.player = new Player(
            processor.processor, threads, width, height, komi, rule, superko)

    def __dealloc__(self):
        del self.player

    def clear(self) -> None:
        '''対戦の状態を初期状態に戻す。
        '''
        self.player.clear()

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
    ) -> Tuple[Tuple[int, int], int, float, float, float, List[Tuple[int, int]]]:
        '''パスの候補手を取得する。
        Returns:
            Tuple[Tuple[int, int], int, float, float, float, List[Tuple[int, int]]]: 候補手
        '''
        cdef vector[Candidate] candidates

        with nogil:
            candidates = self.player.getPass()

        return (
            (candidates[0].getX(), candidates[0].getY()), candidates[0].getColor(),
             candidates[0].getVisits(), candidates[0].getPolicy(), candidates[0].getValue(),
             candidates[0].getVariations(),
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
             candidates[0].getVisits(), candidates[0].getPolicy(), candidates[0].getValue(),
             candidates[0].getVariations(),
        )

    def start_evaluation(
        self,
        visits: int,
        equally: bool,
        use_ucb1: bool,
        width: int,
    ) -> None:
        '''評価を開始する。
        Args:
            visits (int): 訪問数
            equality (int): 探索回数を均等にするならTrue、UCB1かPUCBを使用するならFalse
            use_ucb1 (int): 探索先の基準としてUCB1を使用するならTrue、PUCBを使用するならFalse
            width (int): 探索幅(0ならば探索幅を制限しない)
        '''
        self.player.startEvaluation(visits, equally, use_ucb1, width)

    def stop_evaluation(self) -> None:
        '''評価を停止する。
        '''
        self.player.stopEvaluation()

    def wait_evaluation(self, timelimit: float) -> None:
        '''探索がが終了するまで待機する。
        Args:
            timelimit (float): 時間制限
        '''
        cdef float timelimit_float = timelimit

        with nogil:
            self.player.waitEvaluation(timelimit_float)

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
             candidates[i].getVisits(), candidates[i].getPolicy(),
             candidates[i].getValue(), candidates[i].getVariations(),
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
