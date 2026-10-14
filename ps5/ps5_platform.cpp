// Start-up of Rush 2 Recompiled as a PS5 title: the log file and SDL's video driver.
#include "ps5_platform.h"

#include <SDL.h>

#include <unwind.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include <chrono>
#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <thread>

int main(int argc, char** argv);
extern "C" int sceSystemServiceHideSplashScreen(void);

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

    std::atomic<bool> crash_reported{false};

    // The faulting instruction and the code addresses found on the stack above it: the backtrace
    // of a signal stops at the signal frame. Addresses match Rush2Recompiled.elf (llvm-symbolizer).
    // Only the first crashing thread reports; the others wait for the process to end.
    void on_signal(int signal, siginfo_t* info, void* context) {
        if (crash_reported.exchange(true)) {
            for (;;) {
                sleep(1);
            }
        }
        std::fflush(stdout);
        std::fflush(stderr);
        const mcontext_t& machine = static_cast<ucontext_t*>(context)->uc_mcontext;
        raw_printf("signal %d at address %p\n", signal, info != nullptr ? info->si_addr : nullptr);
        raw_printf("  rip %p rsp %p rbp %p (main at %p)\n", reinterpret_cast<void*>(machine.mc_rip),
                   reinterpret_cast<void*>(machine.mc_rsp), reinterpret_cast<void*>(machine.mc_rbp),
                   reinterpret_cast<void*>(&main));
        const uintptr_t code_start = 0x400000;
        const uintptr_t code_end = code_start + 0x8000000;
        const uintptr_t* stack = reinterpret_cast<const uintptr_t*>(machine.mc_rsp);
        int found = 0;
        for (int i = 0; i < 1024 && found < 32; i++) {
            if (stack[i] >= code_start && stack[i] < code_end) {
                raw_printf("  stack[%d] %p\n", i, reinterpret_cast<void*>(stack[i]));
                found++;
            }
        }
        std::_Exit(128 + signal);
    }
}

void rush2::ps5::init() {
    open_log();
    std::set_terminate(on_terminate);
    struct sigaction action {};
    action.sa_sigaction = on_signal;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    for (int sig : { SIGABRT, SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGSYS }) {
        sigaction(sig, &action, nullptr);
    }
    // There is no video driver for the console's display: windows are offscreen windows, and RT64
    // presents through VK_KHR_display (plume) instead of the window.
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
    // The system's splash screen stays over the title's output until the title hides it.
    sceSystemServiceHideSplashScreen();
}

void rush2::ps5::flush_log() {
    std::fflush(stderr);
    std::fflush(stdout);
}
