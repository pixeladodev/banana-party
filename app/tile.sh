#!/bin/sh
# Apps shelf entry. The menu has already let go of both panels.
# sh, not a direct exec: a copy over SMB can drop the executable bit on scripts.
APP=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
. "$APP/common.sh"
mp_clean_junk
chmod a+x "$APP/ui/banana-party" "$APP/lan.sh" "$APP/status.sh" "$APP/room.sh" "$APP/run-emu" "$APP/emu/bananads" \
    "$APP/uninstall.sh" "$APP/update.py" "$APP/post-install.sh" "$APP/pre-remove.sh" 2>/dev/null || true
# a copy put in by hand over an older version (no post_install ran): what the old one left comes off here
mp_legacy_cleanup >/dev/null 2>&1 || true
# the shelf's picture for this tile: banana-ui looks for store/tiles/media/box/<tile name>.png
BOX=/data/store/tiles/media/box
if [ -f "$APP/tile.png" ] && ! cmp -s "$APP/tile.png" "$BOX/Banana Party.png" 2>/dev/null; then
    mkdir -p "$BOX" 2>/dev/null && cp "$APP/tile.png" "$BOX/Banana Party.png" 2>/dev/null || true
fi
# banana-ui reads /run/banana/ra-progress when this app exits and stores it on the Apps tile.
# A DS game launched from here writes that file for itself. It is not this tile's count, and it is
# not copied onto another game: drop it, and do not let the game see the variable.
drop_ra_progress() {
    rm -f /run/banana/ra-progress /run/banana/ra-progress.new 2>/dev/null || true
}
drop_ra_progress
# A game picked in the app's list: the UI writes its path to $PLAY and exits with 76; the game runs on the app's own
# BananaDS (run-emu) and the list opens again when it ends. For testing over SSH, a path written to
# /run/banana/bananads-lan-launch is played at once (once), as banana-ui's /run/banana/launch does for the menu.
PLAY=/tmp/bananads-lan.play
rm -f /tmp/bananads-lan-game.pid    # (a copy left by a session that ended without putting it back)
TEST=/run/banana/bananads-lan-launch
# the app's own entry in game.pid, put back after a game (run-emu points it at the game while it runs)
restore_pid() {
    [ -f /tmp/bananads-lan-game.pid ] && mv -f /tmp/bananads-lan-game.pid /run/banana/game.pid 2>/dev/null
    return 0
}
# a game's time in the app's own playstats.tsv, the menu's format (nds/<rom> TAB plays TAB seconds TAB last): the menu
# rewrites its own file from memory when the app closes, so a line put there now would be lost; the app's library adds
# both up. As the menu counts: a start under 5 seconds isn't a play
STATS_DIR=/data/bananads-lan
record_play() {
    secs=$(( $(date +%s) - $2 ))
    [ "$secs" -ge 5 ] || return 0
    mkdir -p "$STATS_DIR" 2>/dev/null || return 0
    rom="$1"
    case "$rom" in
        /run/banana/sd2/roms/nds/*) key="nds/@sd2/${rom#/run/banana/sd2/roms/nds/}" ;;
        /data/roms/nds/*) key="nds/${rom#/data/roms/nds/}" ;;
        *) key="nds/$(basename "$rom")" ;;
    esac
    now=$(date +%s)
    [ -f "$STATS_DIR/playstats.tsv" ] || : > "$STATS_DIR/playstats.tsv"
    for stats in "$STATS_DIR/playstats.tsv" /data/banana/playstats.tsv; do
        [ -f "$stats" ] || : > "$stats" 2>/dev/null || continue
        awk -F '\t' -v OFS='\t' -v k="$key" -v s="$secs" -v t="$now" '
            $1 == k { $2 += 1; $3 += s; $4 = t; found = 1 } { print }
            END { if (!found) print k, 1, s, t }' "$stats" > "${stats}.new" 2>/dev/null \
            && mv -f "${stats}.new" "$stats"
        rm -f "${stats}.new"
    done
    sync 2>/dev/null
    return 0
}
play() {
    rm -f "$PLAY"
    started=$(date +%s)
    drop_ra_progress
    (
        unset BANANA_RA_PROGRESS
        sh "$APP/room.sh" play "$1"
    )
    rc=$?
    drop_ra_progress
    [ "$1" = @firmware ] || record_play "$1" "$started"
    restore_pid
    # the app's clocks again (systems.conf's perf for [storeapps], as banana-ui set them when the app started)
    command -v banana-perf >/dev/null 2>&1 && banana-perf balanced 2>/dev/null
    [ "$rc" = 0 ] || echo "run-emu: exit $rc" >&2
}
if [ -f "$TEST" ]; then
    rom=$(head -n 1 "$TEST"); rm -f "$TEST"
    play "$rom"
    sh "$APP/room.sh" quit
    exit 0
fi
sh "$APP/room.sh" boot
ui_arg=
while :; do
    rm -f "$PLAY"
    "$APP/ui/banana-party" $ui_arg
    rc=$?
    # 76: a game picked in the list; when it ends the list opens again on it (--games)
    if [ "$rc" = 76 ] && [ -f "$PLAY" ]; then
        play "$(head -n 1 "$PLAY")"
        ui_arg=--games
        continue
    fi
    break
done
sh "$APP/room.sh" quit
[ -x "$APP/ui/banana-party" ] || sync 2>/dev/null || true
drop_ra_progress
# 75: the app has just updated itself (its folder is the new version now): open the new one
if [ "$rc" = 75 ] && [ -f "$APP/tile.sh" ]; then
    exec sh "$APP/tile.sh"
fi
exit "$rc"
