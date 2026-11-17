#!/bin/sh
# Player binary and launch scripts for the package.
# When ../bananads-lan/dist/bananads is present, that local draft is what ships.
# Otherwise download the published release named in app/manifest and check binary_sha.
# emu/bananads and emu/lobby.nds stay gitignored. The room ROM is copied when this machine
# already has the pinned one (lobby_rom_sha), otherwise build/build-lobby-rom.sh builds it.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
APP=$1
[ -n "$APP" ] && [ -d "$APP" ] || { echo "fetch-emulator: need app dir" >&2; exit 2; }
LAN=${BANANA_PARTY_LAN:-$ROOT/../bananads-lan}
mkdir -p "$APP/emu"

sha_file() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        shasum -a 256 "$1" | awk '{print $1}'
    fi
}

copy_scripts() {
    src=$1
    for f in run-emu lan.sh status.sh; do
        [ -s "$src/$f" ] || { echo "fetch-emulator: missing $src/$f" >&2; exit 1; }
        cp "$src/$f" "$APP/$f"
        chmod 755 "$APP/$f"
    done
}

copy_rom() {
    want_rom=$(sed -n 's/^lobby_rom_sha=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
    rom=${BANANA_PARTY_LOBBY_NDS:-}
    if [ ! -s "$rom" ]; then
        if [ -s "$LAN/dist/lobby.nds" ]; then
            rom=$LAN/dist/lobby.nds
        elif [ -s "$ROOT/app/emu/lobby.nds" ]; then
            rom=$ROOT/app/emu/lobby.nds
        else
            rom=
        fi
    fi
    if [ -n "$rom" ] && [ "$(sha_file "$rom")" = "$want_rom" ]; then
        cp "$rom" "$APP/emu/lobby.nds"
        chmod 644 "$APP/emu/lobby.nds"
    else
        [ -z "$rom" ] || echo "fetch-emulator: $rom is not the pinned room ROM, building it" >&2
        sh "$ROOT/build/build-lobby-rom.sh" "$APP/emu/lobby.nds"
    fi
}

draft=${BANANA_PARTY_EMU_BIN:-$LAN/dist/bananads}
if [ -s "$draft" ]; then
    cp "$draft" "$APP/emu/bananads"
    chmod 755 "$APP/emu/bananads"
    copy_scripts "$LAN/app"
    copy_rom
    exit 0
fi

ver=$(sed -n 's/^emulator_version=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
pkg=$(sed -n 's/^emulator_package=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
want=$(sed -n 's/^binary_sha=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
[ -n "$ver" ] && [ -n "$pkg" ] && [ -n "$want" ] || {
    echo "fetch-emulator: manifest needs emulator_version, emulator_package, binary_sha" >&2
    exit 1
}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
url="https://github.com/pixeladodev/${pkg}/releases/download/v${ver}/${pkg}-${ver}.zip"
curl -fsSL -o "$tmp/emu.zip" "$url"
for f in emu/bananads run-emu lan.sh status.sh; do
    unzip -p "$tmp/emu.zip" "${pkg}/$f" > "$APP/$f" || {
        echo "fetch-emulator: missing $f in $url" >&2
        exit 1
    }
done
chmod 755 "$APP/emu/bananads" "$APP/run-emu" "$APP/lan.sh" "$APP/status.sh"
got=$(sha_file "$APP/emu/bananads")
[ "$got" = "$want" ] || { echo "fetch-emulator: binary sha $got, manifest wants $want" >&2; exit 1; }
copy_rom
