#!/bin/sh
# Build the on-screen app, attach the player (local draft when present, otherwise the published release), and pack banana-party-*.zip.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ver=$(sed -n 's/^app_version=//p' "$ROOT/app/manifest" | head -n 1 | tr -d '\r')
[ -n "$ver" ] || { echo "app/manifest needs app_version" >&2; exit 1; }
if grep -Eq '^[a-z_]+=(TODO|todo)' "$ROOT/app/manifest"; then
    echo "app/manifest still has placeholders" >&2
    exit 1
fi

STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
APP=$STAGE/banana-party
mkdir -p "$APP/ui" "$ROOT/dist"
sh "$ROOT/build/fetch-emulator.sh" "$APP"

DRM_INC=${DRM_INC:-}
if [ -n "${CC:-}" ]; then
    if [ -z "$DRM_INC" ]; then
        SYS=$("$CC" -print-sysroot 2>/dev/null || true)
        [ -n "$SYS" ] && [ -d "$SYS/usr/include/libdrm" ] && DRM_INC="$SYS/usr/include/libdrm"
    fi
    "$CC" -O2 -Wall -Wextra ${DRM_INC:+-I"$DRM_INC"} -I"$ROOT/app/ui/third_party" -o "$APP/ui/banana-party" \
        "$ROOT/app/ui/main.c" "$ROOT/app/ui/view.c" "$ROOT/app/ui/lobby.c" "$ROOT/app/ui/room_ctl.c" "$ROOT/app/ui/pixel.c" "$ROOT/app/ui/updater.c" "$ROOT/app/ui/stb_impl.c" \
        "$ROOT/app/ui/pixel_sound.c" "$ROOT/app/ui/pixel_image.c" "$ROOT/app/ui/rominfo.c" -ldrm -lm -ldl -lpthread
elif docker image inspect bananads-build:1 >/dev/null 2>&1; then
    docker run --rm --platform linux/amd64 -v "$ROOT:/src:ro" -v "$APP/ui:/out" bananads-build:1 \
        /workspace/bananads-lan-test/aarch64--glibc--stable-2025.08-1/bin/aarch64-buildroot-linux-gnu-gcc \
        -O2 -Wall -Wextra \
        -I/workspace/bananads-lan-test/aarch64--glibc--stable-2025.08-1/aarch64-buildroot-linux-gnu/sysroot/usr/include/libdrm \
        -I/src/app/ui/third_party -o /out/banana-party \
        /src/app/ui/main.c /src/app/ui/view.c /src/app/ui/lobby.c /src/app/ui/room_ctl.c /src/app/ui/pixel.c /src/app/ui/updater.c /src/app/ui/stb_impl.c \
        /src/app/ui/pixel_sound.c /src/app/ui/pixel_image.c /src/app/ui/rominfo.c -ldrm -lm -ldl -lpthread
else
    echo "set CC to an aarch64 gcc with libdrm, or build where bananads-build:1 exists" >&2
    exit 1
fi

cp "$ROOT/app/common.sh" "$ROOT/app/tile.sh" "$ROOT/app/room.sh" "$ROOT/app/uninstall.sh" "$ROOT/app/update.py" \
    "$ROOT/app/post-install.sh" "$ROOT/app/pre-remove.sh" "$ROOT/app/app.json" "$ROOT/app/manifest" "$APP/"
[ -s "$APP/emu/bananads" ] && [ -s "$APP/run-emu" ] && [ -s "$APP/lan.sh" ] && [ -s "$APP/status.sh" ] || {
    echo "package: emu/bananads, run-emu, lan.sh and status.sh are required" >&2
    exit 1
}
[ -s "$APP/emu/lobby.nds" ] || { echo "package: emu/lobby.nds (the room ROM) is required" >&2; exit 1; }
cp "$ROOT/tiles/Banana Party.sh" "$STAGE/Banana Party.sh"
cp "$ROOT/packaging/store/tile.png" "$APP/tile.png"
cp "$ROOT/INSTALL.txt" "$STAGE/INSTALL.txt"
chmod 755 "$APP/ui/banana-party" "$APP/tile.sh" "$APP/uninstall.sh" "$APP/update.py" \
    "$APP/post-install.sh" "$APP/pre-remove.sh" "$STAGE/Banana Party.sh"

ZIP=$ROOT/dist/banana-party-$ver.zip
python3 - "$STAGE" "$ZIP" "$ROOT/dist/SHA256SUMS" << 'PY'
import hashlib, os, sys, zipfile
stage, dest, sums = sys.argv[1], sys.argv[2], sys.argv[3]
with zipfile.ZipFile(dest, "w", zipfile.ZIP_DEFLATED) as z:
    for dirpath, _, files in os.walk(stage):
        for name in files:
            full = os.path.join(dirpath, name)
            rel = os.path.relpath(full, stage).replace(os.sep, "/")
            base = os.path.basename(rel)
            if base.startswith("._") or base == ".DS_Store" or "__MACOSX" in rel.split("/"):
                continue
            info = zipfile.ZipInfo(rel)
            info.external_attr = (os.stat(full).st_mode & 0xFFFF) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            with open(full, "rb") as fh:
                z.writestr(info, fh.read())
h = hashlib.sha256()
with open(dest, "rb") as fh:
    for chunk in iter(lambda: fh.read(1 << 20), b""):
        h.update(chunk)
with open(sums, "w") as fh:
    fh.write("%s  %s\n" % (h.hexdigest(), os.path.basename(dest)))
with zipfile.ZipFile(dest) as z:
    names = z.namelist()
    tops = set(n.split("/")[0] for n in names)
    want = {"banana-party", "Banana Party.sh", "INSTALL.txt"}
    if tops != want or any(n.startswith("store/") for n in names):
        sys.stderr.write("zip root is %s\n" % ", ".join(sorted(tops)))
        sys.exit(1)
print(dest)
PY
if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$ZIP" "$APP/emu/bananads" "$APP/ui/banana-party"
else
    shasum -a 256 "$ZIP" "$APP/emu/bananads" "$APP/ui/banana-party"
fi