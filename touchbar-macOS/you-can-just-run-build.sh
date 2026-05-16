#!/usr/bin/env bash
set -euo pipefail

./build.sh
exec ./GoogleBallsTouchBar.app/Contents/MacOS/GoogleBallsTouchBar
