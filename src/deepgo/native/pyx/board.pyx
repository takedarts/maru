from typing import List, Tuple

from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.pair cimport pair
from libcpp.vector cimport vector

import numpy
cimport numpy

from deepgo.config import MODEL_INPUT_SIZE


cdef extern from "cpp/Board.h" namespace "deepgo":
    cdef cppclass Board:
        Board(int32_t, int32_t) except +
        int32_t getWidth()
        int32_t getHeight()
        int32_t play(int32_t, int32_t, int32_t)
        pair[int32_t, int32_t] getKo(int32_t)
        vector[pair[int32_t, int32_t]] getHistories(int32_t)
        int32_t getColor(int32_t, int32_t)
        void getColors(int32_t*, int32_t)
        int32_t getRenSize(int32_t, int32_t)
        int32_t getRenSpace(int32_t, int32_t)
        bool isShicho(int32_t, int32_t)
        bool isEnabled(int32_t, int32_t, int32_t, bool)
        void getEnableds(int32_t*, int32_t, bool)
        void getTerritories(int32_t*, int32_t)
        void getOwners(int32_t*, int32_t, int32_t)
        vector[int32_t] getPatterns()
        void getInputs(float*, int32_t, float, int32_t, bool)
        vector[int32_t] getState()
        void loadState(vector[int32_t])
        void copyFrom(Board*)


