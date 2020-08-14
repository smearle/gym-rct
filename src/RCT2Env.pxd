
cdef extern from "../src/rct2env/Env.h" namespace "OpenRCT2":
    cdef cppclass RCT2Env:
        RCT2Env() except +

cdef extern from "../src/rct2env/Env.cpp":
    pass
