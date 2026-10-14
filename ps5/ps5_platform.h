// Start-up of Rush 2 Recompiled as a PS5 title (ps5_platform.cpp).
#pragma once

namespace rush2::ps5 {
    // Opens the buffered log in the title folder and selects SDL's offscreen video driver.
    // Call first in main, before SDL is initialized.
    void init();
    // Writes the buffered log out now (at exit and in error paths).
    void flush_log();
    // Logs a start-up step and writes the log out at once, so a crash shows how far start-up got.
    void step(const char* name);
}

#if defined(__PROSPERO__)
#define RUSH2_PS5_STEP(name) rush2::ps5::step(name)
#else
#define RUSH2_PS5_STEP(name) ((void)0)
#endif
