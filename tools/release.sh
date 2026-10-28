#!/bin/bash
# Builds a standalone, ad-hoc signed Brotherhood Studio.app (Qt frameworks embedded) and zips it for a GitHub release.
set -euo pipefail
cd "$(dirname "$0")/.."
QT="$(brew --prefix qt)"
BUILD="${BS_BUILD_DIR:-build-release}"   # the build folder path must not contain "&" (Qt's generated shell commands break on it): set BS_BUILD_DIR when the project lives under one
VER="$(grep -m1 'project(BrotherhoodStudio VERSION' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/')"
cmake -S . -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QT" -DBS_BUNDLE_ECOM_STYLE=OFF   # release zips are public: no brand catalog photos inside
cmake --build "$BUILD"
APP="$BUILD/Brotherhood Studio.app"
"$QT/bin/macdeployqt" "$APP" -qmldir=qml -always-overwrite
tools/prune_bundle.sh "$APP"
codesign --force --deep --sign - "$APP"
ZIP="dist/Brotherhood-Studio-$VER-arm64-mac.zip"
mkdir -p dist
rm -f "$ZIP"
ditto -c -k --keepParent "$APP" "$ZIP"
echo "$ZIP"
