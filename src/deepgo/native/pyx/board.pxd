from libc.stdint cimport int32_t
from libcpp cimport bool
from libcpp.pair cimport pair
from libcpp.vector cimport vector


cdef extern from "cpp/Board.h" namespace "deepgo":
    cdef cppclass Board:
        Board(int32_t, int32_t) except +
        int32_t getWidth()
        int32_t getHeight()
        int32_t play(int32_t, int32_t, int32_t)
        pair[int32_t, int32_t] getKo(int32_t)
        vector[pair[int32_t, int32_t]] getHistories(int32_t)
        int32_t getColor(int32_t, int32_t)
        void getColors(int32_t*, int32_t)
        int32_t getRenSize(int32_t, int32_t)
        int32_t getRenSpace(int32_t, int32_t)
        bool isShicho(int32_t, int32_t)
        bool isEnabled(int32_t, int32_t, int32_t, bool)
        void getEnableds(int32_t*, int32_t, bool)
        void getTerritories(int32_t*, int32_t)
        void getOwners(int32_t*, int32_t, int32_t)
        vector[int32_t] getPatterns()
        void getInputs(int32_t*, int32_t, float, int32_t, bool)
        vector[int32_t] getState()
        void loadState(vector[int32_t])
        void copyFrom(Board*)
