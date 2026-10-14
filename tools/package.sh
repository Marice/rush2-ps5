#!/usr/bin/env bash
# Assembles the title folder dist/PPSA69423 from the build: eboot, home screen files,
# the system libc module, the UI assets and the ROM (if present next to the repo).
set -euo pipefail
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
cp "$upstream/recompcontrollerdb.txt" "$app/"
# The game imports a ROM it finds in the title folder on its first start.
if [[ -f $upstream/rush2.us.z64 ]]; then
    cp "$upstream/rush2.us.z64" "$app/"
fi
echo "package.sh: $app ($(du -sh "$app" | cut -f1))"
