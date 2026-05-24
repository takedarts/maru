from libc.stdint cimport int8_t
from libcpp.vector cimport vector


cdef extern from "cpp/Move.h" namespace "deepgo":
    cdef cppclass Move:
        Move(int8_t x, int8_t y, int8_t color) except +
        Move() except +
        int8_t getX()
        int8_t getY()
        int8_t getColor()
