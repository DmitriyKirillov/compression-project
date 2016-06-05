#!/bin/bash

mkdir build
cd build
cmake ../
make -j 3
cd ../
rm tester
ln -s build/codecs/tools/tester/tools-tester tester
