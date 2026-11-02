#!/usr/bin/env bash
# Runs a command in a private session, so that a real pointer, the windows of the person at the keyboard and their desktop
# services cannot touch the ui tests: a headless GNOME Shell with one virtual monitor and its own Wayland socket, in a private
# D-Bus session, with a throwaway HOME and GSETTINGS_BACKEND=memory. EASEL_ISOLATED=1 is set for the command, which makes the
# ui tests and tools/mutate.sh skip their "another Easel window" check (nothing else can be in this display).
#
#   tools/isolated.sh meson test -C build --print-errorlogs      all the tests, the ui ones in the isolated display
#   tools/isolated.sh tools/mutate.sh                            a full mutation run
#   tools/isolated.sh --scale 2 meson test -C build ui-0025-scale2   on a screen of scale 2: a virtual monitor of 3840 x 2400 (1920 x 1200 logical)
#
# Not for the benchmarks (perf-*): they stay in the real session, with no Easel window open (docs/engineering.md, Testing and
# Performance budgets); this script refuses them.
#
# Exit status: the command's; 130, 143 or 129 if interrupted (INT, TERM, HUP); 90 if the session did not start; 69 if a tool is
# missing; 2 for a wrong use. Everything it started is stopped, and the throwaway HOME removed, on every way out.
# Hooks for its own test: EASEL_ISOLATED_SHELL and EASEL_ISOLATED_DBUS name the two programs it needs (gnome-shell and
# dbus-run-session by default).
set -u

SHELL_PROGRAM=${EASEL_ISOLATED_SHELL:-gnome-shell}
DBUS_PROGRAM=${EASEL_ISOLATED_DBUS:-dbus-run-session}

if [ "${1:-}" != "--inner" ]; then
  if [ "${1:-}" = --scale ]; then
    if [ "${2:-}" != 2 ]; then
      echo "tools/isolated.sh: --scale takes 2 (no other scale has been tried)" >&2
      exit 2
    fi
    export EASEL_ISOLATED_SCALE=2
    shift 2
  fi
  if [ $# -eq 0 ]; then
    echo "usage: tools/isolated.sh [--scale 2] COMMAND [ARGUMENT]..." >&2
    exit 2
  fi
  for arg in "$@"; do
    case "$arg" in
      *perf-[0-9]*|--benchmark)
        echo "tools/isolated.sh: benchmarks stay in the real session with no Easel window open: the isolated session has a virtual" >&2
        echo "monitor with its own frame clock and the ui tests there use the Cairo renderer, so its timings are not comparable" >&2
        echo "(docs/engineering.md, Performance budgets). Run them without this script." >&2
        exit 2 ;;
    esac
  done
  if [ "${EASEL_ISOLATED:-}" = 1 ]; then          # already in the private session: nothing to start
    if [ -n "${EASEL_ISOLATED_SCALE:-}" ]; then
      echo "tools/isolated.sh: --scale needs a session of its own; this one is already running" >&2
      exit 2
    fi
    exec "$@"
  fi
  for program in "$SHELL_PROGRAM" "$DBUS_PROGRAM"; do
    if ! command -v "$program" >/dev/null 2>&1; then
      echo "tools/isolated.sh needs gnome-shell (with --headless) and dbus-run-session; '$program' was not found in PATH." >&2
      exit 69
    fi
  done
  if [ -z "${XDG_RUNTIME_DIR:-}" ] || [ ! -d "$XDG_RUNTIME_DIR" ]; then
    echo "tools/isolated.sh needs XDG_RUNTIME_DIR (the folder for the Wayland socket); it is not set." >&2
    exit 69
  fi
  exec "$DBUS_PROGRAM" -- "$0" --inner "$@"
fi
shift

ROOT=$(cd "$(dirname "$0")/.." && pwd)
SOCKET=easel-isolated-$$
HOME_DIR=$(mktemp -d /tmp/easel-isolated-home-XXXXXX) || exit 90
LOG_DIR=$ROOT/.scratch/isolated
LOG=$LOG_DIR/shell.log
mkdir -p "$LOG_DIR"
SHELL_PID=
COMMAND_PID=
WATCHDOG_PID=
SIGNALLED=
INNER_PID=$$

