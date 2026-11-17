#!/bin/sh
# The one BananaDS process Banana Party keeps. It starts in the background, Off.
#   room.sh boot | quit | host <2-4> | join | off | play <rom>
set -eu
APP=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
. "$APP/common.sh"
LOBBY_DIR=${BANANADS_LOBBY_DIR:-/data/bananads-lan/lobby}
PIDF="$LOBBY_DIR/room.pid"
LOGF="$LOBBY_DIR/room.log"
EMU=$APP/emu/bananads
ROM=$APP/emu/lobby.nds
STATES=/data/states/nds-lan-lobby
LOGS=/data/banana/logs/bananads-lan-lobby
# For a test run only, normally absent: BananaDS settings for the room's emulator, one BANANADS_*=value a line
# (e.g. BANANADS_LOG_TIME=on, BANANADS_LAN_TIMEOUT_MS=25 for the LAN's per-second [diag] lines). Read as the room starts.
ROOM_ENV=${BANANADS_ROOM_ENV:-/data/bananads-lan/room.env}

mkdir -p "$LOBBY_DIR" "$STATES" "$LOGS"

room_running() {
    [ -f "$PIDF" ] || return 1
    pid=$(head -n1 "$PIDF" 2>/dev/null) || return 1
    kill -0 "$pid" 2>/dev/null
}

# Kill the headless process only. Do not write "off": that command ends the LAN session,
# and the other consoles hear it at once.
room_kill() {
    if room_running; then
        pid=$(head -n1 "$PIDF")
        kill "$pid" 2>/dev/null || true
        for _ in 1 2 3 4 5 6 7 8 9 10; do
            kill -0 "$pid" 2>/dev/null || break
            sleep 0.2
        done
        kill -9 "$pid" 2>/dev/null || true
    fi
    rm -f "$PIDF" "$LOBBY_DIR/cmd"
}

room_stop() {
    if room_running; then
        echo off > "$LOBBY_DIR/cmd" 2>/dev/null || true
        sleep 0.3
    fi
    room_kill
}

# One process. It starts headless, in Off, and stays that process when a game is shown.
start_room() {
    mode=$1
    max=$(sed -n 's/^ *wifi\.local_players *= *\([0-9][0-9]*\).*/\1/p' "$MP_BDCONF" 2>/dev/null | tail -n 1)
    [ -n "$max" ] || max=4
    rm -f "$LOBBY_DIR/cmd"
    : > "$LOGF"
    (
        unset BANANADS_WIFI_LOCAL
        if [ -f "$ROOM_ENV" ]; then
            while IFS= read -r line || [ -n "$line" ]; do
                line=$(printf '%s' "$line" | tr -d '\r')
                case $line in
                BANANADS_WIFI_LOCAL=* | BANANADS_DATA=* | BANANADS_LOGDIR=* | BANANADS_FRONTEND=*) ;;
                BANANADS_[A-Z0-9_]*=*) export "$line" ;;
                esac
            done < "$ROOM_ENV"
        fi
        BANANADS_DATA=/data/bananads BANANADS_LOGDIR="$LOGS" \
        BANANADS_FRONTEND=headless BANANADS_TOP=${BANANA_TOP:-DSI-2} \
        exec "$EMU" --frontend headless \
            --set "wifi.local=$mode" \
            --set "wifi.local_players=$max" \
            --set "wifi.lobbyctl_dir=$LOBBY_DIR" \
            --set headless.frames=0 \
            --set "log.file=$LOGF" \
            "$ROM"
    ) &
    echo $! > "$PIDF"
}

status_field() {
    sed -n "s/^$1=//p" "$LOBBY_DIR/status" 2>/dev/null | head -n 1 | tr -d '\r'
}

wait_gone() {
    i=0
    while room_running && [ "$i" -lt 50 ]; do
        sleep 0.1
        i=$((i + 1))
    done
    room_kill
}

