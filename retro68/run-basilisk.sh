#!/bin/sh
#
# Runs a trace build of Solitaire (cmake -DSOLITAIRE_TRACE=ON) unattended in
# Basilisk II and prints its startup log:
#
#   retro68/run-basilisk.sh <Solitaire.bin> [seconds] [workdir]
#
# A copy of the boot disk ($BOOT_DISK, default ~/MacROM/HD.img; the original
# is never touched) gets the application in "System Folder:Startup Items"
# and a "wxtest.shutdown" marker in Preferences, so the trace build shuts the
# Mac down once its window has been up for 25 seconds. Basilisk II runs with
# a prefs file of its own; screenshots go to the work directory every few
# seconds. Afterwards "wxtest.log" is read back from the disk and printed.

set -e

APP=$1
TIMEOUT=${2:-180}
WORK=${3:-"${TMPDIR:-/tmp}/solitaire-basilisk"}
BOOT_DISK=${BOOT_DISK:-$HOME/MacROM/HD.img}
ROM=${ROM:-"$HOME/MacROM/3DC27823 - Quadra 950.ROM"}
BASILISK=${BASILISK:-/Applications/BasiliskII.app/Contents/MacOS/BasiliskII}
HFS=${HFS:-/opt/retro68/bin}
SCREEN=${SCREEN:-win/640/480}
RAMSIZE=${RAMSIZE:-67108864}
FPU=${FPU:-false}
DEPTH=${DEPTH:-8}           # displaycolordepth: 1, 2, 4, 8, 15/16 or 24/32 bits

[ -f "$APP" ] || { echo "usage: $0 <Solitaire.bin> [seconds] [workdir]" >&2; exit 2; }
APP=$(cd "$(dirname "$APP")" && pwd)/$(basename "$APP")
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

mkdir -p "$WORK"
WORK=$(cd "$WORK" && pwd)
rm -f "$WORK"/shot-*.png "$WORK/wxtest.log"

# An APFS clone when possible: fast and takes no space.
cp -c "$BOOT_DISK" "$WORK/HD.img" 2>/dev/null || cp "$BOOT_DISK" "$WORK/HD.img"

# hfsutils keep their state in $HOME/.hcwd: use a private one.
export HOME="$WORK"
"$HFS/hmount" "$WORK/HD.img" >/dev/null
STARTUP=":System Folder:Startup Items"
"$HFS/hls" "$STARTUP" >/dev/null 2>&1 || "$HFS/hmkdir" "$STARTUP"
"$HFS/hcopy" -m "$APP" "$STARTUP:"
# In Preferences, not next to the application: the Finder opens every file
# in Startup Items.
touch "$WORK/wxtest.shutdown"
"$HFS/hcopy" -r "$WORK/wxtest.shutdown" ":System Folder:Preferences:"
"$HFS/humount" >/dev/null

cat > "$WORK/prefs" <<EOF
disk $WORK/HD.img
screen $SCREEN
displaycolordepth $DEPTH
rom $ROM
bootdrive 0
bootdriver 0
ramsize $RAMSIZE
frameskip 1
modelid 14
cpu 4
fpu $FPU
nocdrom true
nosound true
nogui true
jit false
ignoresegv true
idlewait true
ether none
EOF

if [ ! -x "$WORK/winid" ] || [ "$SCRIPT_DIR/winid.c" -nt "$WORK/winid" ]; then
    clang -O "$SCRIPT_DIR/winid.c" -o "$WORK/winid" \
        -framework CoreGraphics -framework CoreFoundation
fi
SHOT_EVERY=${SHOT_EVERY:-3}

"$BASILISK" --config "$WORK/prefs" >"$WORK/basilisk.log" 2>&1 &
PID=$!

n=0
while kill -0 $PID 2>/dev/null && [ $n -lt "$TIMEOUT" ]; do
    sleep 1
    n=$((n + 1))
    if [ $((n % SHOT_EVERY)) -eq 0 ]; then
        WID=$("$WORK/winid" $PID || true)
        if [ -n "$WID" ]; then
            screencapture -x -o -l"$WID" "$WORK/shot-$(printf %03d $n).png" \
                2>/dev/null || true
        fi
    fi
done

if kill -0 $PID 2>/dev/null; then
    echo "*** Timeout after $TIMEOUT seconds, killing Basilisk II"
    kill $PID
    sleep 1
    kill -9 $PID 2>/dev/null || true
fi

"$HFS/hmount" "$WORK/HD.img" >/dev/null
if "$HFS/hcopy" -r "$STARTUP:wxtest.log" "$WORK/wxtest.log" 2>/dev/null; then
    tr '\r' '\n' < "$WORK/wxtest.log"
else
    echo "*** No wxtest.log found"
fi
"$HFS/humount" >/dev/null
