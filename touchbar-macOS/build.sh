#!/usr/bin/env bash
set -euo pipefail

APP_NAME="GoogleBallsTouchBar.app"
EXECUTABLE="GoogleBallsTouchBar"
APP_DIR="${APP_NAME}/Contents"

mkdir -p "${APP_DIR}/MacOS"

clang \
  -fobjc-arc \
  -ObjC \
  -framework Cocoa \
  -o "${APP_DIR}/MacOS/${EXECUTABLE}" \
  main.m

cat > "${APP_DIR}/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleExecutable</key>
  <string>GoogleBallsTouchBar</string>
  <key>CFBundleIdentifier</key>
  <string>app.googleballs.touchbar.macos</string>
  <key>CFBundleName</key>
  <string>Google Balls Touch Bar</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>1.0</string>
  <key>CFBundleVersion</key>
  <string>1</string>
  <key>LSMinimumSystemVersion</key>
  <string>10.12.2</string>
  <key>NSHighResolutionCapable</key>
  <true/>
</dict>
</plist>
PLIST

echo "Built ${APP_NAME}"
