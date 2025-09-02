#!/bin/bash

# Compile with static linking for gcc/g++ libs
g++ -O2 balls.cpp -o googleballs-desktop \
    $(sdl2-config --cflags) \
    -static-libgcc -static-libstdc++ \
    $(sdl2-config --libs) \
    -lpthread -ldl
