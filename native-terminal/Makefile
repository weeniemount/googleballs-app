# Cross-platform Makefile for Google Balls Terminal Edition

CXX := g++
CXXFLAGS := -std=c++11 -O2 -Wall
TARGET_BASE := balls

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET_BASE).exe
    LDFLAGS := 
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        TARGET := $(TARGET_BASE)-macos
        LDFLAGS := 
    else
        TARGET := $(TARGET_BASE)-linux
        LDFLAGS := 
    endif
endif

# Build directories
BUILD_DIR := build
DIST_DIR := dist

# Source files
SOURCES := balls.cpp

.PHONY: all clean windows linux macos dist

# Default target builds for current platform
all: $(TARGET)

# Build for current platform
$(TARGET): $(SOURCES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(BUILD_DIR)/$(TARGET) $(LDFLAGS)
	@echo "Built: $(BUILD_DIR)/$(TARGET)"

# Cross-compile for Windows (requires MinGW)
windows:
	@mkdir -p $(DIST_DIR)
	x86_64-w64-mingw32-g++ $(CXXFLAGS) $(SOURCES) -o $(DIST_DIR)/$(TARGET_BASE).exe -static-libgcc -static-libstdc++
	@echo "Built: $(DIST_DIR)/$(TARGET_BASE).exe"

# Build for Linux
linux:
	@mkdir -p $(DIST_DIR)
	g++ $(CXXFLAGS) $(SOURCES) -o $(DIST_DIR)/$(TARGET_BASE)-linux $(LDFLAGS)
	@echo "Built: $(DIST_DIR)/$(TARGET_BASE)-linux"

# Build for macOS (run this on macOS or use osxcross)
macos:
	@mkdir -p $(DIST_DIR)
	g++ $(CXXFLAGS) $(SOURCES) -o $(DIST_DIR)/$(TARGET_BASE)-macos $(LDFLAGS)
	@echo "Built: $(DIST_DIR)/$(TARGET_BASE)-macos"

# Build all platforms (requires cross-compilation tools)
dist: linux windows macos
	@echo "All platforms built successfully!"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
	@echo "Cleaned build directories"
