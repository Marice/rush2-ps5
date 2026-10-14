// Start-up of Rush 2 Recompiled as a PS5 title: the log file and SDL's video driver.
#include "ps5_platform.h"

#include <SDL.h>

#include <chrono>
#include <cstdio>
#include <thread>
#include <unistd.h>

namespace {
    constexpr const char* log_path = "/app0/rush2.log";
    constexpr size_t log_buffer_size = 64 * 1024;
    constexpr auto log_flush_interval = std::chrono::seconds(5);

    // Many small writes to the title folder are very slow on the console (each one stalls), so the
    // log is fully buffered and written out on a timer instead of per line.
    void open_log() {
        if (std::freopen(log_path, "w", stdout) == nullptr) {
            return;
        }
        static char stdout_buffer[log_buffer_size];
        std::setvbuf(stdout, stdout_buffer, _IOFBF, sizeof(stdout_buffer));
        dup2(fileno(stdout), fileno(stderr));
        static char stderr_buffer[log_buffer_size];
        std::setvbuf(stderr, stderr_buffer, _IOFBF, sizeof(stderr_buffer));

        std::thread([] {
            for (;;) {
                std::this_thread::sleep_for(log_flush_interval);
                std::fflush(stderr);
                std::fflush(stdout);
            }
        }).detach();
    }
}

void rush2::ps5::init() {
    open_log();
    std::printf("Rush 2 Recompiled for the PS5: starting\n");
    // There is no video driver for the console's display: windows are offscreen windows, and RT64
    // presents through VK_KHR_display (plume) instead of the window.
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
}

void rush2::ps5::flush_log() {
    std::fflush(stderr);
    std::fflush(stdout);
}
