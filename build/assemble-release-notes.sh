#!/bin/bash
# Print release body: release-notes/<ver>.md plus checksum from the shipped zip.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
"$ROOT/build/check-release-notes.sh" >&2
ver=$(sed -n 's/^app_version=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
zip="$ROOT/dist/banana-party-${ver}.zip"
sums="$ROOT/dist/SHA256SUMS"
[ -f "$zip" ] || { echo "assemble-release-notes: missing $zip (run package.sh first)" >&2; exit 1; }
[ -f "$sums" ] || { echo "assemble-release-notes: missing $sums" >&2; exit 1; }
zip_sha=$(awk '{print $1; exit}' "$sums")
emu=$(sed -n 's/^emulator_version=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
[ -n "$emu" ] || { echo "assemble-release-notes: app/manifest has no emulator_version" >&2; exit 1; }
# {emulator_version} in the notes is the bananads-lan release pinned in app/manifest
sed "s/{emulator_version}/$emu/g" "$ROOT/release-notes/${ver}.md"
# Blank line before --- so GitHub does not treat the last paragraph as a setext heading.
echo
echo
echo '---'
echo
echo '## Checksums'
echo
printf 'SHA256 of `banana-party-%s.zip`: `%s`\n\n' "$ver" "$zip_sha"
echo 'Source: this repository (`LICENSE`). The shipped player is the local draft when that binary is present, otherwise the published release named in `app/manifest`. `emu/lobby.nds` is built from the stress ROM sources at the commit pinned in `app/manifest` and checked against `lobby_rom_sha`. Neither file is stored in git.'