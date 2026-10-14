# Cross-compiling Rush 2 Recompiled for the PS5: Mihawk's payload SDK fork, with
# SDL2 and FreeType from the PacBrew sysroot. Paths come from build.sh.
include("$ENV{PS5_PAYLOAD_SDK}/toolchain/prospero.cmake")
list(APPEND CMAKE_FIND_ROOT_PATH "$ENV{PACBREW}")
list(APPEND CMAKE_PREFIX_PATH "$ENV{PACBREW}")
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "" FORCE)
# Host tools the build runs (shader blobs, recompiler), built by build.sh for the host.
set(RUSH2_HOST_TOOLS "$ENV{RUSH2_HOST_TOOLS}")
set(RUSH2_PS5_DIR "${CMAKE_CURRENT_LIST_DIR}/../ps5" CACHE PATH "")
# SDL2 headers are included as <SDL.h>; windows are SDL (offscreen) windows, as RT64 has them on Linux.
set(CMAKE_C_FLAGS_INIT "-DPLUME_SDL_VULKAN_ENABLED -DRT64_SDL_WINDOW_VULKAN -DZSTD_TRACE=0 -isystem $ENV{PACBREW}/include/SDL2 -isystem $ENV{PACBREW}/include")
set(CMAKE_CXX_FLAGS_INIT "-DPLUME_SDL_VULKAN_ENABLED -DRT64_SDL_WINDOW_VULKAN -DZSTD_TRACE=0 -isystem $ENV{PACBREW}/include/SDL2 -isystem $ENV{PACBREW}/include")
# Executables are linked into a title by the PS5_Vulkan recipe (static RADV, signing).
set(CMAKE_CXX_LINK_EXECUTABLE "bash ${CMAKE_CURRENT_LIST_DIR}/../ps5/link-title.sh <TARGET> <OBJECTS> <LINK_LIBRARIES>")
