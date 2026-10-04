from libc.stdint cimport int32_t, uint32_t, uint64_t
from libcpp cimport bool
from libcpp.pair cimport pair
from libcpp.vector cimport vector
from pyx.move cimport Move, MoveResult


cdef extern from "cpp/Board.h" namespace "deepgo":
    cdef cppclass Board:
        Board(int32_t width, int32_t height) except +
        int32_t getWidth()
        int32_t getHeight()
        MoveResult play(Move move) except +
        void undo(const MoveResult& result) except +
        pair[int32_t, int32_t] getKo(int32_t color)
        int32_t getColor(int32_t x, int32_t y)
        void getColors(int32_t* colors, int32_t size)
        vector[uint32_t] getPattern()
        int32_t getRenSize(int32_t x, int32_t y)
        int32_t getRenSpace(int32_t x, int32_t y)
        bool isShicho(int32_t x, int32_t y)
        bool isEnabled(int32_t x, int32_t y, int32_t color, bool checkSeki)
        void getEnableds(int32_t* enableds, int32_t size, bool checkSeki)
        void getFixedTerritories(int32_t* territories)
        void getOwners(int32_t* owners, const int32_t* territories, int32_t rule)
        void getInputs(int32_t* inputs, int32_t color, float komi, int32_t rule, bool superko)
        void copyFrom(Board*)
