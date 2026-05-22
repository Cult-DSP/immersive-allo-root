#!/bin/bash

# Configure debug build
(
  mkdir -p build/Debug
  cmake -DCMAKE_BUILD_TYPE=Debug -Wno-deprecated -DBUILD_EXAMPLES=0 \
    -B build/Debug -S .
)

(
  cmake --build build/Debug -j 9
)

result=$?
if [ ${result} == 0 ]; then
  cd ./bin
  lldb -o run ./immersive-allo-root
fi
