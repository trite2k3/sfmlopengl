# sfmlopengl


the google and copy paste is real!


[![cat](https://img.youtube.com/vi/8vnNBKrEGpA/0.jpg)](https://www.youtube.com/watch?v=8vnNBKrEGpA)

Requires: SFML, GLM

sudo apt install build-essential cmake libsfml-dev libglm-dev libglew-dev

change window.setFramerateLimit(144); to whatever hz your monitor is (or limit for your hardware)

vsync is disabled.

build with:



cmake -B build -DCMAKE_BUILD_TYPE=Release


cmake --build build --config Release
