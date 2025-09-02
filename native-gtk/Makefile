APP_NAME      ?= googleballs-desktop-gtk
CC            ?= gcc
PKG_CONFIG    ?= pkg-config
PKGS          := gtk+-3.0

# Build config
CSTD          ?= c11
WARNINGS      ?= -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
OPT_RELEASE   ?= -O2 -DNDEBUG
OPT_DEBUG     ?= -O0 -g3

# Sources
SRC           := main.c

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
	$(CC) -std=$(CSTD) $(WARNINGS) $(CFLAGS) $(OPT_RELEASE) $(PKG_CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(OUT_DEBUG): $(SRC) | $(BUILD_DIR)
	$(CC) -std=$(CSTD) $(WARNINGS) $(CFLAGS) $(OPT_DEBUG) $(PKG_CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(BUILD_DIR)
