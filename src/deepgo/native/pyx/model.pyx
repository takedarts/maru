from libc.stdint cimport int32_t, uint32_t
from libcpp cimport bool
from libcpp.string cimport string

import numpy
cimport numpy

from deepgo.config import MODEL_OUTPUT_SIZE


cdef extern from "cpp/Model.h" namespace "deepgo":
    cdef cppclass Model:
        Model(string, int32_t, bool, bool) except +
        void forward(float*, float*, uint32_t)
        int32_t isCuda()


cdef class NativeModel:
    cdef Model *model

    def __cinit__(self, model: str, gpu: int, fp16: bool, deterministic: bool) -> None:
        '''Create a model object.
        Args:
            model (str): Path to the model file
            gpu (int): GPU ID to use
            fp16 (bool): True to compute with FP16
            deterministic (bool): True to make computation results reproducible
        '''
        self.model = new Model(model.encode('utf-8'), gpu, fp16, deterministic)

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

        cdef int size = inputs.shape[0]
        cdef float* in_data = <float*> inputs.data
        cdef float* out_data = <float*> outputs.data

        self.model.forward(in_data, out_data, size)

        return outputs

    def is_cuda(self) -> bool:
        '''Check if CUDA is being used.
        Returns:
            bool: True if using CUDA
        '''
        return self.model.isCuda() != 0
