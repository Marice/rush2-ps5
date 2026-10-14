#!/usr/bin/env bash
# Runs a command in the build container with the project mounted at the same path.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
exec env DOCKER_CONFIG="$root/docker/.dockercfg" docker run --rm -u "$(id -u):$(id -g)" \
    -v "$root:$root" ${PACBREW:+-v "$PACBREW:$PACBREW:ro"} -w "${WORKDIR:-$root}" -e HOME="$root/.home" -e CCACHE_DIR="$root/.home/ccache" \
    rush2-ps5-build bash -lc "$*"
