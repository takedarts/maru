from libc.stdint cimport int32_t
from libcpp.vector cimport vector
from pyx.move cimport Move


cdef extern from "cpp/Candidate.h" namespace "deepgo":
    cdef cppclass Candidate:
        Candidate() except +
        Candidate(const Candidate& other) except +
        Move getMove()
        int32_t getVisits()
        float getPolicy()
        float getValue()
        float getScore()
        vector[Move] getVariations()
        void getTerritories(float* territories)
