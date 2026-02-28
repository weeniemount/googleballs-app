# NXDK Makefile for Google Balls - OG Xbox
TARGET = balls.xbe
SRCS   = main.cpp

# NXDK handles SDL2 internally via its own port
SDL2       = y
SDL2_IMAGE = n

# Point the compiler at NXDK's bundled SDL2 headers explicitly.
# Must be set before including NXDK's Makefile so they take priority
# over any system SDL2 headers (or absence thereof).
CXXFLAGS += -I$(NXDK_DIR)/lib/SDL2/include

include $(NXDK_DIR)/Makefile