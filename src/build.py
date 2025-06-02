import argparse
import os
import platform
import string
import subprocess
from pathlib import Path
from typing import List

import cmake
import torch
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
    torch_path = str(Path(torch.__file__).parent).replace('\\', '/')
    cpp_paths = [p.relative_to(work_path) for p in (work_path / 'cpp').glob('**/*.cpp')]
    cpp_files = ' '.join(map(str, cpp_paths)).replace('\\', '/')
    cmake_path = work_path / 'CMakeLists.txt'
    cmake_text = (work_path / 'CMakeLists.template').read_text()
    cmake_text = cmake_text.replace('%PYTHON3_VERSION%', platform.python_version())
    cmake_text = cmake_text.replace('%TORCH_PATH%', torch_path)
    cmake_text = cmake_text.replace('%CPP_FILES%', cpp_files)

    if not cmake_path.exists() or cmake_path.read_text() != cmake_text:
        cmake_path.write_text(cmake_text)

    # modules.pyxのタイムスタンプを更新する
    module_path = work_path / 'modules.pyx'
    module_path.touch()


def run_cmake(path: str) -> None:
    '''cmakeを実行してCythonライブラリを作成する'''
    # cmakeの実行ファイルのパスを取得する
    if hasattr(cmake, 'CMAKE_BIN_DIR'):
        cmake_path = os.path.join(cmake.CMAKE_BIN_DIR, 'cmake')
    else:
        cmake_path = 'cmake'

    # buildディレクトリを作成する
    curr_path = Path('.').resolve()
    work_path = Path(__file__).parent / path
    build_path = work_path / 'build'

    if not build_path.is_dir():
        build_path.mkdir()

    # buildディレクトリに移動する
    os.chdir(build_path)

    # cmakeを実行する
    if subprocess.run([cmake_path, '..']).returncode != 0:
        raise Exception('failed in making module (cmake).')

    # makeを実行する（Windowsではmsbuildを使用する）
    if os.name == 'nt':
        sln_path = str(build_path / 'DEEPGO_CYTHON.sln')
        sln_args = ['/t:Clean;Rebuild', '/p:Configuration=Release']
        if subprocess.run(['msbuild', sln_path] + sln_args).returncode != 0:
            raise Exception('failed in making module (msbuild).')
    else:
        if subprocess.run(['make']).returncode != 0:
            raise Exception('failed in making module (make).')

    # buildディレクトリから元のディレクトリに戻る
    os.chdir(curr_path)

    # コンパイルしたモジュールファイルをwork_pathに移動する
    if (build_path / 'modules.so').is_file():
        (build_path / 'modules.so').rename(work_path / 'modules.so')
    elif (build_path / 'Release' / 'modules.pyd').is_file():
        (build_path / 'Release' / 'modules.pyd').rename(work_path / 'modules.pyd')
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
        work_path / 'modules.so',
        work_path / 'modules.pyd',
    ]

    _clean(targets)


def main() -> None:
    args = parse_args()
    path = 'deepgo/native'

    if args.clean:
        clean(path)
    else:
        make_files(path)
        run_cmake(path)


if __name__ == '__main__':
    main()
