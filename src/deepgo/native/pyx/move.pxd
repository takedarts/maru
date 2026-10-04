from libc.stdint cimport int8_t, int32_t
from libcpp cimport bool
from libcpp.pair cimport pair
from libcpp.vector cimport vector


cdef extern from "cpp/Move.h" namespace "deepgo":
    cdef cppclass Move:
        Move(int8_t x, int8_t y, int8_t color) except +
        Move() except +
        int8_t getX()
        int8_t getY()
        int8_t getColor()


cdef extern from "cpp/MoveResult.h" namespace "deepgo":
    cdef cppclass MoveResult:
        MoveResult() except +
        MoveResult(
            const Move& move,
            int32_t captured,
            bool capturedUp,
            bool capturedRight,
            bool capturedDown,
            bool capturedLeft,
            const pair[int32_t, int32_t]& previousKo,
        ) except +
        int32_t getX()
        int32_t getY()
        int32_t getColor()
        int32_t getCaptured()
        bool getCapturedDirection(int32_t direction) except +
        pair[int32_t, int32_t] getPreviousKo()
