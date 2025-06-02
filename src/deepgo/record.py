import struct
from io import IOBase, TextIOBase
from pathlib import Path
from typing import Dict, List, Tuple

from .board import Board, get_color_mark, is_valid_position
from .config import BLACK, DEFAULT_SIZE, PASS, WHITE
from .exception import GoException

CHARSETS = ('utf-8', 'shift-jis', 'euc-jp', 'iso-2022-jp')


def _read_text(file: str | Path | IOBase) -> str:
    if isinstance(file, IOBase):
        binary = file.read()
    else:
        binary = Path(file).read_bytes()

    if isinstance(binary, str):
        return binary

    text = None

    for charset in CHARSETS:
        try:
            text = binary.decode(charset)
        except BaseException:
            pass

    if text is None:
        raise GoException('unsupported charset')

    return text


def _parse_text(text: str) -> List[Dict[str, str]]:
    values_list: List[Dict[str, str]] = []
    index = 0
    state = 0
    name = ''
    value = ''
    escape = False

    while index < len(text) and state == 0:
        if text[index] == '(':
            state = 1
        index += 1

    while state > 0:
        if state == 1 and text[index] == ')':
            state = 0
        elif state == 1 and text[index] == ';':
            values_list.append({})
            name = ''
            value = ''
        elif state == 1 and text[index] == '[':
            state = 2
        elif state == 1 and not text[index].isspace():
            name += text[index]
        elif state == 2 and not escape and text[index] == ']':
            values_list[-1][name.lower()] = value
            name = ''
            value = ''
            state = 1
        elif state == 2 and not escape and text[index] == '\\':
            escape = True
        elif state == 2:
            value += text[index]
            escape = False

        index += 1

    return values_list


def _escape_value(value: str) -> str:
    return value.replace('\\', '\\\\').replace(']', '\\]')


class Properties(dict[str, str]):
    def get(self, key: str, default: str = '') -> str:  # type: ignore
        return self[key.lower()] if key.lower() in self else default

    def __getitem__(self, key: str) -> str:
        return super().__getitem__(key.lower())

    def __setitem__(self, key: str, value: str) -> None:
        super().__setitem__(key.lower(), value)

    def __delitem__(self, key: str) -> None:
        return super().__delitem__(str(key).lower())

    def __contains__(self, key: object) -> bool:
        return super().__contains__(str(key).lower())

    def update(self, values: Dict[str, str]) -> None:  # type: ignore
        for k, v in values.items():
            self[k] = v


class Record(object):
    '''SGFの棋譜データの変換を行うためのクラス。'''

    def __init__(self, file: str | Path | TextIOBase | None = None) -> None:
        '''棋譜データオブジェクトを初期化する。
        Args:
            file (str | Path | TextIOBase | None): 入力ファイル
        '''
        self.properties = Properties()
        self.moves: List[Tuple[Tuple[int, int], int, str | None]] = []

        if file is not None:
            self._parse(_read_text(file))

    @property
    def size(self) -> int:
        return int(self.properties.get('sz', str(DEFAULT_SIZE)))

    def _parse(self, text: str) -> None:
        values = _parse_text(text)

        self.properties.clear()
        self.moves.clear()

        self.properties.update(values[0])

        if 'sz' not in self.properties:
            self.properties['sz'] = str(DEFAULT_SIZE)

        for v in values[1:]:
            if 'b' in v:
                color = BLACK
                coord = v['b']
            elif 'w' in v:
                color = WHITE
                coord = v['w']
            else:
                continue

            if len(coord) == 2:
                pos = (ord(coord[0]) - 97, ord(coord[1]) - 97)
            else:
                pos = PASS

            if not is_valid_position(pos, self.size, self.size):
                pos = PASS

            if 'c' in v:
                message = v['c']
            else:
                message = None

            self.moves.append((pos, color, message))

    def dump(self, file: str | Path | IOBase) -> None:
        '''棋譜ファイルにデータを書き込む。
        Args:
            file (str | Path | IOBase): 出力ファイル
        '''
        if isinstance(file, IOBase):
            file.write(self.dumps())
        elif isinstance(file, Path):
            file.write_text(self.dumps())
        else:
            Path(str(file)).write_text(self.dumps())

    def dumps(self) -> str:
        '''棋譜を表現する文字列を返す。
        Returns:
            str: 棋譜を表現する文字列
        '''
        props: Dict[str, str] = {}
        props.update(self.properties)
        props['gm'] = '1'
        props['ff'] = '4'
        props['sz'] = str(self.size)

        props_text = ''.join(f'{k.upper()}[{v}]' for k, v in sorted(props.items()))
        text = f'(;{props_text}'

        for pos, color, message in self.moves:
            c = 'B' if color == BLACK else 'W'

            if is_valid_position(pos, self.size, self.size):
                p = struct.pack('BB', 97 + pos[0], 97 + pos[1]).decode('utf-8')
            else:
                p = ''

            text += f';{c}[{p}]'

            if message is not None:
                text += f'C[{_escape_value(message)}]'

        text += ')'

        return text

    def create_board(self) -> Board:
        '''盤面オブジェクトを作成する。
        Returns:
            Board: 盤面オブジェクト
        '''
        from .board import Board

        board = Board(self.size, self.size)

        if 'ha' in self.properties:
            board.set_handicap(int(self.properties['ha']))

        for pos, color, _ in self.moves:
            board.play(pos, color)

        return board

    def __str__(self) -> str:
        text = ['komi: {}'.format(self.properties.get('km', 'N/A')),
                'player[X]: {}'.format(self.properties.get('pb', 'N/A')),
                'player[O]: {}'.format(self.properties.get('pw', 'N/A')),
                'result: {}'.format(self.properties.get('re', 'N/A')),
                'points:']
        text.extend([f'  {get_color_mark(c)}{p} : {m}' for p, c, m in self.moves])

        return "\n".join(text)
