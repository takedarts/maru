from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector
from pyx.candidate cimport Candidate
from pyx.processor cimport Processor


cdef extern from "cpp/Player.h" namespace "deepgo":
    cdef cppclass Player:
        Player(
            Processor*, int32_t, int32_t, int32_t, float, int, bool, 
            float, float, float, bool, int32_t) except +
        void initialize()
        int32_t play(int32_t, int32_t)
        vector[Candidate] getPass() nogil
        vector[Candidate] getRandom(float) nogil
        void startEvaluation(bool, int32_t, int32_t, float, float)
        void waitEvaluation(int32_t, int32_t, float, bool) nogil
        vector[Candidate] getCandidates()
        int32_t getColor()
        vector[int32_t] getBoardState()
        string getDebugInfo()
