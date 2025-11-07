import logging
import sys
from pathlib import Path
from typing import TextIO

from .config import LOGGING_DATE_FORMAT, LOGGING_FORMAT


def start_logging(
    *,
    debug: bool = False,
    file: str | Path | None = None,
    console: TextIO | None = None,
) -> None:
    '''Start log output.
    Args:
        file (str | Path | None): File to output logs
        debug (bool): True to output debug information
    '''
    formatter = logging.Formatter(LOGGING_FORMAT, LOGGING_DATE_FORMAT)

    if file is None and console is None:
        console = sys.stdout

    if file is not None:
        file_handler = logging.FileHandler(file, mode='a')
        file_handler.setFormatter(formatter)
        logging.getLogger().addHandler(file_handler)

    if console is not None:
        console_handler = logging.StreamHandler(stream=console)
        console_handler.setFormatter(formatter)
        logging.getLogger().addHandler(console_handler)

    logging.getLogger().setLevel(logging.DEBUG if debug else logging.INFO)
    logging.getLogger('PIL').setLevel(logging.INFO)
    logging.getLogger('matplotlib').setLevel(logging.INFO)
