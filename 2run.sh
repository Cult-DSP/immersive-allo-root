#!/bin/bash

mkdir -p build/Release
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DRTAUDIO_API_JACK=OFF -DRTMIDI_API_JACK=OFF \
  -Wno-deprecated -DBUILD_EXAMPLES=0 \
  -B build/Release -S .

cmake --build build/Release --config Release -j 9

result=$?
if [ ${result} == 0 ]; then
  cd bin
  ./immersive-allo-root
fi
