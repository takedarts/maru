from pathlib import Path
from typing import List, Tuple

from deepgo.config import COLOR_BLACK, COLOR_EMPTY, COLOR_WHITE
from deepgo.record import Record


def read_record(
    name: str,
) -> Tuple[
        Record,
        List[List[int | None]] | None,  # Stone colors
        List[List[int | None]] | None,  # Settled territories
        List[List[int | None]] | None,  # Group sizes
        List[List[int | None]] | None,  # Liberty counts
        List[List[int | None]] | None,  # Black legal moves
        List[List[int | None]] | None,  # White legal moves
        List[List[int | None]] | None,  # Ladder positions
]:
    '''Read a game record and its expected board data.
    Args:
        name (str): Fixture name without its extension.
    Returns:
        Tuple[Record, ...]: Record followed by seven optional List[List[int | None]] grids.
    '''
    # Fixture directory
    base_path = Path(__file__).parent

    # Load the game record
    record = Record(base_path / f'{name}.sgf')

    # Load expected board data
    lines = (base_path / f'{name}.txt').read_text().splitlines()
    data: List[List[str]] = []
    data.append([])

    for line in lines:
        if len(line) == 0:
            data.append([])
        else:
            data[-1].append(line)

    def conv(c: str) -> int | None:
        '''Decode a board fixture cell.
        Args:
            c (str): Stone color
        Returns:
            int | None: Result of the operation.
        '''
        if c == 'x':
            return COLOR_BLACK
        elif c == 'o':
            return COLOR_WHITE
        elif c == '.':
            return COLOR_EMPTY
        elif c == '*':
            return None
        else:
            return int(c)

    def conv_line(line: str) -> List[int | None]:
        '''Decode one row of fixture data.
        Args:
            line (str): line
        Returns:
            List[int | None]: Result of the operation.
        '''
        return [conv(c) for c in line]

    def conv_data(data: List[str]) -> List[List[int | None]]:
        '''Decode a section of fixture data.
        Args:
            data (List[str]): data
        Returns:
            List[List[int | None]]: Result of the operation.
        '''
        return [conv_line(line) for line in data]

    colors = conv_data(data[0]) if len(data[0]) == record.size else None
    territories = conv_data(data[1]) if len(data[1]) == record.size else None
    ren_sizes = conv_data(data[2]) if len(data[2]) == record.size else None
    ren_spaces = conv_data(data[3]) if len(data[3]) == record.size else None
    black_enables = conv_data(data[4]) if len(data[4]) == record.size else None
    white_enables = conv_data(data[5]) if len(data[5]) == record.size else None
    shicho_positions = conv_data(data[6]) if len(data[6]) == record.size else None

    return (
        record,
        colors, territories, ren_sizes, ren_spaces,
        black_enables, white_enables, shicho_positions,
    )