cleanup () {
  trap - EXIT INT TERM HUP
  [ -n "$WATCHDOG_PID" ] && kill "$WATCHDOG_PID" 2>/dev/null
  [ -n "$COMMAND_PID" ] && kill -TERM "$COMMAND_PID" 2>/dev/null
  if [ -n "$SHELL_PID" ]; then
    kill "$SHELL_PID" 2>/dev/null
    wait "$SHELL_PID" 2>/dev/null
  fi
  case "$HOME_DIR" in
    /tmp/easel-isolated-home-*) rm -rf "$HOME_DIR" ;;
  esac
  # A shell killed by the signal that stops the whole group leaves its socket and lock file behind.
  [ -n "${XDG_RUNTIME_DIR:-}" ] && rm -f "$XDG_RUNTIME_DIR/$SOCKET" "$XDG_RUNTIME_DIR/$SOCKET.lock"
}
trap cleanup EXIT
trap 'SIGNALLED=130; [ -n "$COMMAND_PID" ] && kill -TERM "$COMMAND_PID" 2>/dev/null' INT
trap 'SIGNALLED=143; [ -n "$COMMAND_PID" ] && kill -TERM "$COMMAND_PID" 2>/dev/null' TERM
trap 'SIGNALLED=129; [ -n "$COMMAND_PID" ] && kill -TERM "$COMMAND_PID" 2>/dev/null' HUP

# The environment of the session: a throwaway HOME, settings in memory, a Wayland session of our own.
PRIVATE_ENV=(HOME="$HOME_DIR" XDG_CONFIG_HOME="$HOME_DIR/.config" XDG_DATA_HOME="$HOME_DIR/.local/share"
             XDG_CACHE_HOME="$HOME_DIR/.cache" XDG_STATE_HOME="$HOME_DIR/.local/state" GSETTINGS_BACKEND=memory
             XDG_SESSION_TYPE=wayland)

# If the D-Bus session this runs in is killed (a plain kill of the top process does not reach us), stop too: the watchdog sees
# the parent go and sends us TERM, which stops the command and runs the cleanup.
PARENT_PID=$PPID
( while kill -0 "$PARENT_PID" 2>/dev/null; do sleep 1; done; kill -TERM "$INNER_PID" 2>/dev/null ) &
WATCHDOG_PID=$!

MONITOR=1920x1200
[ "${EASEL_ISOLATED_SCALE:-}" = 2 ] && MONITOR=3840x2400
env "${PRIVATE_ENV[@]}" "$SHELL_PROGRAM" --headless --wayland --no-x11 --virtual-monitor "$MONITOR" \
    --wayland-display="$SOCKET" >"$LOG" 2>&1 &
SHELL_PID=$!
for _ in $(seq 1 50); do
  [ -S "$XDG_RUNTIME_DIR/$SOCKET" ] && break
  kill -0 "$SHELL_PID" 2>/dev/null || break
  sleep 0.2
done
if [ ! -S "$XDG_RUNTIME_DIR/$SOCKET" ]; then
  echo "tools/isolated.sh: the headless GNOME Shell did not come up (it stopped, or made no Wayland socket in 10 s); its log: $LOG" >&2
  exit 90
fi
sleep 2                                             # the shell settles: the monitor, the first frame

# A monitors.xml in the throwaway HOME is not honored (0006's Plan): the scale is asked of the running shell (0025's Plan).
if [ "${EASEL_ISOLATED_SCALE:-}" = 2 ]; then
  STATE=$(gdbus call --session --dest org.gnome.Mutter.DisplayConfig --object-path /org/gnome/Mutter/DisplayConfig \
          --method org.gnome.Mutter.DisplayConfig.GetCurrentState 2>>"$LOG")
  SERIAL=$(echo "$STATE" | grep -o "^(uint32 [0-9]*" | grep -o "[0-9]*$")
  MODE=$(echo "$STATE" | grep -o "'$MONITOR@[0-9.]*'" | head -1 | tr -d "'")
  if [ -z "$SERIAL" ] || [ -z "$MODE" ] \
     || ! gdbus call --session --dest org.gnome.Mutter.DisplayConfig --object-path /org/gnome/Mutter/DisplayConfig \
          --method org.gnome.Mutter.DisplayConfig.ApplyMonitorsConfig "$SERIAL" 1 \
          "[(0, 0, 2.0, uint32 0, true, [('Meta-0', '$MODE', {})])]" "{}" >>"$LOG" 2>&1; then
    echo "tools/isolated.sh: the shell did not take scale 2 (serial '$SERIAL', mode '$MODE'); its log: $LOG" >&2
    exit 90
  fi
  sleep 2                                           # the monitor settles at its new scale
fi

env "${PRIVATE_ENV[@]}" WAYLAND_DISPLAY="$SOCKET" GDK_BACKEND=wayland EASEL_ISOLATED=1 "$@" &
COMMAND_PID=$!
wait "$COMMAND_PID"
status=$?
if [ -n "$SIGNALLED" ]; then                        # a signal ended the wait: the command is stopping, let it
  wait "$COMMAND_PID" 2>/dev/null
  status=$SIGNALLED
fi
COMMAND_PID=
exit "$status"
