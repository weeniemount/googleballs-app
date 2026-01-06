# Makefile for Google Balls Desktop
# Supports Linux, macOS, and Windows builds

# Detect OS
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

# Common variables
APP_NAME = GoogleBallsDesktopSDL2
SOURCES = balls.cpp
ICON_C = icon/balls.c

# Default target
.PHONY: all
all: build

# Linux build
.PHONY: linux
linux:
	g++ -O2 $(SOURCES) $(ICON_C) -o googleballs-desktop-sdl2 \
		-I/usr/local/include/SDL2 \
		-L/usr/local/lib \
		-lSDL2 -lSDL2_image

# macOS build
.PHONY: macos
macos: macos-binary macos-bundle

.PHONY: macos-binary
macos-binary:
	@echo "Compiling Google Balls Desktop for macOS (Universal Binary)..."
	@if ! brew list sdl2 &>/dev/null; then \
		echo "Error: SDL2 not found. Install with: brew install sdl2 sdl2_image"; \
		exit 1; \
	fi
	@if ! brew list sdl2_image &>/dev/null; then \
		echo "Error: SDL2_image not found. Install with: brew install sdl2 sdl2_image"; \
		exit 1; \
	fi
	$(eval BREW_PREFIX := $(shell brew --prefix))
	@echo "Using Homebrew prefix: $(BREW_PREFIX)"
	@if [ -f "$(ICON_C)" ]; then \
		echo "Compiling icon resource..."; \
		clang -c -arch x86_64 -arch arm64 $(ICON_C) -o icon/balls.o; \
	fi
	@if [ -f "$(BREW_PREFIX)/lib/libSDL2.a" ]; then \
		echo "Using static SDL2 libraries..."; \
		clang++ -std=c++11 -O2 -arch x86_64 -arch arm64 \
			-I$(BREW_PREFIX)/include -I$(BREW_PREFIX)/include/SDL2 \
			$(SOURCES) icon/balls.o -o googleballs-desktop-macos \
			$(BREW_PREFIX)/lib/libSDL2.a $(BREW_PREFIX)/lib/libSDL2_image.a \
			-framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio \
			-framework AudioToolbox -framework ForceFeedback -framework Metal \
			-framework QuartzCore -framework Security -framework GameController \
			-framework CoreFoundation -liconv -lz; \
	else \
		echo "Using dynamic SDL2 libraries..."; \
		clang++ -std=c++11 -O2 -arch x86_64 -arch arm64 \
			-I$(BREW_PREFIX)/include -I$(BREW_PREFIX)/include/SDL2 \
			$(SOURCES) icon/balls.o -o googleballs-desktop-macos \
			-L$(BREW_PREFIX)/lib -lSDL2 -lSDL2_image \
			-framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio \
			-framework AudioToolbox -framework ForceFeedback -framework Metal \
			-framework QuartzCore -framework Security -framework GameController; \
	fi
	@echo "Successfully compiled: googleballs-desktop-macos"
	@file googleballs-desktop-macos
	@lipo -info googleballs-desktop-macos

.PHONY: macos-bundle
macos-bundle: macos-binary
	@echo "Creating .app bundle..."
	@rm -rf $(APP_NAME).app
	@mkdir -p $(APP_NAME).app/Contents/MacOS
	@mkdir -p $(APP_NAME).app/Contents/Resources
	@cp googleballs-desktop-macos $(APP_NAME).app/Contents/MacOS/$(APP_NAME)
	@chmod +x $(APP_NAME).app/Contents/MacOS/$(APP_NAME)
	@echo '<?xml version="1.0" encoding="UTF-8"?>' > $(APP_NAME).app/Contents/Info.plist
	@echo '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >> $(APP_NAME).app/Contents/Info.plist
	@echo '<plist version="1.0"><dict>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '  <key>CFBundleExecutable</key><string>$(APP_NAME)</string>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '  <key>CFBundleIdentifier</key><string>com.googleballs.desktop</string>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '  <key>CFBundleName</key><string>Google Balls Desktop</string>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '  <key>CFBundleDisplayName</key><string>Google Balls Desktop</string>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '  <key>CFBundleVersion</key><string>1.0.0</string>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '  <key>CFBundlePackageType</key><string>APPL</string>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '  <key>LSMinimumSystemVersion</key><string>10.14</string>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '  <key>NSHighResolutionCapable</key><true/>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '  <key>LSApplicationCategoryType</key><string>public.app-category.games</string>' >> $(APP_NAME).app/Contents/Info.plist
	@echo '</dict></plist>' >> $(APP_NAME).app/Contents/Info.plist
	@if [ -f "icon/balls.icns" ]; then \
		cp icon/balls.icns $(APP_NAME).app/Contents/Resources/icon.icns; \
		echo "Copied icon file"; \
	fi
	@echo "Build complete! Created $(APP_NAME).app"

# Windows build
icon/resource.o: icon/resource.rc
	windres icon/resource.rc -o icon/resource.o

.PHONY: windows
windows: icon/resource.o
	g++ -O2 $(SOURCES) $(ICON_C) icon/resource.o -o googleballs-desktop.exe \
		-Lsdl2/ -lmingw32 -lSDL2main -lSDL2 -lSDL2_image \
		-ljxl -ljxl_threads -lhwy -lbrotlienc -lbrotlidec -lbrotlicommon \
		-lavif -laom -ldav1d -lrav1e -lSvtAv1Enc -lyuv \
		-ltiff -ljpeg -lsharpyuv -lwebp -lwebpdemux -lpng -ljbig -llzma -lzstd -ldeflate -lz \
		-lsetupapi -limm32 -lole32 -loleaut32 -lwinmm -lversion -lcfgmgr32 \
		-lgdi32 -luser32 -lshell32 -ladvapi32 -lws2_32 \
		-mwindows \
		-static -static-libgcc -static-libstdc++

		
# Auto-detect build
.PHONY: build
build:
ifeq ($(UNAME_S),Linux)
	@$(MAKE) linux
else ifeq ($(UNAME_S),Darwin)
	@$(MAKE) macos
else
	@$(MAKE) windows
endif

# Clean build artifacts
.PHONY: clean
clean:
	rm -f googleballs-desktop-sdl2 googleballs-desktop-macos googleballs-desktop.exe
	rm -f icon/balls.o icon/*.o
	rm -rf $(APP_NAME).app
