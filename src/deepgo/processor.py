from pathlib import Path
from typing import List

import numpy as np

from .config import DEFAULT_BATCH_SIZE, DEFAULT_THREADS_PER_GPU
from .native import NativeInferenceProcessor


class Processor(object):
    def __init__(
        self,
        model: str | Path,
        gpus: List[int] = [-1],
        batch_size: int = DEFAULT_BATCH_SIZE,
        fp16: bool = False,
        deterministic: bool = False,
        threads_per_gpu: int = DEFAULT_THREADS_PER_GPU,
        cache_size: int = 0,
    ) -> None:
        '''Initialize computation management object.
        Args:
            model (str | Path): Path to model file
            gpus (List[int]): List of GPU numbers to use
            batch_size (int): Maximum batch size
            fp16 (bool): True to use FP16
            deterministic (bool): True to make results reproducible
            threads_per_gpu (int): Number of threads per GPU
            cache_size (int): Cache size for board evaluation
        '''
        if not Path(model).exists():
            raise FileNotFoundError(f'File not found: {model}')

        self.native = NativeInferenceProcessor(
            str(model), gpus, fp16, deterministic, batch_size, threads_per_gpu, cache_size)

    def execute(self, inputs: np.ndarray) -> np.ndarray:
        '''Execute inference.
        Args:
            inputs (np.ndarray): Input data
        '''
        return self.native.execute(inputs)

    def get_batch_fill_rate(self) -> float:
        '''Get the ratio of inference requests included in the batch.
        Returns:
            float: Ratio of inference requests included in the batch
        '''
        return self.native.get_batch_fill_rate()

    def get_cache_hit_rate(self) -> float:
        '''Get the cache hit rate of inference.
        Returns:
            float: Cache hit rate of inference
        '''
        return self.native.get_cache_hit_rate()
