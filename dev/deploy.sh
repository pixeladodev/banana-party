#!/bin/bash
# Copy Banana Party onto the consoles, over the installed path. Does not commit.
#   CONSOLES="ip ip" ./dev/deploy.sh
#   ./dev/deploy.sh                          # the consoles from dev/consoles.local
#   ./dev/deploy.sh -b /path/to/bananads      # the emulator to ship (default: the local draft
#                                             # in ../bananads-lan/dist/bananads, if it is there)
#   ./dev/deploy.sh -V 0.2.2                   # the version the installed app will say (so the
#                                             # updater does not wipe the test build)
# The app carries its own emulator (emu/bananads): /usr/bin/bananads, the stock BananaDS of
# BananaOS, is never touched and never mounted over (app/common.sh: the app plays DS games on
# its own BananaDS). After installing, close and open Banana Party: the room process execs the
# binary when it starts.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BIN=
VER=
while getopts b:V:h o; do
    case $o in
        b) BIN=$OPTARG ;;
        V) VER=$OPTARG ;;
        h) sed -n '2,14s/^# \{0,1\}//p' "$0"; exit 0 ;;
        *) echo "unknown option: $o" >&2; sed -n '2,14s/^# \{0,1\}//p' "$0" >&2; exit 2 ;;
    esac
done
if [ -z "${CONSOLES:-}" ] && [ -f "$ROOT/dev/consoles.local" ]; then
    CONSOLES=$(grep -v '^[[:space:]]*#' "$ROOT/dev/consoles.local" | tr '\n' ' ')
fi
[ -n "${CONSOLES:-}" ] || { echo "deploy: set CONSOLES or dev/consoles.local" >&2; exit 2; }
KEY=${BANANADS_LAN_KEY:-$HOME/.ssh/bananaos_rgds}

# package.sh builds the on-screen app and attaches the player: the local draft when there is
# one, otherwise the published release named in app/manifest.
if [ -n "$BIN" ]; then
    BANANA_PARTY_EMU_BIN=$BIN "$ROOT/build/package.sh" >/dev/null
else
    "$ROOT/build/package.sh" >/dev/null
fi
ver=$(sed -n 's/^app_version=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
ZIP=$ROOT/dist/banana-party-$ver.zip
[ -f "$ZIP" ] || { echo "deploy: no $ZIP" >&2; exit 1; }
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
unzip -q -d "$STAGE" "$ZIP"

# the version the installed app will say: only in the staged copy, the repo's
# manifest is not touched. A test build with the published version would be
# overwritten by the updater as soon as a newer release comes out.
if [ -n "$VER" ]; then
    sed -i.bak "s/^app_version=.*/app_version=$VER/" "$STAGE/banana-party/manifest" && rm -f "$STAGE/banana-party/manifest.bak"
    echo "version installed: $VER"
fi

# what the consoles must end up with
LOCAL_EMU=$(sha256sum "$STAGE/banana-party/emu/bananads" | awk '{print $1}')
LOCAL_UI=$(sha256sum "$STAGE/banana-party/ui/banana-party" | awk '{print $1}')
echo "zip:   $ZIP"
echo "emu:   $LOCAL_EMU  (local)"
echo "ui:    $LOCAL_UI  (local)"

one() {
    local ip=$1
    COPYFILE_DISABLE=1 tar -C "$STAGE" -cf - banana-party "Banana Party.sh" | ssh -i "$KEY" -o IdentitiesOnly=yes -o ConnectTimeout=8 -o BatchMode=yes "root@$ip" '
        set -eu
        d=$(mktemp -d)
        tar -C "$d" -xf -
        mkdir -p /data/store/apps/banana-party.new /data/store/tiles
        tar -C "$d/banana-party" -cf - . | tar -C /data/store/apps/banana-party.new -xf -
        rm -rf /data/store/apps/banana-party
        mv /data/store/apps/banana-party.new /data/store/apps/banana-party
        cp "$d/Banana Party.sh" "/data/store/tiles/Banana Party.sh"
        chmod 755 /data/store/apps/banana-party/emu/bananads /data/store/apps/banana-party/ui/banana-party
        cd /; rm -rf "$d"; sync
    '
    ssh -i "$KEY" -o IdentitiesOnly=yes -o ConnectTimeout=8 -o BatchMode=yes "root@$ip" '
        set -eu
        A=/data/store/apps/banana-party
        sha256sum $A/emu/bananads $A/ui/banana-party
        echo -n "stock /usr/bin/bananads: "; sha256sum /usr/bin/bananads
        echo -n "montado sobre el stock: "; awk "\$5 == \"/usr/bin/bananads\" { print \"SI\" ; found=1 } END { if (!found) print \"no\" }" /proc/self/mountinfo
        $A/status.sh
    '
}
pids=
for ip in $CONSOLES; do
    one "$ip" > "$STAGE/out-$ip.txt" 2>&1 &
    pids="$pids $!"
done
st=0
for p in $pids; do wait "$p" || st=1; done
for ip in $CONSOLES; do
    echo "==== $ip ===="
    cat "$STAGE/out-$ip.txt"
done
echo
echo "Now: close and open Banana Party on the consoles (the room process execs the binary)."
exit $st
