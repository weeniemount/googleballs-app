
CXX = g++
CC = gcc
CXXFLAGS = -std=c++11 -Wall -O2
CFLAGS = -Wall -O2
LIBS = -lwayland-client -lwayland-cursor -lrt

XDG_SHELL_PROTOCOL = ./xdg-shell.xml

XDG_DECORATION_PROTOCOL = ./xdg-decoration-unstable-v1.xml

all: google-balls-wayland

xdg-shell-protocol.c:
	wayland-scanner public-code $(XDG_SHELL_PROTOCOL) $@

xdg-shell-client-protocol.h:
	wayland-scanner client-header $(XDG_SHELL_PROTOCOL) $@

xdg-decoration-protocol.c:
	wayland-scanner public-code $(XDG_DECORATION_PROTOCOL) $@

xdg-decoration-client-protocol.h:
	wayland-scanner client-header $(XDG_DECORATION_PROTOCOL) $@

# Compile C file with GCC
xdg-shell-protocol.o: xdg-shell-protocol.c xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c -o $@ $<

xdg-decoration-protocol.o: xdg-decoration-protocol.c xdg-decoration-client-protocol.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile C++ file with G++
main.o: main.cpp xdg-shell-client-protocol.h xdg-decoration-client-protocol.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Link with G++
google-balls-wayland: main.o xdg-shell-protocol.o xdg-decoration-protocol.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f google-balls-wayland *.o xdg-shell-protocol.c xdg-shell-client-protocol.h xdg-decoration-protocol.c xdg-decoration-client-protocol.h

.PHONY: all clean
