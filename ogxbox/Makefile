# NXDK Makefile for Google Balls - OG Xbox
# Requires NXDK to be installed at $(NXDK_DIR)
#
# Usage:
#   export NXDK_DIR=/path/to/nxdk
#   make

TARGET = balls.xbe
SRCS   = main.cpp

# NXDK handles SDL2 internally via its own port
SDL2       = y
SDL2_IMAGE = n   # not needed; icon is embedded

include $(NXDK_DIR)/Makefile