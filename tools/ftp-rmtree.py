#!/usr/bin/env python3
"""Deletes a folder and everything in it on a console over FTP.

Usage: tools/ftp-rmtree.py HOST:PORT /data/homebrew/PPSAxxxxx
Only paths under /data/homebrew/ are accepted, so a typo cannot reach system folders.
"""
import ftplib
import sys


def rmtree(ftp, path):
    entries = []
    ftp.retrlines(f"LIST {path}", entries.append)
    for line in entries:
        name = line.split(None, 8)[-1]
        if name in (".", ".."):
            continue
        child = f"{path}/{name}"
        if line.startswith("d"):
            rmtree(ftp, child)
        else:
            ftp.delete(child)
    ftp.rmd(path)


def main():
    host, port = sys.argv[1].split(":")
    path = sys.argv[2].rstrip("/")
    if not path.startswith("/data/homebrew/") or path.count("/") < 3:
        raise SystemExit(f"refusing to delete {path}: only folders in /data/homebrew/")
    ftp = ftplib.FTP()
    ftp.connect(host, int(port), timeout=20)
    ftp.login()
    rmtree(ftp, path)
    ftp.quit()
    print(f"deleted {path}")


if __name__ == "__main__":
    main()
