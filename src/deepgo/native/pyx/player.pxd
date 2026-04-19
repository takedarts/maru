from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector
from pyx.candidate cimport Candidate
from pyx.processor cimport Processor


cdef extern from "cpp/Player.h" namespace "deepgo":
    cdef cppclass Player:
        Player(
            Processor* processor, int32_t threads, int32_t cacheSize,
            int32_t width, int32_t height, float komi, int32_t rule, bool superko,
            float ucbConstant, float pucbConstantInit, float pucbConstantBase,
            bool evalLeafOnly, int32_t maxVisits) except +
        void initialize()
        int32_t play(int32_t x, int32_t y)
        vector[Candidate] getPass() nogil
        vector[Candidate] getRandom(float temperature) nogil
        void startEvaluation(
            bool equally, int32_t algorithm, int32_t width, float temperature, float noise)
        void waitEvaluation(int32_t visits, int32_t playouts, float timeout, bool stop) nogil
        vector[Candidate] getCandidates()
        int32_t getColor()
        vector[int32_t] getBoardState()
        string getDebugInfo()
