# distutils: language = c++

from RCT2Env cimport RCT2Env

# Create a Cython extension type which holds a C++ instance
# as an attribute and create a bunch of forwarding methods
# Python extension type.
cdef class PyRCT2Env:
    cdef RCT2Env*c_rct2env  # Hold a C++ instance which we're wrapping

    def __cinit__(self):
        self.c_rct2env = new RCT2Env()



#def main():
#    rrct2env_ptr = new RCT2Env()
#
#    cdef RCT2env rct2env_stack


