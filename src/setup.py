
from setuptools import setup, find_packages
from setuptools.extension import Extension
from Cython.Build import cythonize

extensions = [
        Extension(
            "rct2env",
            ["rct2env.pyx"],
        include_dirs=[
           #'../../../../usr/include',
           #'/usr/local/include',
            '../../libtorch/include/torch/csrc/api/include',
           #'../../pytorch-cpp-rl/include',
            '../../libtorch/include',
           #'../../pytorch-cpp-rl/example/lib/spdlog/include',
            ],
        libraries=[
            'torch', 'c++',
            'icuuc',
           #'icutu',
           #'icuio',
           #'icudata',
           #'icui18n',
           #'icutest',
           #'icule',
            'openrct2',
            'z',
            'jansson',
            ],
        library_dirs=[
            #'../../pytorch-cpp-rl',
             '../../libtorch/lib',
            #'/usr/lib/x86_64-linux-gnu',
            ],
        extra_compile_args=['-std=c++17', '-v'],
        ),
]
                 
setup(
    name = "rct2env",
    packages = find_packages(),
   ext_modules = cythonize(extensions)
   )
