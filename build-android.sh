#!/usr/bin/env bash
#
# Build the Android (arm64-v8a) version of wxSolitaire and package it into a
# signed release APK with androiddeployqt.
#
# The game is compiled against the wxWidgets "Qt" port (wxQt) that ships
# prebuilt inside the Qt 6.9.3 Android kit, so no wxWidgets rewrite is needed.
#
# Usage:
#   ./build-android.sh                 # configure + build .so + signed APK
#   ./build-android.sh clean           # remove the build directory first
#
# Signing (release APK): provide your existing keystore via these variables,
# either exported in the environment or placed in android-signing.env (which
# this script sources automatically and which is .gitignore'd):
#
#   QT_ANDROID_KEYSTORE_PATH=/absolute/path/to/your.keystore
#   QT_ANDROID_KEYSTORE_ALIAS=your_key_alias
#   QT_ANDROID_KEYSTORE_STORE_PASS=******
#   QT_ANDROID_KEYSTORE_KEY_PASS=******
#
set -euo pipefail

# --- Paths (override by exporting before running) --------------------------
QT_ANDROID="${QT_ANDROID:-/usr/local/qt-6.9.3-android-arm64}"
QT_HOST="${QT_HOST:-/usr/local/qt-6.9.3}"
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}"

QT_CMAKE="$QT_ANDROID/bin/qt-cmake"
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$SRC_DIR/build-android}"

if [[ "${1:-}" == "clean" ]]; then
    echo ">> Removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

# --- Load signing credentials if present -----------------------------------
if [[ -f "$SRC_DIR/android-signing.env" ]]; then
    echo ">> Sourcing android-signing.env"
    # shellcheck disable=SC1091
    source "$SRC_DIR/android-signing.env"
fi

[[ -x "$QT_CMAKE" ]] || { echo "ERROR: qt-cmake not found at $QT_CMAKE"; exit 1; }

# --- Configure --------------------------------------------------------------
# Enabling signing is a configure-time switch (QT_ANDROID_SIGN_APK adds --sign
# to androiddeployqt); the keystore itself is read from the QT_ANDROID_KEYSTORE_*
# environment variables at packaging time.
SIGN_ARGS=()
if [[ -n "${QT_ANDROID_KEYSTORE_PATH:-}" ]]; then
    SIGN_ARGS+=("-DQT_ANDROID_SIGN_APK=ON")
fi

echo ">> Configuring with qt-cmake"
"$QT_CMAKE" -S "$SRC_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" \
    "${SIGN_ARGS[@]}"

# --- Build the application shared library -----------------------------------
echo ">> Building libwxSolitaire_arm64-v8a.so"
cmake --build "$BUILD_DIR" --parallel

# --- Package the APK --------------------------------------------------------
# The 'apk' target runs androiddeployqt. When the QT_ANDROID_KEYSTORE_* vars
# above are set, the release APK is signed; otherwise it is left unsigned.
if [[ -n "${QT_ANDROID_KEYSTORE_PATH:-}" ]]; then
    echo ">> Packaging SIGNED release APK (keystore: $QT_ANDROID_KEYSTORE_PATH)"
else
    echo ">> WARNING: no QT_ANDROID_KEYSTORE_* set - APK will be UNSIGNED"
fi
cmake --build "$BUILD_DIR" --target apk

echo
echo ">> Done. APK(s):"
find "$BUILD_DIR" -name '*.apk' -print
