// Start-up of Rush 2 Recompiled as a PS5 title (ps5_platform.cpp).
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <vector>

namespace rush2::ps5 {
    // Opens the buffered log in the title folder and selects SDL's offscreen video driver.
    // Call first in main, before SDL is initialized.
    void init();
    // Writes the buffered log out now (at exit and in error paths).
    void flush_log();

    // Checks a ROM file and gives its data in big-endian byte order; false if it is not the ROM wanted.
    using RomReader = std::function<bool(const std::filesystem::path&, std::vector<uint8_t>&)>;
    // The PS5 has no file dialog to select the SF Rush and Rush 2049 ROMs. If `stored` (the file the game keeps
    // the ROM in) does not exist, finds a .z64, .n64 or .v64 file of `size` bytes in the same folder whose
    // header has `game_code` at 0x3B, checks it with `read` and writes the result to `stored`.
    // Returns true if it imported a ROM.
    bool import_rom(const std::filesystem::path& stored, const char* game_code, size_t size, const RomReader& read);

    // The light bar of the controller on `port` shows `rgba` (0xRRGGBBAA, alpha ignored) from the next
    // update_light_bars on, until another color is set. Any thread.
    void set_light_bar(int port, uint32_t rgba);
    // Sends the light bar colors that changed, also to a controller that took the port since. Once per VI, from the
    // VI callback (next to rush2::input::update_rumble).
    void update_light_bars();

    // The console beeps and lights up at a race start (3, 2, 1, RUSH!) through the ps5-signal payload
    // (github.com/Marice/ps5-signal) when it runs; without it nothing happens. On by default.
    void set_race_signal(bool enabled);
    // Follows the game mode (0x8010C0D0: 10 the race intro, then 9 and 3 as the race starts; -1 when unknown) and
    // the intro's countdown (the number shown). Once per VI.
    void update_race_signal(int32_t game_mode, int32_t countdown);
}
