from libc.stdint cimport int8_t, int32_t


cdef extern from "cpp/Policy.h" namespace "deepgo":
  cdef cppclass Policy:
    Policy(int8_t x, int8_t y, float probability, int32_t visits) except +
    Policy() except +
    int8_t getX() const
    int8_t getY() const
    float getProbability() const
    int32_t getVisits() const
