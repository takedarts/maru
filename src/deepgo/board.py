import functools
import struct
from typing import List, Tuple

import numpy as np

from .config import BLACK, DEFAULT_KOMI, RULE_CH, WHITE
from .exception import GoException
from .native import NativeBoard


def get_opposite_color(color: int) -> int:
    '''指定された色の相手の色を取得する。
    Args:
        color (int): 石の色
    Returns:
        int: 相手の石の色
    '''
    return color * -1


def get_color_name(color: int) -> str:
    '''石の色を表す文字列を取得する。
    Args:
        color (int): 石の色
    Returns:
        str: 石の色を表す文字列
    '''
    if color == BLACK:
        return 'black'
    elif color == WHITE:
        return 'white'
    else:
        return 'empty'


def get_color_mark(color: int) -> str:
    '''石の色を表す記号を取得する。
    Args:
        color (int): 石の色
    Returns:
        str: 石の色を表す記号
    '''
    if color == BLACK:
        return 'X'
    elif color == WHITE:
        return 'O'
    else:
        return '.'


def is_valid_position(pos: tuple[int, int], width: int, height: int) -> bool:
    '''指定された座標が盤面内にあるかどうかを判定する。
    Args:
        pos (Tuple[int, int]): 座標
        width (int): 盤面の幅
        height (int): 盤面の高さ
    Returns:
        bool: 盤面内にある場合はTrue
    '''
    return 0 <= pos[0] < width and 0 <= pos[1] < height


def _get_array_string(
    values: np.ndarray,
    pos: Tuple[int, int] | None = None,
) -> List[str]:
    '''指定されたデータの文字列を返す
    BLACKをX、WHITEをO、空を.で表現する。
    Args:
        values (np.ndarray): データ
        pos (Tuple[int, int]): 強調する座標
    Returns:
        List[str]: 盤面の文字列
    '''
    text = []
    text.append('   ' + ''.join(f'{i:2d}' for i in range(values.shape[1])) + '  ')
    text.append('  +' + ('-' * (values.shape[1] * 2 + 1)) + '+')

    for y in range(values.shape[0]):
        line = f'{y:2d}|'

        if pos is not None and pos[0] == 0 and pos[1] == y:
            line += '['
        else:
            line += ' '

        for x in range(values.shape[1]):
            line += get_color_mark(values[y, x])

            if pos is not None and pos[0] == x and pos[1] == y:
                line += ']'
            elif pos is not None and pos[0] == x + 1 and pos[1] == y:
                line += '['
            else:
                line += ' '

        line += '|'
        text.append(line)

    text.append('  +' + ('-' * (values.shape[1] * 2 + 1)) + '+')

    return text


def get_array_string(
    *values: np.ndarray,
    pos: Tuple[int, int] | None = None,
) -> str:
    '''指定された盤面データの文字列を返す
    Args:
        *values (np.ndarray): 盤面データ
        pos (Tuple[int, int]): 強調する座標
    Returns:
        str: 盤面の文字列
    '''
    return "\n".join([' '.join(s) for s in zip(*[_get_array_string(v, pos) for v in values])])


def get_board_string(
    *boards: 'Board',
    pos: Tuple[int, int] | None = None,
) -> str:
    '''指定された盤面データの文字列を返す
    Args:
        *boards (Board): 盤面
        pos (Tuple[int, int]): 強調する座標
    Returns:
        str: 盤面の文字列
    '''
    return get_array_string(*[v.get_colors() for v in boards], pos=pos)


