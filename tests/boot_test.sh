#!/usr/bin/env bash
# ============================================================================
#  PyroOS  -  headless boot test
# ----------------------------------------------------------------------------
#  Boots the disk image in QEMU with no display, waits for the shell prompt,
#  then runs one ring-3 ELF program and checks what it prints. The screen is
#  read straight out of VGA text memory through the QEMU monitor, so nothing
#  in the kernel has to change to be testable.
#
#  usage: tests/boot_test.sh [image] [timeout-seconds]
# ============================================================================
set -euo pipefail

image=${1:-build/os-image.bin}
timeout_s=${2:-60}

work=$(mktemp -d)
qpid=
cleanup() {
    [ -n "$qpid" ] && kill "$qpid" 2>/dev/null || true
    rm -rf "$work"
}
trap cleanup EXIT

# The kernel writes PyroFS onto its boot disk, so boot a copy and leave the
# build output untouched.
cp "$image" "$work/disk.img"
mkfifo "$work/monitor"

(cd "$work" && exec qemu-system-i386 -drive format=raw,file=disk.img \
    -display none -no-reboot -monitor stdio <monitor >/dev/null 2>&1) &
qpid=$!
exec 3>"$work/monitor"

# Print the 80x25 text screen. Each cell is a character byte followed by a
# colour byte, so keep every other byte and break the lines every 80 cells.
screen() {
    od -An -v -tu1 -w2 "$work/vga.bin" |
        awk '{ printf "%c", ($1 >= 32 && $1 < 127) ? $1 : 32 }
             NR % 80 == 0 { print "" }'
}

# Dump VGA text memory and wait for it to contain $1. pmemsave gets a relative
# file name: the monitor would parse a leading "/" as division.
wait_for() {
    for _ in $(seq "$timeout_s"); do
        sleep 1
        rm -f "$work/vga.bin"
        echo "pmemsave 0xb8000 4000 vga.bin" >&3
        sleep 0.5
        # grep without -q reads all its input, so the pipeline never ends
        # early with SIGPIPE, which pipefail would count as a failure.
        if [ -s "$work/vga.bin" ] && screen | grep -F -- "$1" >/dev/null; then
            return 0
        fi
    done
    echo "boot test: timed out waiting for: $1" >&2
    [ -s "$work/vga.bin" ] && screen >&2
    return 1
}

# Type a line of lowercase letters, digits, spaces and dots, then Enter.
type_line() {
    local s=$1 c i
    for ((i = 0; i < ${#s}; i++)); do
        c=${s:i:1}
        case "$c" in
            ' ') c=spc ;;
            '.') c=dot ;;
        esac
        echo "sendkey $c" >&3
        sleep 0.1
    done
    echo "sendkey ret" >&3
}

wait_for "pyro>"
echo "boot test: shell prompt is up"

type_line "exec whoami"
wait_for "[whoami] my process id is"
wait_for "program exited cleanly"
echo "boot test: a ring-3 ELF program ran and exited"

echo "quit" >&3
wait "$qpid" || true
qpid=
echo "boot test: passed"
