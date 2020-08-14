# distutils: sources = ../src/rct2env/Env.cpp

from __future__ import print_function

print('prct2env.pyx')
def fib(n):
    """Print the Fibonacci series up to n."""
    a, b = 0, 1
    while b < n:
        print(b, end=' ')
        a, b = b, a + b

    print()

def rct2env():
    cdef rct2env = RCT2Env()
