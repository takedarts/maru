from pathlib import Path
from typing import List

import numpy as np

from .native import NativeProcessor


class Processor(object):
    def __init__(
        self,
        model: str | Path,
        gpus: List[int] = [-1],
        batch_size: int = 2048,
        fp16: bool = False,
        deterministic: bool = False,
        threads_par_gpu: int = 1,
    ) -> None:
        '''計算管理オブジェクトを初期化する。
        Args:
            model (str | Path): モデルファイルのパス
            gpus (List[int]): 使用するGPUの番号のリスト
            batch_size (int): 最大バッチサイズ
            fp16 (bool): FP16を使用するかどうか
            deterministic (bool): 結果を再現可能にするならTrue
            threads_par_gpu (int): 1GPUあたりのスレッド数
        '''
        if not Path(model).exists():
            raise FileNotFoundError(f'File not found: {model}')

        self.native = NativeProcessor(
            str(model), gpus, batch_size, fp16, deterministic, threads_par_gpu)

    def execute(self, inputs: np.ndarray) -> np.ndarray:
        '''推論を実行する。
        Args:
            inputs (np.ndarray): 入力データ
        '''
        return self.native.execute(inputs)
