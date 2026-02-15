from libc.stdint cimport int32_t, uint32_t
from libcpp cimport bool
from libcpp.string cimport string


cdef extern from "cpp/Model.h" namespace "deepgo":
    cdef cppclass Model:
        Model(string, int32_t, bool, bool) except +
        void forward(int32_t*, float*, uint32_t)
        int32_t isCuda()
