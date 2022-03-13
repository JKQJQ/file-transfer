#!/bin/bash

cd server/
cmake -S . -B build/
cd build/
make -j8
