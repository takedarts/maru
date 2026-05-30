from libc.stdint cimport int32_t
from libcpp cimport bool as cpp_bool
from libcpp.string cimport string
from libcpp.vector cimport vector
from pyx.board cimport Board
from pyx.policy cimport Policy


cdef extern from "cpp/InferenceModel.h" namespace "deepgo":
    cdef cppclass InferenceModel:
        @staticmethod
        vector[int32_t] getAvailableGPUs() except +

        InferenceModel(string filename, int32_t gpu, cpp_bool fp16, cpp_bool deterministic) except +
        void forward(int32_t* inputs, float* outputs, int32_t size) except +
        cpp_bool isCuda()


cdef extern from "cpp/InferenceProcessor.h" namespace "deepgo":
    cdef cppclass InferenceProcessor:
        InferenceProcessor(
            string model, vector[int32_t] gpus, cpp_bool fp16, cpp_bool deterministic,
            int32_t batchSize, int32_t threadsPerGpu, int32_t cacheSize) except +
        float predict(
            Board* board, int32_t color, float komi, int32_t rule, cpp_bool superko) except +
        void execute(int32_t* inputs, float* outputs, int32_t size) except +
        float getEfficiency() except +


cdef extern from "cpp/InferenceResult.h" namespace "deepgo":
    cdef cppclass InferenceResult:
        InferenceResult() except +
        float getValue() except +
        vector[Policy] getPolicies() except +
        void getTerritories(float* buffer) except +
