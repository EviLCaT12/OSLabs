#!/bin/bash
git pull
mkdir build
cd build
cmake ..
make
echo 'Done'