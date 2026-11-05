#!/usr/bin/env bash
# Checks that the private session of tools/isolated.sh sees nothing of the person's own desktop or accounts:
#
#   tools/isolated.sh tools/check-isolated-private.sh
#
# Every process of the session (those on its D-Bus session bus, and the bus itself) has a private HOME, XDG folders and runtime folder, no
# socket variable of the real desktop, and no file open or mapped from the real home outside this project; the account services, asked
# on the session's bus, hold no account. It prints process names and counts only, never the name or a detail of an account.
# It cannot see where a process's already open sockets lead; a private XDG_RUNTIME_DIR is what keeps the real ones from being found.
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
failures=0
check () {                      # check DESCRIPTION CONDITION-EXIT-STATUS
  if [ "$2" -eq 0 ]; then echo "  ok   $1"; else echo "  FAIL $1"; failures=$((failures + 1)); fi
}

[ "${EASEL_ISOLATED:-}" = 1 ] || { echo "run this through tools/isolated.sh" >&2; exit 2; }
REAL_HOME=$(getent passwd "$(id -u)" | cut -d: -f6)
REAL_RUNTIME=/run/user/$(id -u)
BUS=$DBUS_SESSION_BUS_ADDRESS

# Asking wakes the services: the checks below then see them running. Only counts leave these calls.
managed () {                    # managed BUS-NAME OBJECT-PATH: the reply of GetManagedObjects, empty if there is none
  timeout 30 gdbus call --session --dest "$1" --object-path "$2" --method org.freedesktop.DBus.ObjectManager.GetManagedObjects 2>/dev/null
}
ACCOUNTS=$(managed org.gnome.OnlineAccounts /org/gnome/OnlineAccounts)
SOURCES=$(managed org.gnome.evolution.dataserver.Sources5 /org/gnome/evolution/dataserver/SourceManager)
[ -n "$ACCOUNTS" ]; check "the online accounts service answers" $?
[ -n "$SOURCES" ]; check "the calendar and contacts source service answers" $?
[ "$(printf '%s' "$ACCOUNTS" | grep -o "/org/gnome/OnlineAccounts/Accounts/" | wc -l)" -eq 0 ]; check "the online accounts service holds no account" $?
# The sources that come with the service have names for ids (system-calendar); one made from a real account has a 40-digit hash.
[ "$(printf '%s' "$SOURCES" | grep -oE "'UID': <'[0-9a-f]{40}'>" | wc -l)" -eq 0 ]; check "the source service holds no source of a real account" $?

# The session's processes: on its bus, or the bus itself (a child of the dbus-run-session above this command).
pids=()
for p in /proc/[0-9]*; do
  [ -r "$p/environ" ] || continue
  if tr '\0' '\n' < "$p/environ" 2>/dev/null | grep -qxE "(DBUS_SESSION_BUS_ADDRESS|DBUS_STARTER_ADDRESS)=$BUS"; then pids+=("${p#/proc/}"); fi
done
run_session=$(ps -o ppid= -p "$(ps -o ppid= -p $$)" | tr -d ' ')
for up in "$PPID" "$run_session"; do
  [ "$(ps -o comm= -p "$up" 2>/dev/null)" = dbus-run-session ] && pids+=($(ps -o pid= --ppid "$up" -C dbus-daemon 2>/dev/null))
done
for bus in $(pgrep -x dbus-daemon); do
  [ "$(ps -o comm= -p "$(ps -o ppid= -p "$bus" | tr -d ' ')" 2>/dev/null)" = dbus-run-session ] || continue
  tr '\0' '\n' < "/proc/$bus/cmdline" 2>/dev/null | grep -q -- --print-address && [ "$(ps -o ppid= -p "$(ps -o ppid= -p "$bus" | tr -d ' ')" | tr -d ' ')" -gt 0 ] && pids+=("$bus")
done
pids=($(printf '%s\n' "${pids[@]}" | sort -un))
[ "${#pids[@]}" -gt 5 ]; check "the session has its services (${#pids[@]} processes looked at)" $?

bad_env=0; bad_files=0
for pid in "${pids[@]}"; do
  [ "$pid" = "$$" ] && continue
  env_text=$(tr '\0' '\n' < "/proc/$pid/environ" 2>/dev/null) || continue
  comm=$(cat "/proc/$pid/comm" 2>/dev/null)
  for var in HOME XDG_CONFIG_HOME XDG_DATA_HOME XDG_CACHE_HOME XDG_STATE_HOME XDG_RUNTIME_DIR; do
    value=$(printf '%s\n' "$env_text" | sed -n "s/^$var=//p" | head -1)
    case "$value" in
      /tmp/easel-isolated-home-*) ;;
      *) echo "       $comm: $var is not private"; bad_env=$((bad_env + 1)) ;;
    esac
  done
  for var in GNOME_KEYRING_CONTROL SSH_AUTH_SOCK DISPLAY XAUTHORITY SESSION_MANAGER; do
    if printf '%s\n' "$env_text" | grep -q "^$var="; then echo "       $comm: $var comes from the real desktop"; bad_env=$((bad_env + 1)); fi
  done
  files=$( { ls -l "/proc/$pid/fd" 2>/dev/null | sed -n 's/.* -> //p'; awk '{ print $6 }' "/proc/$pid/maps" 2>/dev/null; } |
           grep -E "^($REAL_HOME|$REAL_RUNTIME)(/|\$)" | grep -vE "^$ROOT(/|\$)" | sort -u | wc -l)
  if [ "$files" -gt 0 ]; then echo "       $comm: $files files of the real home or runtime folder"; bad_files=$((bad_files + 1)); fi
done
[ "$bad_env" -eq 0 ]; check "no process has a real folder or a socket variable of the real desktop ($bad_env found)" $?
[ "$bad_files" -eq 0 ]; check "no process has a file of the real home or runtime folder open or mapped ($bad_files processes)" $?

if [ "$failures" -eq 0 ]; then echo "privacy check: all ok"; else echo "privacy check: $failures failed"; exit 1; fi
