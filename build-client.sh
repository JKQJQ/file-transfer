#!/bin/bash

cd client/
cmake -S . -B build/
cd build/
make -j8