cdef class NativeBoard:
    cdef Board *board

    def __cinit__(self, width: int, height: int) -> None:
        '''盤面オブジェクトを作成する。
        Args:
            width (int): 盤面の幅
            height (int): 盤面の高さ
        '''
        self.board = new Board(width, height)

    def __dealloc__(self) -> None:
        '''盤面オブジェクトを破棄する。'''
        del self.board

    def get_width(self) -> int:
        '''盤面の幅を取得する。
        Returns:
            int: 盤面の幅
        '''
        return self.board.getWidth()

    def get_height(self) -> int:
        '''盤面の高さを取得する。
        Returns:
            int: 盤面の高さ
        '''
        return self.board.getHeight()

    def play(self, pos: Tuple[int, int], color: int) -> int:
        '''指定した位置に石を置く。
        Args:
            pos (Tuple[int, int]): 石を置く位置
            color (int): 石の色
        Returns:
            int: 取り上げた石の数（おけない場合は-1）
        '''
        return self.board.play(pos[0], pos[1], color)

    def get_ko(self, color: int) -> Tuple[int, int]:
        '''コウの位置を取得する。
        Args:
            color (int): コウの対象となる石の色
        Returns:
            Tuple[int, int]: コウの位置
        '''
        cdef pair[int32_t, int32_t] ko = self.board.getKo(color)
        return (ko.first, ko.second)

    def get_histories(self, color: int) -> List[Tuple[int, int]]:
        '''指定した色の着手履歴を取得する。
        Args:
            color (int): 石の色
        Returns:
            List[Tuple[int, int]]: 着手履歴
        '''
        cdef vector[pair[int32_t, int32_t]] moves = self.board.getHistories(color)
        return [(move.first, move.second) for move in moves]

    def get_color(self, pos: Tuple[int, int]) -> int:
        '''指定した位置の石の色を取得する。
        Args:
            pos (Tuple[int, int]): 石の色を取得する位置
        Returns:
            int: 石の色
        '''
        return self.board.getColor(pos[0], pos[1])

    def get_colors(self, color: int) -> numpy.ndarray:
        '''全体の石の色を取得する。
        Args:
            color (int): 基準とする石の色（WHITEを指定すると色を反転する）
        Returns:
            numpy.ndarray: 全体の石の色
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode="c"] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getColors(<int32_t*> &data[0], color)

        return data.reshape((height, width))

    def get_ren_size(self, pos: Tuple[int, int]) -> int:
        '''指定した位置の連の大きさを取得する。
        Args:
            pos (Tuple[int, int]): 連の大きさを取得する位置
        Returns:
            int: 連の大きさ
        '''
        return self.board.getRenSize(pos[0], pos[1])

    def get_ren_space(self, pos: Tuple[int, int]) -> int:
        '''指定した位置の連のダメの数を取得する。
        Args:
            pos (Tuple[int, int]): 連の空きを取得する位置
        Returns:
            int: 連のダメの数
        '''
        return self.board.getRenSpace(pos[0], pos[1])

    def is_shicho(self, pos: Tuple[int, int]) -> bool:
        '''指定した位置がシチョウかどうかを取得する。
        Args:
            pos (Tuple[int, int]): シチョウかどうかを取得する位置
        Returns:
            bool: シチョウならtrue
        '''
        return self.board.isShicho(pos[0], pos[1])

    def is_enabled(
        self,
        pos: Tuple[int, int],
        color: int,
        check_seki: bool,
    ) -> bool:
        '''指定した位置に石を置けるかどうかを取得する。
        Args:
            pos (Tuple[int, int]): 石を置く位置
            color (int): 石の色
            check_seki (bool): セキを確認するかどうか
        Returns:
            bool: 石を置けるならtrue
        '''
        return self.board.isEnabled(pos[0], pos[1], color, check_seki)

    def get_enableds(self, color: int, check_seki: bool) -> numpy.ndarray:
        '''全体の石を置けるかどうかを取得する。
        Args:
            color (int): 石の色
            check_seki (bool): セキを確認するかどうか
        Returns:
            numpy.ndarray: 全体の石を置けるかどうか
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode="c"] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getEnableds(<int32_t*> &data[0], color, check_seki)

        return data.reshape((height, width))

    def get_territories(self, color: int) -> numpy.ndarray:
        '''確定地の一覧を取得する。
        Args:
            color (int): 基準とする石の色（WHITEを指定すると色を反転する）
        Returns:
            numpy.ndarray: 確定地の一覧
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode="c"] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getTerritories(<int32_t*> &data[0], color)

        return data.reshape((height, width))

    def get_owners(self, color: int, rule: int) -> numpy.ndarray:
        '''各座標の所有者の一覧を取得する。
        Args:
            color (int): 基準とする石の色（WHITEを指定すると色を反転する）
            rule (int): 計算ルール
        Returns:
            numpy.ndarray: 所有者の一覧
        '''
        cdef width = self.board.getWidth()
        cdef height = self.board.getHeight()
        cdef numpy.ndarray[numpy.int32_t, ndim=1, mode="c"] data = numpy.zeros(
            (height * width,), dtype=numpy.int32)

        self.board.getOwners(<int32_t*> &data[0], color, rule)

        return data.reshape((height, width))

    def get_patterns(self) -> List[int]:
        '''石の並びを表現した値を取得する。
        Returns:
            List[int: 石の並びを表現した値
        '''
        return self.board.getPatterns()

    def get_inputs(self, color: int, komi: float, rule: int, superko: bool) -> numpy.ndarray:
        '''推論モデルに入力する盤面データを取得する。
        Args:
            color (int): 着手する石の色
            komi (float): コミの目数
            rule (int): 勝敗の判定ルール
            superko (bool): スーパーコウルールを適用するならTrue
        Returns:
            numpy.ndarray: 盤面データ
        '''
        cdef numpy.ndarray[numpy.float32_t, ndim=1, mode="c"] inputs = numpy.zeros(
            (MODEL_INPUT_SIZE,), dtype=numpy.float32)

        self.board.getInputs(<float*> &inputs[0], color, komi, rule, superko)

        return inputs

    def get_state(self) -> List[int]:
        '''盤面の状態を取得する。
        Returns:
            List[int]: 盤面の状態
        '''
        return self.board.getState()

    def load_state(self, state: List[int]) -> None:
        '''盤面の状態を設定する。
        Args:
            state (List[int]): 盤面の状態
        '''
        self.board.loadState(state)

    def copy_from(self, board: NativeBoard) -> None:
        '''
        盤面をコピーする。
        Args:
            board (NativeBoard): コピー元の盤面
        '''
        self.board.copyFrom(board.board)
