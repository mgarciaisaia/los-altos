#!/bin/sh
# Estaria bueno hacer un makefile general :)
cd commons/Debug
make clean
make all
cd ../../memcached-1.6
./configure --prefix=/usr/local
make
cd ../fsc/Debug
make clean
make all
cd ../../rfs/Debug
make clean
make all
