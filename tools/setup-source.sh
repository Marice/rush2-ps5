#!/usr/bin/env bash
# Sets up the sources the PS5 build needs, next to this repository's files:
#   src-upstream/  Rush 2 Recompiled at the pinned commit, its submodules, upstream's own
#                  lib/patches and then this port's patches/
#   radv/          Mihawk's PS5_Vulkan, PS5_Mesa and PS5_PayloadSDK at the pinned commits
# and, given the Rush 2 (USA) ROM, generates the recompiled game code from it.
#   tools/setup-source.sh "Rush 2 - Extreme Racing USA (USA).n64"
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
rom=${1:?usage: tools/setup-source.sh <Rush 2 USA ROM>}
rom=$(realpath "$rom")

upstream_url=https://github.com/bryantmh/rush2recompiled
upstream_commit=5a60c91b4bf092a4f50a3ae315b990214bcc0f9b   # v1.0.18
src="$root/src-upstream"

if [[ ! -d $src ]]; then
    git clone -q "$upstream_url" "$src"
    git -C "$src" checkout -q "$upstream_commit"
    git -C "$src" submodule update --init --recursive --jobs 8
    # Upstream's own submodule patches, committed locally so tools/make-patches.sh diffs
    # only this port's changes against them.
    for module in rt64 N64ModernRuntime RecompFrontend; do
        git -C "$src/lib/$module" apply --whitespace=nowarn "$src/lib/patches/$module.patch"
        git -C "$src/lib/$module" -c user.name=base -c user.email=base@local \
            commit -qam "rush2recompiled lib/patches/$module.patch"
    done
    git -C "$src" apply "$root/patches/rush2recompiled.patch"
    for module in rt64 N64ModernRuntime RecompFrontend; do
        git -C "$src/lib/$module" apply "$root/patches/$module.patch"
    done
    git -C "$src/lib/rt64/src/contrib/plume" apply "$root/patches/plume.patch"
fi

# Mihawk's RADV port at the commits ProsperoEden pins (its tools/deps.json).
mkdir -p "$root/radv"
for pin in "PS5_Vulkan mihawk-vulkan-review 5b5e4fc2d80fdb67a4f61ba9e6a8a424026bb8a5" \
           "PS5_Mesa mihawk-mesa-review 7b59ef27c1b09b9671bc4153c41940c3155c3af2" \
           "PS5_PayloadSDK mihawk-sdk-review b83202be73e930050e00de0f1b4d9ec46c0391bf"; do
    read -r repo dir commit <<< "$pin"
    [[ -d $root/radv/$dir ]] || git clone -q "https://github.com/mihawk-99/$repo" "$root/radv/$dir"
    git -C "$root/radv/$dir" checkout -q "$commit"
done
# PS5_Vulkan's scripts look for the Mesa and SDK checkouts under these names.
ln -sfn mihawk-sdk-review "$root/radv/PS5_PayloadSDK"
ln -sfn mihawk-mesa-review "$root/radv/PS5_Mesa"

# The recompiled game code, generated from the ROM by the recompiler of the runtime's pin.
python3 -I "$src/tools/extract.py" "$rom"
"$root/docker/run.sh" "
    cmake -S src-upstream/lib/N64ModernRuntime/N64Recomp -B build-host/n64recomp -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18 >/dev/null &&
    cmake --build build-host/n64recomp --target N64RecompCLI RSPRecomp >/dev/null &&
    cd src-upstream && mkdir -p rsp &&
    ../build-host/n64recomp/N64Recomp us.toml >/dev/null &&
    ../build-host/n64recomp/RSPRecomp aspMain.us.toml >/dev/null"
echo "setup-source.sh: sources ready; next: ./build.sh radv, ./build.sh configure, ./build.sh"