def get_handicap_positions(width: int, height: int, handicap: int) -> List[Tuple[int, int]]:
    positions: List[Tuple[int, int]] = []
    ver_line = 3 if width >= 13 else 2
    hor_line = 3 if height >= 13 else 2

    if handicap >= 2:
        positions.append((width - ver_line - 1, hor_line))
        positions.append((ver_line, height - hor_line - 1))

    if handicap >= 3:
        positions.append((ver_line, hor_line))

    if handicap >= 4:
        positions.append((width - ver_line - 1, height - hor_line - 1))

    if handicap == 5:
        positions.append((width // 2, height // 2))

    if handicap >= 6:
        positions.append((ver_line, height // 2))
        positions.append((width - ver_line - 1, height // 2))

    if handicap == 7:
        positions.append((width // 2, height // 2))

    if handicap >= 8:
        positions.append((width // 2, hor_line))
        positions.append((width // 2, height - hor_line - 1))

    if handicap == 9:
        positions.append((width // 2, height // 2))

    return positions


class Board(object):
    def __init__(self, width: int, height: int) -> None:
        '''盤面オブジェクトを初期化する。
        Args:
            width (int): 盤面の幅
            height (int): 盤面の高さ
        '''
        self.native = NativeBoard(width, height)

    def get_width(self) -> int:
        '''盤面の幅を取得する。
        Returns:
            int: 盤面の幅
        '''
        return self.native.get_width()

    def get_height(self) -> int:
        '''盤面の高さを取得する。
        Returns:
            int: 盤面の高さ
        '''
        return self.native.get_height()

    def is_valid_position(self, pos: tuple[int, int]) -> bool:
        '''指定された座標が盤面内にあるかどうかを判定する。
        Args:
            pos (Tuple[int, int]): 座標
        Returns:
            bool: 盤面内にある場合はTrue
        '''
        return is_valid_position(pos, self.get_width(), self.get_height())

    def set_handicap(self, handicap: int) -> None:
        '''置石を設定する。
        Args:
            handicap (int): 置石の数
        '''
        for pos in get_handicap_positions(self.get_width(), self.get_height(), handicap):
            self.native.play(pos, BLACK)

    def play(self, pos: tuple[int, int], color: int) -> int:
        '''指定された位置に石を置く。
        Args:
            pos (Tuple[int, int]): 置く位置
            color (int): 石の色
        Returns:
            int: 取り上げた石の数
        '''
        captured = self.native.play(pos, color)

        if captured < 0:
            raise GoException(f'Invalid move: {pos} {get_color_name(color)}')

        return captured

    def get_ko(self, color: int) -> tuple[int, int]:
        '''コウの位置を取得する。
        Args:
            color (int): コウの対象となる石の色
        Returns:
            Tuple[int, int]: コウの位置
        '''
        return self.native.get_ko(color)

    def get_histories(self, color: int) -> List[tuple[int, int]]:
        '''着手履歴を取得する。
        Args:
            color (int): 着手の色
        Returns:
            List[Tuple[int, int]]: 着手の一覧
        '''
        return self.native.get_histories(color)

    def get_color(self, pos: tuple[int, int]) -> int:
        '''指定された位置の石の色を取得する。
        Args:
            pos (Tuple[int, int]): 位置
        Returns:
            int: 石の色
        '''
        return self.native.get_color(pos)

    def get_colors(self, color: int = BLACK) -> np.ndarray:
        '''石の一覧を取得する。
        引数にWHITEを指定すると黒白反転した状態で返す。
        Args:
            color (int): 石の色
        Returns:
            np.ndarray: 石の色の一覧
        '''
        return self.native.get_colors(color)

    def get_ren_size(self, pos: tuple[int, int]) -> int:
        '''指定された位置の連の大きさを取得する。
        Args:
            pos (Tuple[int, int]): 位置
        Returns:
            int: 連の大きさ
        '''
        return self.native.get_ren_size(pos)

    def get_ren_space(self, pos: tuple[int, int]) -> int:
        '''指定された位置の連のダメの数を取得する。
        Args:
            pos (Tuple[int, int]): 位置
        Returns:
            int: 連のダメの数
        '''
        return self.native.get_ren_space(pos)

    def is_shicho(self, pos: tuple[int, int]) -> bool:
        '''指定された位置がシチョウかどうかを判定する。
        Args:
            pos (Tuple[int, int]): 位置
        Returns:
            bool: シチョウの場合はTrue
        '''
        return self.native.is_shicho(pos)

    def is_enabled(
        self,
        pos: tuple[int, int],
        color: int,
        check_seki: bool = False,
    ) -> bool:
        '''指定された位置に石を置けるかどうかを判定する。
        Args:
            pos (Tuple[int, int]): 位置
            color (int): 石の色
            check_seki (bool): セキを考慮するかどうか
        Returns:
            bool: 石を置ける場合はTrue
        '''
        return self.native.is_enabled(pos, color, check_seki)

    def get_enableds(
        self,
        color: int,
        check_seki: bool = False,
    ) -> np.ndarray:
        '''石を置ける位置を取得する。
        Args:
            color (int): 石の色
            check_seki (bool): セキを考慮するかどうか
        Returns:
            np.ndarray: 石を置ける位置
        '''
        return self.native.get_enableds(color, check_seki)

    def get_territories(self, color: int = BLACK) -> np.ndarray:
        '''確定地の一覧を取得する。
        Args:
            color (int): 基準とする石の色（WHITEの場合は黒白反転した状態で返す）
        Returns:
            np.ndarray: 確定地の一覧
        '''
        return self.native.get_territories(color)

    def get_owners(self, color: int = BLACK, rule: int = RULE_CH) -> np.ndarray:
        '''各座標の所有者の一覧を取得する。
        Args:
            color (int): 基準とする石の色（WHITEの場合は黒白反転した状態で返す）
            rule (int): 勝敗の判定ルール
        Returns:
            np.ndarray: 所有者の一覧
        '''
        return self.native.get_owners(color, rule)

    def get_score(
        self,
        color: int = BLACK,
        rule: int = RULE_CH,
        komi: float = DEFAULT_KOMI,
    ) -> float:
        '''指定された色の得点を取得する。
        Args:
            color (int): 得点を計算する石の色
            rule (int): 勝敗の判定ルール
            komi (float): コミの目数
        Returns:
            float: 得点
        '''
        if color == BLACK:
            return self.get_owners(color, rule).sum() - komi
        else:
            return self.get_owners(color, rule).sum() + komi

    def get_patterns(self) -> List[int]:
        '''石の並びを表現した値を取得する。
        Returns:
            List[int]: 石の並びを表現した値
        '''
        return self.native.get_patterns()

    def get_inputs(
        self,
        color: int,
        komi: float,
        rule: int,
        superko: bool,
    ) -> np.ndarray:
        '''推論モデルに入力するデータを取得する。
        Args:
            color (int): 着手する石の色
            komi (float): コミの目数
            rule (int): 勝敗の判定ルール
            superko (bool): スーパーコウルールを適用するかどうか
        Returns:
            np.ndarray: 入力データ
        '''
        return self.native.get_inputs(color, komi, rule, superko)

    def get_state(self) -> List[int]:
        '''盤面の状態をシリアライズした値を返す。
        :return: 盤面の状態値
        '''
        return self.native.get_state()

    def load_state(self, state: List[int]) -> None:
        '''盤面の状態をデシリアライズする。
        :param state: 盤面の状態値
        '''
        self.native.load_state(state)

    def copy_from(self, board: 'Board') -> None:
        '''盤面をコピーする。
        Args:
            board (Board): コピー元の盤面
        '''
        self.native.copy_from(board.native)

    def __getstate__(self) -> bytes:
        width = self.get_width()
        height = self.get_height()
        values = self.native.get_state()
        return struct.pack(f'>bb{len(values)}i', width, height, *values)

    def __setstate__(self, state: bytes) -> None:
        values = list(struct.unpack(f'>bb{(len(state) - 2) // 4}i', state))
        width = values[0]
        height = values[1]
        self.native = NativeBoard(width, height)
        self.native.load_state(values[2:])

    def __hash__(self) -> int:
        return int(functools.reduce(lambda x, y: x ^ y, self.native.get_state()))

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Board):
            return False

        return (self.native.get_state() == other.native.get_state()).all()

    def __str__(self) -> str:
        '''盤面を文字列で表現する。
        Returns:
            str: 盤面の文字列表現
        '''
        return get_board_string(self)
