#!/bin/bash

# macOS compile script for Google Balls Desktop
# This script compiles a universal (x86_64 + arm64) SDL2 application

set -e  # Exit on any error

echo "Compiling Google Balls Desktop for macOS (Universal Binary)..."

# Configuration
APP_NAME="GoogleBallsDesktop"
EXECUTABLE_NAME="googleballs-desktop-macos"
APP_BUNDLE="${APP_NAME}.app"

# Check if SDL2 is installed
if ! brew list sdl2 &>/dev/null; then
    echo "Error: SDL2 not found. Please install with: brew install sdl2 sdl2_image"
    exit 1
fi

if ! brew list sdl2_image &>/dev/null; then
    echo "Error: SDL2_image not found. Please install with: brew install sdl2 sdl2_image"
    exit 1
fi

# Get Homebrew prefix (different on Intel vs Apple Silicon)
BREW_PREFIX=$(brew --prefix)
echo "Using Homebrew prefix: $BREW_PREFIX"

# Check if we have the source files
if [ ! -f "balls.cpp" ]; then
    echo "Error: balls.cpp not found in current directory"
    exit 1
fi

# Create icon object file if icon exists
ICON_OBJ=""
if [ -f "icon/balls.c" ]; then
    echo "Compiling icon resource..."
    ICON_OBJ="icon/balls.o"
    
    # Compile icon for both architectures
    clang -c -arch x86_64 -arch arm64 icon/balls.c -o $ICON_OBJ
else
    echo "Warning: icon/balls.c not found, skipping icon embedding"
fi

# Compiler flags
COMMON_FLAGS="-std=c++11 -O2 -I${BREW_PREFIX}/include -I${BREW_PREFIX}/include/SDL2"

# Try to link statically with SDL2 libraries
SDL2_STATIC_LIBS=""
if [ -f "${BREW_PREFIX}/lib/libSDL2.a" ]; then
    echo "Found static SDL2 libraries, using static linking..."
    SDL2_STATIC_LIBS="${BREW_PREFIX}/lib/libSDL2.a ${BREW_PREFIX}/lib/libSDL2_image.a"
    
    # Add dependencies that static SDL2 needs
    EXTRA_LIBS="-liconv -lz -framework CoreFoundation"
    
    # Check for additional image format libraries
    if [ -f "${BREW_PREFIX}/lib/libpng.a" ]; then
        SDL2_STATIC_LIBS="${SDL2_STATIC_LIBS} ${BREW_PREFIX}/lib/libpng.a"
    fi
    if [ -f "${BREW_PREFIX}/lib/libjpeg.a" ]; then
        SDL2_STATIC_LIBS="${SDL2_STATIC_LIBS} ${BREW_PREFIX}/lib/libjpeg.a"
    fi
else
    echo "Static SDL2 libraries not found, using dynamic linking..."
    SDL2_STATIC_LIBS="-lSDL2 -lSDL2_image -L${BREW_PREFIX}/lib"
    EXTRA_LIBS=""
fi

# System frameworks required by SDL2
FRAMEWORKS="-framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio -framework AudioToolbox -framework ForceFeedback -framework Metal -framework QuartzCore -framework Security -framework GameController"

# Compile universal binary
echo "Compiling universal binary..."
clang++ $COMMON_FLAGS \
    -arch x86_64 -arch arm64 \
    balls.cpp $ICON_OBJ \
    -o $EXECUTABLE_NAME \
    $SDL2_STATIC_LIBS \
    $FRAMEWORKS \
    $EXTRA_LIBS

echo "Successfully compiled universal binary: $EXECUTABLE_NAME"

# Verify it's universal
echo "Verifying universal binary:"
file $EXECUTABLE_NAME
lipo -info $EXECUTABLE_NAME

# Create .app bundle
echo "Creating .app bundle..."

# Clean up any existing bundle
rm -rf "$APP_BUNDLE"

# Create app bundle structure
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"

# Move executable to bundle
cp $EXECUTABLE_NAME "${APP_BUNDLE}/Contents/MacOS/${APP_NAME}"
chmod +x "${APP_BUNDLE}/Contents/MacOS/${APP_NAME}"

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
    <key>CFBundleDisplayName</key>
    <string>Google Balls Desktop</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>GBLZ</string>
    <key>CFBundleIconFile</key>
    <string>icon</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.14</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.games</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
</dict>
</plist>
EOF

# Copy the existing .icns icon
if [ -f "icon/balls.icns" ]; then
    cp "icon/balls.icns" "${APP_BUNDLE}/Contents/Resources/icon.icns"
    echo "✓ Copied .icns icon file"
elif [ -f "icon/icon.icns" ]; then
    cp "icon/icon.icns" "${APP_BUNDLE}/Contents/Resources/icon.icns"
    echo "✓ Copied .icns icon file"
else
    echo "Warning: No .icns icon file found in icon/ directory"
    # Remove icon reference from Info.plist if no icon found
    sed -i '' '/<key>CFBundleIconFile<\/key>/,+1d' "${APP_BUNDLE}/Contents/Info.plist"
fi

# Check dependencies
echo ""
echo "Checking dynamic library dependencies:"
otool -L "${APP_BUNDLE}/Contents/MacOS/${APP_NAME}" | grep -v ":" | head -10

echo ""
echo "Build complete!"
echo "Created:"
echo "  - Standalone executable: $EXECUTABLE_NAME"
echo "  - macOS app bundle: $APP_BUNDLE"
echo ""

# Final verification
echo "Final verification:"
echo "Universal binary check:"
lipo -info $EXECUTABLE_NAME

echo ""
echo "App bundle structure:"
find "$APP_BUNDLE" -type f

echo ""
echo "You can now run:"
echo "  ./$EXECUTABLE_NAME"
echo "  or"
echo "  open $APP_BUNDLE"