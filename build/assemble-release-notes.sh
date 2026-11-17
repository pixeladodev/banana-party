#!/bin/bash
# Print the release body: release-notes/<ver>.md with {emulator_version} filled from app/manifest.
# Only the notes go to stdout; the check's messages go to stderr.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
"$ROOT/build/check-release-notes.sh" >&2
ver=$(sed -n 's/^app_version=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
zip="$ROOT/dist/banana-party-${ver}.zip"
[ -f "$zip" ] || { echo "assemble-release-notes: missing $zip (run package.sh first)" >&2; exit 1; }
emu=$(sed -n 's/^emulator_version=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
[ -n "$emu" ] || { echo "assemble-release-notes: app/manifest has no emulator_version" >&2; exit 1; }
sed "s/{emulator_version}/$emu/g" "$ROOT/release-notes/${ver}.md"
