#!/bin/bash

# Fully static compilation
g++ -O2 balls.cpp -o googleballs-desktop \
    -I/usr/local/include/SDL2 \
    -L/usr/local/lib \
    -static \
    -Wl,-Bstatic \
    -lSDL2main -lSDL2 \
    -Wl,-Bdynamic \
    -lpthread -ldl -lm
