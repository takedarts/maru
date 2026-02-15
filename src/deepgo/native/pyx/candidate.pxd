from libc.stdint cimport int32_t
from libcpp.pair cimport pair
from libcpp.vector cimport vector


cdef extern from "cpp/Candidate.h" namespace "deepgo":
    cdef cppclass Candidate:
        int32_t getX()
        int32_t getY()
        int32_t getColor()
        int32_t getVisits()
        int32_t getPlayouts()
        float getPolicy()
        float getValue()
        float getMinimax()
        vector[pair[int32_t, int32_t]] getVariations()
