from typing import List

from libc.stdint cimport int32_t
from libcpp.vector cimport vector

import numpy
cimport numpy

from deepgo.config import MODEL_OUTPUT_SIZE
from pyx.inference cimport InferenceModel, InferenceProcessor


cdef class NativeInferenceModel:
    cdef InferenceModel *model

    @staticmethod
    def get_available_gpus() -> List[int]:
        '''Get the IDs of available GPUs.
        Returns:
            list[int]: List of available GPU IDs
        '''
        return InferenceModel.getAvailableGPUs()

    def __cinit__(self, model: str, gpu: int, fp16: bool, deterministic: bool) -> None:
        '''Create a model object.
        Args:
            model (str): Path to the model file
            gpu (int): ID of the GPU to use
            fp16 (bool): True to perform computation in FP16
            deterministic (bool): True to make computation results reproducible
        '''
        self.model = new InferenceModel(model.encode('utf-8'), gpu, fp16, deterministic)

    def __dealloc__(self) -> None:
        del self.model

    def forward(self, inputs: numpy.ndarray) -> numpy.ndarray:
        '''Run inference.
        Args:
            inputs (numpy.ndarray): Input data
        Returns:
            numpy.ndarray: Output data
        '''
        cdef numpy.ndarray[numpy.float32_t, ndim=2, mode="c"] outputs = numpy.zeros(
            (inputs.shape[0], MODEL_OUTPUT_SIZE), dtype=numpy.float32)

        cdef int32_t size = <int32_t>inputs.shape[0]
        cdef int32_t* in_data = <int32_t*> inputs.data
        cdef float* out_data = <float*> outputs.data

        self.model.forward(in_data, out_data, size)

        return outputs

    def is_cuda(self) -> bool:
        '''Check whether CUDA is being used.
        Returns:
            bool: True if CUDA is being used
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
        '''Create an inference processor object.
        Args:
            model (str): Path to the model file
            gpus (List[int]): List of GPU IDs to use
            fp16 (bool): True to perform computation in FP16
            deterministic (bool): True to make computation results reproducible
            batch_size (int): Batch size for inference
            threads_per_gpu (int): Number of threads per GPU
            cache_size (int): Cache size for inference results
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
        '''Run inference on the board.
        Args:
            board (NativeBoard): Board object
            color (int): Color of the next stone to play
            komi (float): Komi value
            rule (int): Rule
            superko (bool): True to use the superko rule
        Returns:
            float: Evaluation value
        '''
        return self.processor.predict(board.board, color, komi, rule, superko)

    def execute(self, inputs: numpy.ndarray) -> numpy.ndarray:
        '''Run inference synchronously.
        Args:
            inputs (numpy.ndarray): Input data
        Returns:
            numpy.ndarray: Output data
        '''
        cdef numpy.ndarray[numpy.float32_t, ndim=2, mode="c"] outputs = numpy.zeros(
            (inputs.shape[0], MODEL_OUTPUT_SIZE), dtype=numpy.float32)
        cdef int32_t size = <int32_t>inputs.shape[0]
        cdef int32_t* in_data = <int32_t*> inputs.data
        cdef float* out_data = <float*> outputs.data

        self.processor.execute(in_data, out_data, size)

        return outputs

    def get_batch_fill_rate(self) -> float:
        '''Get the ratio of inference requests included in the batch.
        Returns:
            float: Ratio of inference requests included in the batch
        '''
        return self.processor.getBatchFillRate()

    def get_cache_hit_rate(self) -> float:
        '''Get the cache hit rate of inference.
        Returns:
            float: Cache hit rate of inference
        '''
        return self.processor.getCacheHitRate()