case ${1:-} in
boot)
    # Opening the app: no saved room. A process left from last time is closed.
    if room_running; then
        echo stop > "$LOBBY_DIR/cmd"
        wait_gone
    fi
    mp_lan_set off
    rm -f "$LOBBY_DIR/cmd" "$LOBBY_DIR/status"
    start_room off
    # The process is started Off. Off does not write a status line, so waiting for
    # mode=off held the library for the full 5 s every open. Say off again and return.
    echo off > "$LOBBY_DIR/cmd"
    ;;
quit)
    if room_running; then
        echo stop > "$LOBBY_DIR/cmd"
        wait_gone
    fi
    mp_lan_set off
    rm -f "$LOBBY_DIR/status" "$LOBBY_DIR/cmd"
    ;;
host)
    max=${2:-2}
    [ "$max" -ge 2 ] && [ "$max" -le 4 ] || max=4
    room_running || start_room off
    mp_lan_set host
    tmp="$MP_BDCONF.tmp.$$"
    awk -v v="$max" '
        /^ *wifi\.local_players *=/ { if (!done) print "wifi.local_players = " v; done = 1; next }
        { print }
        END { if (!done) print "wifi.local_players = " v }
    ' "$MP_BDCONF" > "$tmp" && mv -f "$tmp" "$MP_BDCONF"
    echo "host $max" > "$LOBBY_DIR/cmd"
    ;;
join)
    room_running || start_room off
    mp_lan_set join
    echo join > "$LOBBY_DIR/cmd"
    ;;
off)
    mp_lan_set off
    if room_running; then echo off > "$LOBBY_DIR/cmd"; fi
    ;;
play)
    rom=${2:-}
    zd=
    # @firmware: the DS menu (DS Download Play) from the user's BIOS and firmware, the cart slot empty
    [ "$rom" = @firmware ] || { [ -n "$rom" ] && [ -f "$rom" ]; } || { echo "room.sh: no game at '$rom'" >&2; exit 2; }
    case $rom in
    *.zip|*.ZIP)
        zd=$(mktemp -d)
        unzip -qq -o "$rom" -d "$zd" 2>/dev/null || true
        found=$(find "$zd" -iname '*.nds' -type f 2>/dev/null | head -n 1)
        [ -n "$found" ] || { echo "room.sh: no game inside $rom" >&2; rm -rf "$zd"; exit 2; }
        rom=$found
        ;;
    esac
    room_running || start_room off
    [ "$rom" = @firmware ] || { [ -x /usr/libexec/banana/nds-saves ] && /usr/libexec/banana/nds-saves "$rom" || true; }
    perf=$(awk '/^\[/{s=$0} s=="[nds]" && /^ *perf *=/{sub(/^ *perf *= */,""); print; exit}' "${BANANA_SYSTEMS:-/run/banana/systems.conf}" 2>/dev/null || true)
    command -v banana-perf >/dev/null 2>&1 && banana-perf "${perf:-superdrastic}" 2>/dev/null || true
    if [ -f /run/banana/game.pid ]; then
        [ -f /tmp/bananads-lan-game.pid ] || cp -f /run/banana/game.pid /tmp/bananads-lan-game.pid
        pid=$(head -n 1 "$PIDF")
        st=$(sed 's/^.*) //' "/proc/$pid/stat" 2>/dev/null | awk '{print $20}')
        echo "$pid $st" > /run/banana/game.pid
    fi
    was=$(status_field played)
    [ -n "$was" ] || was=0
    printf 'play %s\n' "$rom" > "$LOBBY_DIR/cmd"
    seen=0
    i=0
    while room_running; do
        now=$(status_field played)
        [ -n "$now" ] || now=0
        front=$(status_field front)
        [ "$front" = game ] && seen=1
        [ "$now" != "$was" ] && break
        if [ "$seen" = 0 ] && [ "$i" -gt 60 ]; then
            echo "room.sh: the game did not open" >&2
            break
        fi
        sleep 0.25
        i=$((i + 1))
    done
    [ -n "$zd" ] && rm -rf "$zd"
    ;;
*)
    echo "usage: room.sh boot | quit | host <2-4> | join | off | play <rom>" >&2
    exit 2
    ;;
esac