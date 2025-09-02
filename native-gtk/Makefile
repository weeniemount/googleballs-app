APP_NAME      ?= googleballs-desktop-gtk
CC            ?= gcc
PKG_CONFIG    ?= pkg-config
PKGS          := gtk+-3.0

ifeq ($(OS),Windows_NT)
	WINDOWS := 1
	WINDRES := windres
else
	WINDOWS := 0
endif

# Build config
CSTD          ?= c11
WARNINGS      ?= -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
OPT_RELEASE   ?= -O2 -DNDEBUG
OPT_DEBUG     ?= -O0 -g3

# Sources
SRC           := main.c icon/balls.c

# Windows-specific additions
ifeq ($(WINDOWS),1)
	RESOURCE := icon/resource.o
	SRC := $(SRC) $(RESOURCE)
	LINK_FLAGS := -mwindows
else
	LINK_FLAGS :=
endif

# Build directories and outputs
BUILD_DIR     ?= build
OUT_RELEASE   := $(BUILD_DIR)/$(APP_NAME)
OUT_DEBUG     := $(BUILD_DIR)/$(APP_NAME)-debug

# pkg-config flags
PKG_CFLAGS    := $(shell $(PKG_CONFIG) --cflags $(PKGS))
PKG_LIBS      := $(shell $(PKG_CONFIG) --libs $(PKGS))

# Link against math
LIBS          := $(PKG_LIBS) -lm

.PHONY: all release debug run clean

all: release

release: $(OUT_RELEASE)

debug: $(OUT_DEBUG)

run: release
	./$(OUT_RELEASE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OUT_RELEASE): $(SRC) | $(BUILD_DIR)
	$(CC) -std=$(CSTD) $(WARNINGS) $(CFLAGS) $(OPT_RELEASE) $(PKG_CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LIBS) $(LINK_FLAGS)

$(OUT_DEBUG): $(SRC) | $(BUILD_DIR)
	$(CC) -std=$(CSTD) $(WARNINGS) $(CFLAGS) $(OPT_DEBUG) $(PKG_CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LIBS) $(LINK_FLAGS)

clean:
	rm -rf $(BUILD_DIR)
ifeq ($(WINDOWS),1)
	rm -f icon/resource.o
endif