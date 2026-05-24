from typing import List

from libc.stdint cimport int32_t
from libcpp.vector cimport vector

import numpy
cimport numpy

from deepgo.config import MODEL_OUTPUT_PACK_SIZE, MODEL_OUTPUT_SIZE
from pyx.inference cimport InferenceModel, InferenceProcessor


cdef class NativeInferenceModel:
    cdef InferenceModel *model

    @staticmethod
    def get_available_gpus() -> List[int]:
        '''利用可能なGPUの番号を取得する。
        Returns:
            list[int]: GPUの番号のリスト
        '''
        return InferenceModel.getAvailableGPUs()

    def __cinit__(self, model: str, gpu: int, fp16: bool, deterministic: bool) -> None:
        '''モデルオブジェクトを作成する。
        Args:
            model (str): モデルファイルのパス
            gpu (int): 使用するGPUのID
            fp16 (bool): FP16で計算を行うならTrue
            deterministic (bool): 計算結果を再現可能にするならTrue
        '''
        self.model = new InferenceModel(model.encode('utf-8'), gpu, fp16, deterministic)

    def __dealloc__(self) -> None:
        del self.model

    def forward(self, inputs: numpy.ndarray) -> numpy.ndarray:
        '''推論を実行する。
        Args:
            inputs (numpy.ndarray): 入力データ
        Returns:
            numpy.ndarray: 出力データ
        '''
        cdef numpy.ndarray[numpy.float32_t, ndim=2, mode="c"] outputs = numpy.zeros(
            (inputs.shape[0], MODEL_OUTPUT_SIZE), dtype=numpy.float32)

        cdef int32_t size = <int32_t>inputs.shape[0]
        cdef int32_t* in_data = <int32_t*> inputs.data
        cdef float* out_data = <float*> outputs.data

        self.model.forward(in_data, out_data, size)

        return outputs

    def is_cuda(self) -> bool:
        '''CUDAを使用しているかどうかを取得する。
        Returns:
            bool: CUDAを使用しているならTrue
        '''
        return self.model.isCuda()


cdef class NativeInferenceProcessor:
    cdef InferenceProcessor *processor

    def __cinit__(
        self,
        model: str,
        gpus: List[int],
        fp16: bool,
        deterministic: bool,
        batch_size: int,
        threads_per_gpu: int,
        cache_size: int,
    ) -> None:
        '''推論プロセッサオブジェクトを作成する。
        Args:
            model (str): モデルファイルのパス
            gpus (List[int]): 使用するGPUのIDのリスト
            fp16 (bool): FP16で計算を行うならTrue
            deterministic (bool): 計算結果を再現可能にするならTrue
            batch_size (int): 推論のバッチサイズ
            threads_per_gpu (int): GPUごとのスレッド数
            cache_size (int): 推論結果のキャッシュサイズ
        '''
        cdef vector[int32_t] gpu_vector

        for gpu in gpus:
            gpu_vector.push_back(gpu)

        self.processor = new InferenceProcessor(
            model.encode('utf-8'), gpu_vector, fp16, deterministic,
            batch_size, threads_per_gpu, cache_size)

    def __dealloc__(self) -> None:
        del self.processor

    def predict(
        self,
        board: NativeBoard,  # type: ignore
        color: int,
        komi: float,
        rule: int,
        superko: bool,
    ) -> float:
        '''盤面に対する推論を実行する。
        Args:
            board (NativeBoard): 盤面オブジェクト
            color (int): 次に打つ石の色
            komi (float): コミの目数
            rule (int): ルール
            superko (bool): スーパーコウルールを使用するならTrue
        Returns:
            float: 評価値
        '''
        return self.processor.predict(board.board, color, komi, rule, superko)

    def execute(self, inputs: numpy.ndarray) -> numpy.ndarray:
        '''推論を同期実行する。
        Args:
            inputs (numpy.ndarray): 入力データ
        Returns:
            numpy.ndarray: 出力データ
        '''
        cdef numpy.ndarray[numpy.float32_t, ndim=2, mode="c"] outputs = numpy.zeros(
            (inputs.shape[0], MODEL_OUTPUT_SIZE), dtype=numpy.float32)
        cdef int32_t size = <int32_t>inputs.shape[0]
        cdef int32_t* in_data = <int32_t*> inputs.data
        cdef float* out_data = <float*> outputs.data

        self.processor.execute(in_data, out_data, size)

        return outputs
