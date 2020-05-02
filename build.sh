mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=/home/sme/libtorch -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cmake --build . --config Release
make -j15 &&\
#DESTDIR=. make install
sudo make install
