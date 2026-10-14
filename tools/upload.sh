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
# The console's FTP server can refuse to overwrite a file (550), as it did for eboot.bin, so
# each file goes up under a temporary name, the old one is deleted (if there) and the new one renamed.
for f in "${files[@]}"; do
    remote="/data/homebrew/$f"
    curl -s --ftp-create-dirs -T "$f" "ftp://$host$remote.new" || { echo "failed: $f" >&2; exit 1; }
    curl -s "ftp://$host/" -Q "*DELE $remote" -Q "RNFR $remote.new" -Q "RNTO $remote" -o /dev/null ||
        { echo "failed to replace: $f" >&2; exit 1; }
done
echo "upload.sh: ${#files[@]} file(s) to $host"
