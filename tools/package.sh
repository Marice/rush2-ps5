#!/usr/bin/env bash
# Assembles the title folder dist/PPSA69423 from the build: eboot, home screen files,
# the system libc module, the UI assets and, for testing, the ROM (if extracted).
#   tools/package.sh           title folder with the ROM, for upload.sh
#   tools/package.sh --release title folder without the ROM, zipped with its sha256
set -euo pipefail
release=false
[[ ${1:-} == --release ]] && release=true
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build="$root/build-ps5/rush2"
upstream="$root/src-upstream"
radv="$root/radv/mihawk-vulkan-review"
app="$root/dist/PPSA69423"

rm -rf -- "$app"
mkdir -p "$app/sce_sys" "$app/sce_module"
cp "$build/Rush2Recompiled" "$app/eboot.bin"
cp "$root"/sce_sys/{param.json,icon0.png,pic0.dds,pic1.dds} "$app/sce_sys/"
cp "$radv/runtime/libc.prx" "$app/sce_module/libc.prx"
cp -r "$upstream/assets" "$app/assets"
# The port's own art replaces upstream's launcher logo and window icon (assets/icon.png).
python3 -I - "$root/assets/icon.png" "$app/assets" <<'PY'
import sys
from pathlib import Path
from PIL import Image
icon = Image.open(sys.argv[1]).convert("RGBA")
out = Path(sys.argv[2])
icon.resize((512, 512), Image.LANCZOS).save(out / "logo.png", optimize=True)
out.joinpath("icon_128.rgba").write_bytes(icon.resize((128, 128), Image.LANCZOS).tobytes())
PY
cp "$upstream/recompcontrollerdb.txt" "$app/"
# The game imports a ROM it finds in the title folder on its first start. A release never
# carries one: players put their own in the folder.
if ! $release && [[ -f $upstream/rush2.us.z64 ]]; then
    cp "$upstream/rush2.us.z64" "$app/"
fi
if $release; then
    version=$(python3 -c 'import json, sys; print(json.load(open(sys.argv[1]))["contentVersion"])' "$app/sce_sys/param.json")
    zip="$root/dist/Rush2Recompiled-PS5-$version.zip"
    rm -f "$zip"
    (cd "$root/dist" && zip -qr -9 "$zip" PPSA69423)
    (cd "$root/dist" && sha256sum "$(basename "$zip")" > "$(basename "$zip").sha256")
    echo "package.sh: $zip ($(du -h "$zip" | cut -f1))"
else
    echo "package.sh: $app ($(du -sh "$app" | cut -f1))"
fi
