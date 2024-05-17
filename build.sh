#!/bin/bash
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cp sample.ogg build/sample.ogg
./build/sfmlopengl
