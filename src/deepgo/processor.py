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
        '''Initialize computation management object.
        Args:
            model (str | Path): Path to model file
            gpus (List[int]): List of GPU numbers to use
            batch_size (int): Maximum batch size
            fp16 (bool): True to use FP16
            deterministic (bool): True to make results reproducible
            threads_par_gpu (int): Number of threads per GPU
        '''
        if not Path(model).exists():
            raise FileNotFoundError(f'File not found: {model}')

        self.native = NativeProcessor(
            str(model), gpus, batch_size, fp16, deterministic, threads_par_gpu)

    def execute(self, inputs: np.ndarray) -> np.ndarray:
        '''Execute inference.
        Args:
            inputs (np.ndarray): Input data
        '''
        return self.native.execute(inputs)
