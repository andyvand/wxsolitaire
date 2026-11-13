#!/bin/sh
#
# Builds Solitaire for classic Mac OS with Retro68 and wxRetro68:
#
#   1. the 68k application        (<out>/m68k/Solitaire.bin, .dsk)
#   2. the PowerPC application    (<out>/ppc/Solitaire.bin, .dsk)
#   3. a fat application holding both - the 68k CODE resources plus the
#      PowerPC code fragment in the data fork with its 'cfrg' - so the one
#      file runs natively on either kind of Mac:
#
#        <out>/Solitaire.bin   MacBinary
#        <out>/Solitaire.dsk   HFS disk image, for Basilisk II, SheepShaver,
#                           QEMU or a real Mac
#
# Usage: retro68/build.sh [output directory]   (default: build-retro68)
# Environment: RETRO68 (default /opt/retro68), CMAKE_BUILD_TYPE (Release),
# SOLITAIRE_CMAKE_ARGS (extra cmake options, e.g. -DSOLITAIRE_TRACE=ON for a
# build that retro68/run-basilisk.sh can test unattended).

set -e

SRC=$(cd "$(dirname "$0")/.." && pwd)
RETRO68=${RETRO68:-/opt/retro68}
BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}
OUT=${1:-"$SRC/build-retro68"}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)

GENERATOR=
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="-G Ninja"
fi

build() {   # build <dir> <toolchain file>
    # shellcheck disable=SC2086
    cmake -S "$SRC" -B "$OUT/$1" $GENERATOR \
          -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
          -DRETRO68_ROOT="$RETRO68" \
          -DCMAKE_TOOLCHAIN_FILE="$2" $SOLITAIRE_CMAKE_ARGS
    cmake --build "$OUT/$1"
}

build m68k "$RETRO68/m68k-apple-macos/cmake/retro68.toolchain.cmake"
build ppc  "$RETRO68/powerpc-apple-macos/cmake/retroppc.toolchain.cmake"

# The fat application. RetroPPCAPPL.r gives the 'cfrg' that points the
# PowerPC Process Manager at the data fork; the 68k one never looks at it and
# runs the CODE resources instead. The PowerPC build's Finder resources come
# last so that their SIZE (sized for the PowerPC code) replaces the template's.
echo "Making the fat application Solitaire"
mkdir -p "$OUT/rez"
"$RETRO68/bin/Rez" \
    "$RETRO68/powerpc-apple-macos/RIncludes/RetroPPCAPPL.r" \
    -I"$RETRO68/powerpc-apple-macos/RIncludes" \
    -DCFRAG_NAME="\"Solitaire\"" \
    --copy "$OUT/m68k/wxSolitaire.code.bin" \
    --data "$OUT/ppc/wxSolitaire.pef" \
    -t APPL -c WXSL \
    "$OUT/ppc/wxSolitaire.r.rsrc.bin" \
    -o "$OUT/rez/Solitaire.bin"

cmake -DINPUT="$OUT/rez/Solitaire.bin" \
      -DNAME=Solitaire \
      -DOUT_BIN="$OUT/Solitaire.bin" \
      -DOUT_DSK="$OUT/Solitaire.dsk" \
      -DSET_FLAGS="$OUT/ppc/macbinary-set-flags" \
      -DHFSUTILS="$RETRO68/bin" \
      -DWORK_DIR="$OUT" \
      -P "$SRC/cmake/Retro68FinishApplication.cmake"

echo
echo "Done:"
echo "  $OUT/Solitaire.dsk  (HFS disk image, fat 68k + PowerPC application)"
echo "  $OUT/Solitaire.bin  (MacBinary)"
