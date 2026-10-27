#!/bin/sh
# Used by electron-vite on macOS in place of the Electron binary (see electron.vite.config.ts).
# Launches Electron.app through LaunchServices so macOS asks for Bluetooth on behalf of
# Electron.app rather than the terminal, which would otherwise get the app killed.
here="$(cd "$(dirname "$0")/.." && pwd)"
bundle="$here/node_modules/electron/dist/Electron.app"

# electron-vite passes "." for the app folder; open starts Electron in /, so make it absolute.
args=""
for a in "$@"; do
  [ "$a" = "." ] && a="$PWD"
  args="$args $(printf '%s' "$a" | sed "s/'/'\\\\''/g; s/^/'/; s/\$/'/")"
done

# The dev server URL normally arrives in the environment, which open doesn't pass on.
[ -n "$ELECTRON_RENDERER_URL" ] && args="$args '--renderer-url=$ELECTRON_RENDERER_URL'"

# Send the app's output back to this terminal.
io=""
t="$(tty 2>/dev/null)" && case "$t" in /dev/*) io="--stdout $t --stderr $t" ;; esac

eval "exec open -W -n $io \"\$bundle\" --args $args"
