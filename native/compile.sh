#!/bin/bash

# Compile with static SDL2 but dynamic system libraries
g++ -O2 balls.cpp -o googleballs-desktop \
    -I/usr/local/include/SDL2 \
    -L/usr/local/lib \
    -static-libgcc -static-libstdc++ \
    -Wl,-Bstatic -lSDL2 -lSDL2main \
    -Wl,-Bdynamic -lm -ldl -lpthread -lrt -lX11 -lXext -lXcursor -lXinerama -lXi -lXrandr -lXss -lXxf86vm -lwayland-client -lwayland-cursor -lwayland-egl -lxkbcommon
