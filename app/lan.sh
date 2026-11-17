#!/bin/sh
# Local multiplayer on or off. Auto is not a saved mode: a game with no room uses it only
# while that game is open. on does not write auto.
#   lan.sh on|off
set -eu
APP=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
. "$APP/common.sh"
case ${1:-} in
    on) exit 0 ;;
    off) mp_lan_set off ;;
    *) echo "usage: lan.sh on|off" >&2; exit 2 ;;
esac
