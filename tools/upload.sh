#!/usr/bin/env bash
# Copies the title folder to a console over FTP. PS5_HOST picks the console (host:port).
#   tools/upload.sh            everything
#   tools/upload.sh eboot.bin  only the given files (paths inside the title folder)
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
host=${PS5_HOST:-192.168.68.125:1337}
cd "$root/dist"
if [[ $# -eq 0 ]]; then
    mapfile -t files < <(find PPSA69423 -type f)
else
    files=("${@/#/PPSA69423/}")
fi
for f in "${files[@]}"; do
    curl -s --ftp-create-dirs -T "$f" "ftp://$host/data/homebrew/$f" || { echo "failed: $f" >&2; exit 1; }
done
echo "upload.sh: ${#files[@]} file(s) to $host"
