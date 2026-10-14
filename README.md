# Rush 2 Recompiled for the PlayStation 5

A port of [Rush 2 Recompiled](https://github.com/bryantmh/rush2recompiled) (v1.0.18) to native
PS5 homebrew. It runs as an installed title (`PPSA69423`) and renders with RT64 through Vulkan
on [Mihawk's RADV port](https://github.com/mihawk-99/PS5_Vulkan).

> **Private.** Rush 2 Recompiled has no licence yet; its author has been asked for permission
> to publish this port. Until then this repository and its builds stay private.

## What works

Tested on one console (FW 13.60, ShadowMountPlus):

- The game, its menus and races, with sound and the DualSense
- Output at 3840x2160, rendered at the output resolution (`Auto`)
- 60 fps: RT64 interpolates the game's frames to the console's 60 Hz output

## You need your own ROM

Neither this repository nor its builds contain the game. You need Rush 2: Extreme Racing USA
(USA) as a ROM (`.z64`, `.n64` or `.v64`):

- to **play**: put it in the title folder (`/data/homebrew/PPSA69423/`); the game imports it on
  its first start
- to **build**: the game's code is generated from it (see Building)

## Installing

1. Extract `Rush2Recompiled-PS5-<version>.zip`.
2. Copy the `PPSA69423` folder to `/data/homebrew/` on the console (for example over FTP).
3. Put your ROM in that folder.
4. Let ShadowMountPlus register the title, then start Rush 2 Recompiled from the home screen.

Settings, saves and caches live in the title folder. `rush2.log` there records errors and
crashes.

## Building

You need Linux or WSL with Docker, Python 3 and git. Everything else is built in the container
(`docker/Dockerfile`: Ubuntu 26.04 with LLVM 18 and 21), and SDL2, FreeType and their libraries
come from a PacBrew sysroot (`PACBREW`, see `build.sh`).

```sh
docker build -t rush2-ps5-build docker            # once
tools/setup-source.sh "Rush 2 - Extreme Racing USA (USA).n64"
./build.sh radv                                    # once: RADV, SDK fork, title converter
./build.sh configure
./build.sh
tools/package.sh                                   # dist/PPSA69423 with your ROM, for testing
tools/upload.sh                                    # to PS5_HOST over FTP
tools/package.sh --release                         # dist/Rush2Recompiled-PS5-<version>.zip, no ROM
```

`tools/setup-source.sh` clones Rush 2 Recompiled at the pinned commit with its submodules,
applies upstream's own `lib/patches` and then this port's `patches/`, clones Mihawk's
repositories at the commits ProsperoEden pins, and generates the game's code from the ROM.
After changing the sources in `src-upstream/`, `tools/make-patches.sh` writes `patches/` again.

The home screen icon and backgrounds (`sce_sys/icon0.png`, `pic0.dds`, `pic1.dds`) are not in
the repository; `tools/make-backgrounds.py` makes the backgrounds from a picture.

## How the port works

| Part | What it does |
| --- | --- |
| `cmake/ps5-toolchain.cmake` | Cross-compiles with Mihawk's payload SDK fork and PacBrew; host tools run on the host |
| `ps5/link-title.sh` | Links the title with the static RADV driver and the SDK's platform layer, then converts and signs it |
| `ps5/ps5_platform.cpp` | Buffered log in the title folder, crash reports, SDL's offscreen video driver, hides the splash screen |
| `ps5/ps5_paths.c` | Resolves relative paths against the title folder: a title has no working directory |
| `ps5/ps5-shims.c`, `ps5/nfd_stub.c` | C library functions the SDK lacks; file dialogs that always cancel |
| `patches/plume.patch` | Presents through `VK_KHR_display` on the console's display plane |
| `patches/N64ModernRuntime.patch` | N64 memory from a kernel reservation at 0x300000000 backed by direct memory |
| `patches/rt64.patch` | Host tools in a cross build, PS5 branches, ubershader pipelines compiled without temporary threads |
| `patches/RecompFrontend.patch`, `patches/rush2recompiled.patch` | PS5 branches: paths in the title folder, window, framerate default |

`spike/` holds the memory test title that measured how a title may reserve the N64 address
space.

## Licences and credits

- Rush 2 Recompiled by bryantmh: no licence published.
- N64Recomp, N64ModernRuntime, RecompFrontend and RT64: see their repositories.
- Mesa/RADV PS5 port, PS5_Vulkan and the payload SDK fork by Mihawk-99 (GPL-3.0 for PS5_Vulkan).
- Rush 2: Extreme Racing USA is © Midway. This port is not affiliated with or endorsed by
  Midway, Warner Bros., the Rush 2 Recompiled authors or Sony.
