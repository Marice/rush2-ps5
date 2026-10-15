# Changelog

All notable changes to this project are documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/). Versions follow
the title's `contentVersion` in `sce_sys/param.json`.

## [Unreleased]

### Added
- The DualSense light bar shows the player's car color (MAIN COLOR), in the car
  select and in races, and keeps it between races.
- San Francisco Rush and Rush 2049 (USA) N64 ROMs in the title folder are found
  on start under any name and stored as `rush1.z64` and `rush2049.z64`. The
  Games tab's ROM buttons need a file dialog, which the PS5 does not have.
- `tools/upload.sh` takes `PS5_DIR`, the folder that holds the title folder, for
  a title the Prospero Store installed in `/mnt/ext1/etaHEN/games`.

### Changed
- Based on Rush 2 Recompiled v1.0.19 (was v1.0.18): Rush 2049 paint colors,
  custom track fixes, shorter hitches and an optional, unfinished car lighting
  feature.
- The touchpad click opens and closes the Rush 2 Recompiled menu. The PS5 keeps
  the Create and PS buttons for itself, so the default Back button never reached
  the game. The touchpad can no longer be bound to a game action.

### Fixed
- The build no longer needs another project's folder for the PacBrew sysroot.
  `build.sh` downloads PacBrew v0.40.2 into `.deps/` (`tools/setup-pacbrew.sh`).
- `./build.sh configure` failed on a fresh setup: upstream's CMake check for its
  own submodule patches took the port's changes for a patch that is not applied.
  The PS5 build now skips that check; `tools/setup-source.sh` applies those
  patches.
- The game no longer crashes on start after Data Location was set to Portable.
  Portable mode asks for a working directory, which a PS5 title does not have.
  The PS5 build ignores `portable.txt`.
- The Custom upscaler no longer calls a program-start function the title does
  not have, which would crash the game. It reports a failure instead.

### Removed
- The Data Location option. A PS5 title keeps its data in the title folder.
- The Games tab's Select ROM buttons, which open a file dialog the PS5 does not
  have. The tab says to put the ROM in the title folder.
- The Open Dump Folder button. The asset_dump folder is reached over FTP.

## [00.001.000] - 2026-10-14

First build of Rush 2 Recompiled (v1.0.18) as a native PS5 title.

### Added
- Native title `PPSA69423`, rendering with RT64 through Vulkan on Mihawk's RADV
  port, output at 3840x2160 on the console's display plane.
- 60 fps by default: RT64 interpolates the game's frames to the 60 Hz output.
- DualSense and sound through SDL2.
- The ROM is imported from the title folder on the first start; settings and
  saves are kept there.
- `rush2.log` in the title folder with errors and crash reports.
- Home screen icon and background.

### Known issues
- Tested on one console (FW 13.60, ShadowMountPlus) only.
- Native code mods cannot be loaded; the mods folder is reached over FTP.
- The first start compiles the renderer's shaders and takes longer.
