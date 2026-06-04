from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector
from pyx.candidate cimport Candidate
from pyx.inference cimport InferenceProcessor
from pyx.move cimport Move


cdef extern from "cpp/Player.h" namespace "deepgo":
    cdef cppclass Player:
        Player(
            InferenceProcessor* processor, int32_t threads, int32_t maxVisits,
            int32_t width, int32_t height, float komi, int32_t rule, bool superko,
            float pucbConstantInit, float pucbConstantBase) except +
        void initialize()
        int32_t play(Move move) nogil
        vector[Candidate] getPass() nogil
        void startEvaluation(
            bool equally, int32_t candidateWidth, float temperature, float noise) nogil
        void waitEvaluation(int32_t visits, int32_t playouts, float timeout, bool stop) nogil
        vector[Candidate] getCandidates() nogil
        int32_t getColor()
        vector[int32_t] getBoardState()
        string toString()
