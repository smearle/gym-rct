from setuptools import setup, find_packages
from setuptools.extension import Extension
from Cython.Build import cythonize

extensions = [
        Extension(
            "rct2env",
            ["rct2env.pyx"],
        include_dirs['/home/sme/libtorch'],
        libraries=['torch'],
        ),
]
                 
setup(
    name = "rct2env",
    packages = find_packages(),
   ext_modules = cythonize(extensions)
   )

