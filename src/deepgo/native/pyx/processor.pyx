from typing import List

from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector

import numpy
cimport numpy

from deepgo.config import MODEL_OUTPUT_SIZE


cdef extern from "cpp/Processor.h" namespace "deepgo":
    cdef cppclass Processor:
        Processor(string, vector[int32_t], int32_t, bool, bool, int32_t) except +
        void execute(float*, float*, int32_t)


cdef class NativeProcessor:
    cdef Processor *processor

    def __cinit__(
        self,
        model: str,
        gpus: List[int],
        batch_size: int,
        fp16: bool,
        deterministic: bool,
        threads_par_gpu: int,
    ) -> None:
        '''推論処理オブジェクトを作成する。
        Args:
            model (str): モデルファイルのパス
            gpus (List[int]): 使用するGPUのIDのリスト
            batch_size (int): バッチサイズ
            fp16 (bool): FP16で計算を行うならTrue
            deterministic (bool): 結果を再現可能にするならTrue
            threads_par_gpu (int): 1GPUあたりのスレッド数
        '''
        self.processor = new Processor(
            model.encode('utf-8'), gpus,
            batch_size, fp16, deterministic, threads_par_gpu)

    def __dealloc__(self) -> None:
        del self.processor

    def execute(self, inputs: numpy.ndarray) -> numpy.ndarray:
        '''推論を実行する。
        Args:
            inputs (numpy.ndarray): 入力データ
        Returns:
            numpy.ndarray: 出力データ
        '''
        cdef numpy.ndarray[numpy.float32_t, ndim=2, mode="c"] outputs = numpy.zeros(
            (inputs.shape[0], MODEL_OUTPUT_SIZE), dtype=numpy.float32)

        cdef int size = inputs.shape[0]
        cdef float* in_data = <float*> inputs.data
        cdef float* out_data = <float*> outputs.data

        self.processor.execute(in_data, out_data, size)

        return outputs
