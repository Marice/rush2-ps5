#!/usr/bin/env bash
# Builds Rush 2 Recompiled for the PS5 in the build container.
#   ./build.sh radv        RADV, the payload SDK fork and the title converter (once)
#   ./build.sh configure   host tools + CMake configure (build-ps5/rush2)
#   ./build.sh [build]     compile (after configure)
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
: "${PACBREW:=$HOME/development/workspace/trimui/olise-player/.deps/pacbrew/v0.40.2/sysroot/user/homebrew}"
export PACBREW
sdk="$root/radv/mihawk-vulkan-review/.deps/native/ps5-payload-sdk"
env="export PS5_PAYLOAD_SDK=$sdk RADV_ROOT=$root/radv/mihawk-vulkan-review PACBREW=$PACBREW RUSH2_HOST_TOOLS=$root/build-host/file_to_c;"
case ${1:-build} in
radv)
    WORKDIR="$root/radv/mihawk-vulkan-review" "$root/docker/run.sh" "
        bash tools/setup-native-dependencies.sh && bash tools/build-radv.sh release &&
        n=tooling/native && z=.deps/native/zlib/root/usr && mkdir -p build/host &&
        clang++-18 -std=c++20 -O2 -I \$z/include \$n/native_app_builder.cpp \$n/self_container.cpp \
            \$n/elf_object.cpp \$n/sce_module_writer.cpp \$z/lib/libz.a -o build/host/ps5-native-tool &&
        bash tools/rebuild-libc.sh" ;;
configure)
    "$root/docker/run.sh" "$env
        cmake -S src-upstream/lib/rt64/src/tools/file_to_c -B build-host/file_to_c -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null &&
        cmake --build build-host/file_to_c >/dev/null &&
        cmake -S src-upstream -B build-ps5/rush2 -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
            -DCMAKE_TOOLCHAIN_FILE=$root/cmake/ps5-toolchain.cmake" ;;
build)
    "$root/docker/run.sh" "$env cmake --build build-ps5/rush2 -- -k 0" ;;
*)
    echo "usage: $0 [configure|build]" >&2; exit 2 ;;
esac
