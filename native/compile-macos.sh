#!/bin/bash

# macOS compile script for Google Balls Desktop
# This script compiles the SDL2 application on macOS with Homebrew dependencies

set -e  # Exit on any error

echo "Compiling Google Balls Desktop for macOS..."

# Check if SDL2 is installed
if ! brew list sdl2 &>/dev/null; then
    echo "Error: SDL2 not found. Please install with: brew install sdl2 sdl2_image"
    exit 1
fi

if ! brew list sdl2_image &>/dev/null; then
    echo "Error: SDL2_image not found. Please install with: brew install sdl2 sdl2_image"
    exit 1
fi

# Get SDL2 flags from pkg-config
SDL2_CFLAGS=$(pkg-config --cflags sdl2)
SDL2_LIBS=$(pkg-config --libs sdl2)
SDL2_IMAGE_LIBS=$(pkg-config --libs SDL2_image)

# Compile the application
clang++ -std=c++11 -O2 \
    balls.cpp icon/balls.c \
    -o googleballs-desktop-macos \
    $SDL2_CFLAGS \
    $SDL2_LIBS $SDL2_IMAGE_LIBS \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreVideo \
    -framework CoreAudio \
    -framework AudioToolbox \
    -framework ForceFeedback \
    -framework Metal \
    -framework QuartzCore \
    -framework Security

echo "Successfully compiled: googleballs-desktop-macos"

# Optional: Create a simple .app bundle
echo "Creating .app bundle..."

APP_NAME="GoogleBallsDesktop"
APP_BUNDLE="${APP_NAME}.app"

# Create app bundle structure
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"

# Move executable to bundle
cp googleballs-desktop-macos "${APP_BUNDLE}/Contents/MacOS/${APP_NAME}"

# Create Info.plist
cat > "${APP_BUNDLE}/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>${APP_NAME}</string>
    <key>CFBundleIdentifier</key>
    <string>com.googleballs.desktop</string>
    <key>CFBundleName</key>
    <string>Google Balls Desktop</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.14</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
</dict>
</plist>
EOF

# Copy icon if it exists (you might want to create an .icns file)
if [ -f "icon/balls.icns" ]; then
    cp "icon/balls.icns" "${APP_BUNDLE}/Contents/Resources/icon.icns"
fi

echo "Created app bundle: ${APP_BUNDLE}"
echo "Build complete!"