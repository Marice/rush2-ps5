#!/usr/bin/env bash
# Writes the PS5 changes in src-upstream to patches/: one patch for the game's own tree and
# one per submodule. Submodule patches are against the submodule with upstream's own
# lib/patches/<name>.patch applied (committed locally as the submodule's HEAD).
# plume is a submodule of RT64 with no upstream patch of its own.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
src="$root/src-upstream"
mkdir -p "$root/patches"
git -C "$src" diff --ignore-submodules=all > "$root/patches/rush2recompiled.patch"
for module in rt64 N64ModernRuntime RecompFrontend; do
    git -C "$src/lib/$module" diff > "$root/patches/$module.patch"
done
git -C "$src/lib/rt64/src/contrib/plume" diff > "$root/patches/plume.patch"
wc -l "$root"/patches/*.patch
