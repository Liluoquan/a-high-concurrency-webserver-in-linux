#!/bin/bash

#编译
make clean && make -j8 && make install
rm -f *.o


