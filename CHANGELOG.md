# Changelog

All notable changes to this project are documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/). Versions follow
the title's `contentVersion` in `sce_sys/param.json`.

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

### Known issues
- Tested on one console (FW 13.60, ShadowMountPlus) only.
- Native code mods cannot be loaded; the mods folder is reached over FTP.
- The first start compiles the renderer's shaders and takes longer.
