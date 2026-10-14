#!/usr/bin/env bash
# Turns the crash addresses in rush2.log into functions and source lines.
#   tools/symbolize.sh [rush2.log]   (default: fetch it from PS5_HOST over FTP)
# The title is loaded at main's logged address minus main's address in the ELF.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
elf="$root/build-ps5/rush2/Rush2Recompiled.elf"
llvm=${LLVM_BIN:-/usr/lib/llvm-18/bin}
if [[ $# -gt 0 ]]; then
    log=$(cat "$1")
else
    log=$(curl -s "ftp://${PS5_HOST:-192.168.68.125:1337}/data/homebrew/PPSA69423/rush2.log")
fi
main_run=$(grep -o "main at [0-9a-fx]*" <<<"$log" | head -1 | awk '{print $3}')
[[ -n $main_run ]] || { echo "no crash report in the log" >&2; exit 1; }
main_elf=$("$llvm/llvm-nm" "$elf" | awk '$3 == "main" {print $1; exit}')
base=$(( 0x${main_run#0x} - 0x$main_elf ))
grep -oE "(rip|stack\[[0-9]+\]|#[0-9]+) [0-9a-fx]+" <<<"$log" | while read -r label address; do
    [[ $address == 0x* ]] || address="0x$address"
    printf '%-10s %s  ' "$label" "$address"
    "$llvm/llvm-symbolizer" --obj="$elf" -f -C "$(printf '0x%x' $(( address - base )))" | head -2 | paste -sd ' ' |
        sed "s#$root/##g"
done
