mkdir build
cd build
cmake ../
make -j8 &&\
#DESTDIR=. make install
sudo make install
