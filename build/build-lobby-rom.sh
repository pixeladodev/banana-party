#!/bin/sh
# Build the room ROM (emu/lobby.nds) from the SuperDrastic stress ROM sources at the commit pinned in
# app/manifest (lobby_rom_source) and check it against lobby_rom_sha. The build is reproducible: the same
# commit and toolchain give the same bytes. Needs curl, tar, python3, clang, ld.lld and llvm-objcopy.
#   build/build-lobby-rom.sh <output.nds>
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
OUT=${1:-}
[ -n "$OUT" ] || { echo "build-lobby-rom: need an output path" >&2; exit 2; }
manifest() { sed -n "s/^$1=//p" "$ROOT/app/manifest" | head -n 1 | tr -d '\r'; }
src=$(manifest lobby_rom_source)
want=$(manifest lobby_rom_sha)
[ -n "$src" ] && [ -n "$want" ] || { echo "build-lobby-rom: manifest needs lobby_rom_source, lobby_rom_sha" >&2; exit 1; }
sha_file() {
    if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | awk '{print $1}'; else shasum -a 256 "$1" | awk '{print $1}'; fi
}
for t in curl tar python3 clang ld.lld llvm-objcopy; do
    command -v "$t" >/dev/null 2>&1 || { echo "build-lobby-rom: missing $t" >&2; exit 1; }
done
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
curl -fsSL -o "$tmp/sd.tar.gz" "https://github.com/pixeladodev/SuperDrastic/archive/$src.tar.gz"
tar -xzf "$tmp/sd.tar.gz" -C "$tmp" "SuperDrastic-$src/stressrom"
python3 "$tmp/SuperDrastic-$src/stressrom/build.py" >/dev/null
rom="$tmp/SuperDrastic-$src/stressrom/out/dsstress-L1.nds"
[ -s "$rom" ] || { echo "build-lobby-rom: the stress ROM build gave no dsstress-L1.nds" >&2; exit 1; }
got=$(sha_file "$rom")
[ "$got" = "$want" ] || { echo "build-lobby-rom: built $got, manifest wants $want" >&2; exit 1; }
mkdir -p "$(dirname "$OUT")"
cp "$rom" "$OUT"
chmod 644 "$OUT"
echo "build-lobby-rom: $OUT ($got)"
