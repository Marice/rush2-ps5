// Start-up of Rush 2 Recompiled as a PS5 title: the log file and SDL's video driver.
#include "ps5_platform.h"

#include <SDL.h>

#include <unwind.h>
#include <signal.h>
#include <unistd.h>

#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <thread>

int main(int argc, char** argv);

namespace {
    constexpr const char* log_path = "/app0/rush2.log";
    constexpr size_t log_buffer_size = 64 * 1024;
    constexpr auto log_flush_interval = std::chrono::seconds(5);
    int log_fd = -1;

    // Many small writes to the title folder are very slow on the console (each one stalls), so the
    // log is fully buffered and written out on a timer instead of per line.
    void open_log() {
        // stdout and stderr each get their own stream on the log, both appending, so neither
        // overwrites the other (dup2 onto stderr's descriptor did not reach this libc's stderr).
        if (FILE* truncate = std::fopen(log_path, "w")) {
            std::fclose(truncate);
        }
        if (std::freopen(log_path, "a", stdout) == nullptr) {
            return;
        }
        static char stdout_buffer[log_buffer_size];
        std::setvbuf(stdout, stdout_buffer, _IOFBF, sizeof(stdout_buffer));
        log_fd = fileno(stdout);
        if (std::freopen(log_path, "a", stderr) != nullptr) {
            static char stderr_buffer[log_buffer_size];
            std::setvbuf(stderr, stderr_buffer, _IOFBF, sizeof(stderr_buffer));
        }

        std::thread([] {
            for (;;) {
                std::this_thread::sleep_for(log_flush_interval);
                std::fflush(stderr);
                std::fflush(stdout);
            }
        }).detach();
    }
}

namespace {
    // Crash output goes straight to the log's descriptor with write(): stdio may hold locks, and
    // backtrace() makes a system call a title may not use (it ended the process with SIGSYS).

    void raw_write(const char* text) {
        if (log_fd >= 0) {
            (void)write(log_fd, text, std::strlen(text));
        }
    }

    void raw_printf(const char* format, ...) {
        char line[512];
        va_list args;
        va_start(args, format);
        std::vsnprintf(line, sizeof(line), format, args);
        va_end(args);
        raw_write(line);
    }

    _Unwind_Reason_Code backtrace_frame(_Unwind_Context* context, void* arg) {
        int* index = static_cast<int*>(arg);
        uintptr_t ip = _Unwind_GetIP(context);
        if (ip == 0 || *index >= 64) {
            return _URC_END_OF_STACK;
        }
        raw_printf("  #%d %p\n", (*index)++, reinterpret_cast<void*>(ip));
        return _URC_NO_REASON;
    }

    // The return addresses of the current stack, with main's address so they can be matched to
    // the unsigned ELF (build-ps5/rush2/Rush2Recompiled.elf) with llvm-symbolizer.
    void write_backtrace() {
        raw_printf("backtrace (main at %p):\n", reinterpret_cast<void*>(&main));
        int index = 0;
        _Unwind_Backtrace(backtrace_frame, &index);
    }

    [[noreturn]] void on_terminate() {
        std::fflush(stdout);
        std::fflush(stderr);
        raw_write("terminate called");
        if (std::exception_ptr current = std::current_exception()) {
            try {
                std::rethrow_exception(current);
            }
            catch (const std::exception& e) {
                raw_printf(": uncaught exception: %s", e.what());
            }
            catch (...) {
                raw_write(": uncaught exception of an unknown type");
            }
        }
        raw_write("\n");
        write_backtrace();
        std::_Exit(EXIT_FAILURE);
    }

    void on_signal(int signal) {
        raw_printf("signal %d\n", signal);
        write_backtrace();
        std::_Exit(128 + signal);
    }
}

namespace {
    // A title has no environment. Optional NAME=VALUE lines in the title folder set variables for
    // the libraries that read them (RADV_DEBUG, MESA_DEBUG), before any of them starts.
    void load_environment() {
        FILE* file = std::fopen("/app0/rush2-env.txt", "r");
        if (file == nullptr) {
            return;
        }
        char line[256];
        while (std::fgets(line, sizeof(line), file) != nullptr) {
            line[std::strcspn(line, "\r\n")] = '\0';
            char* equals = std::strchr(line, '=');
            if (equals != nullptr && line[0] != '#') {
                *equals = '\0';
                setenv(line, equals + 1, 1);
                std::printf("environment: %s=%s\n", line, equals + 1);
            }
        }
        std::fclose(file);
    }
}

void rush2::ps5::init() {
    open_log();
    load_environment();
    std::set_terminate(on_terminate);
    for (int sig : { SIGABRT, SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGSYS }) {
        signal(sig, on_signal);
    }
    std::printf("Rush 2 Recompiled for the PS5: starting\n");
    // There is no video driver for the console's display: windows are offscreen windows, and RT64
    // presents through VK_KHR_display (plume) instead of the window.
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
}

void rush2::ps5::step(const char* name) {
    std::fprintf(stdout, "[start] %s\n", name);
    flush_log();
}

void rush2::ps5::flush_log() {
    std::fflush(stderr);
    std::fflush(stdout);
}
