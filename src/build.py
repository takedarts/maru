import argparse
import os
import platform
import string
import subprocess
from pathlib import Path
from typing import List

from deepgo import config

SRC_PATH = Path(__file__).parent.absolute()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Build native codes',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument(
        '--clean', action='store_true', default=False, help='Clean object files only.')
    return parser.parse_args()


def make_files(path: str) -> None:
    '''Config.hを作成する'''
    work_path = Path(__file__).parent / path

    # Config.hを作成する
    config_path = work_path / 'cpp' / 'Config.h'
    config_text = (work_path / 'cpp' / 'Config.template').read_text()
    config_text = string.Template(config_text).substitute(config.__dict__)

    if not config_path.exists() or config_path.read_text() != config_text:
        config_path.write_text(config_text)

    # CMakeLists.txtを作成する
    cpp_files = ' '.join(f'cpp/{p.name}' for p in (work_path / 'cpp').glob('*.cpp'))
    cmake_path = work_path / 'CMakeLists.txt'
    cmake_text = (work_path / 'CMakeLists.template').read_text()
    cmake_text = cmake_text.replace('%PYTHON3_VERSION%', platform.python_version())
    cmake_text = cmake_text.replace('%CPP_FILES%', cpp_files)

    if not cmake_path.exists() or cmake_path.read_text() != cmake_text:
        cmake_path.write_text(cmake_text)

    # modules.pyxのタイムスタンプを更新する
    module_path = work_path / 'modules.pyx'
    module_path.touch()


def cmake(path: str) -> None:
    '''cmakeを実行したCythonライブラリを作成する'''
    curr_path = Path('.').resolve()
    work_path = Path(__file__).parent / path
    build_path = work_path / 'build'

    if not build_path.is_dir():
        build_path.mkdir()

    os.chdir(build_path)

    if subprocess.run(['cmake', '..']).returncode != 0:
        raise Exception('failed in making module (cmake).')

    if subprocess.run(['make']).returncode != 0:
        raise Exception('failed in making module.')

    os.chdir(curr_path)

    if (build_path / 'modules.so').is_file():
        (build_path / 'modules.so').rename(work_path / 'modules.so')
    else:
        raise Exception('module file is not found.')


def _clean(paths: List[Path]) -> None:
    for path in paths:
        if path.is_dir():
            _clean(list(path.iterdir()))
            path.rmdir()
        elif path.is_file():
            path.unlink()


def clean(path: str) -> None:
    work_path = Path(__file__).parent / path
    targets = [
        work_path / 'build',
        work_path / 'CMakeLists.txt',
        work_path / 'cpp' / 'Config.h',
    ]

    targets.extend(
        p for p in work_path.iterdir() if p.name.endswith('.so'))

    _clean(targets)


def main() -> None:
    args = parse_args()
    path = 'deepgo/native'

    if args.clean:
        clean(path)
    else:
        make_files(path)
        cmake(path)


if __name__ == '__main__':
    main()
