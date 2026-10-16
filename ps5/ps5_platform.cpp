// Start-up of Rush 2 Recompiled as a PS5 title: the log file and SDL's video driver, the import of the
// SF Rush and Rush 2049 ROMs, and the controllers' light bars in the car colors.
#include "ps5_platform.h"
#include "rush2.h"

#include <SDL.h>

#include <unwind.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include <array>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <string>
#include <thread>
#include <utility>

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

namespace {
    // The 3-letter game code at 0x3B of an N64 ROM's header, in any of the three byte orders; "" if the file
    // has no N64 ROM header.
    std::string header_game_code(const std::filesystem::path& path) {
        uint8_t header[0x40];
        std::ifstream file{ path, std::ios::binary };
        if (!file.read(reinterpret_cast<char*>(header), sizeof(header))) {
            return {};
        }
        bool byteswapped = header[0] == 0x37 && header[1] == 0x80 && header[2] == 0x40 && header[3] == 0x12;
        bool little_endian = header[0] == 0x40 && header[1] == 0x12 && header[2] == 0x37 && header[3] == 0x80;
        bool big_endian = header[0] == 0x80 && header[1] == 0x37 && header[2] == 0x12 && header[3] == 0x40;
        if (!big_endian && !byteswapped && !little_endian) {
            return {};
        }
        for (size_t i = 0; i < sizeof(header); i += 4) {
            if (byteswapped) {
                std::swap(header[i], header[i + 1]);
                std::swap(header[i + 2], header[i + 3]);
            }
            else if (little_endian) {
                std::swap(header[i], header[i + 3]);
                std::swap(header[i + 1], header[i + 2]);
            }
        }
        return std::string(reinterpret_cast<const char*>(&header[0x3B]), 3);
    }
}

bool rush2::ps5::import_rom(const std::filesystem::path& stored, const char* game_code, size_t size, const RomReader& read) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (fs::exists(stored, ec)) {
        return false;
    }
    for (const auto& entry : fs::directory_iterator(stored.parent_path(), ec)) {
        const fs::path& path = entry.path();
        std::string ext = path.extension().string();
        // Lower case only, as for the Rush 2 ROM (main.cpp's auto_import_rom).
        if ((ext != ".z64" && ext != ".n64" && ext != ".v64") || !entry.is_regular_file(ec) ||
            entry.file_size(ec) != size || header_game_code(path) != game_code) {
            continue;
        }
        std::vector<uint8_t> data;
        if (!read(path, data)) {
            printf("[PS5] Skipped %s: not the USA version this needs\n", path.string().c_str());
            continue;
        }
        std::ofstream out{ stored, std::ios::binary };
        if (!out.write(reinterpret_cast<const char*>(data.data()), data.size())) {
            out.close();
            fs::remove(stored, ec);
            printf("[PS5] Failed to write %s\n", stored.string().c_str());
            return false;
        }
        printf("[PS5] Imported %s as %s\n", path.string().c_str(), stored.filename().string().c_str());
        return true;
    }
    return false;
}

namespace {
    // The car color each port's light bar should show (0: none chosen yet, the system's color stays), set from the
    // game thread; and what was last sent, to which controller, on the VI thread.
    constexpr int light_bar_ports = 4;
    std::array<std::atomic<uint32_t>, light_bar_ports> light_bar_wanted{};
    std::array<int32_t, light_bar_ports> light_bar_controller{ -1, -1, -1, -1 };
    std::array<uint32_t, light_bar_ports> light_bar_sent{};
}

void rush2::ps5::set_light_bar(int port, uint32_t rgba) {
    if (port >= 0 && port < light_bar_ports) {
        light_bar_wanted[port].store(rgba | 0xFF, std::memory_order_relaxed); // Never 0 once a color is set.
    }
}

void rush2::ps5::update_light_bars() {
    std::lock_guard lock{ rush2::input::sdl_device_mutex() };
    for (int port = 0; port < light_bar_ports; port++) {
        uint32_t wanted = light_bar_wanted[port].load(std::memory_order_relaxed);
        int32_t id = rush2::input::get_port_controller(port);
        if (wanted == 0 || id < 0 || (id == light_bar_controller[port] && wanted == light_bar_sent[port])) {
            continue;
        }
        // Sent once per change even if it fails, so a pad without a light bar isn't asked again every VI.
        light_bar_controller[port] = id;
        light_bar_sent[port] = wanted;
        if (SDL_GameController* controller = SDL_GameControllerFromInstanceID(id)) {
            SDL_GameControllerSetLED(controller, uint8_t(wanted >> 24), uint8_t(wanted >> 16), uint8_t(wanted >> 8));
        }
    }
}


// ---- The console's beeper and light at a race start, through ps5-signal ----------------------------------------
// ps5-signal (github.com/Marice/ps5-signal) is a payload that plays beeps and LED patterns for homebrew over HTTP
// on 127.0.0.1:9150; ps5_signal.h is its client. When it doesn't run, every request fails at once and nothing
// happens. The countdown (game mode 9) shows 3, 2 and 1, then the race (mode 3) starts with RUSH!: three short beeps
// a tick apart and an amber flash per tick, then a long beep and the LEDs cycling. The service puts the player's own
// colour back after a light pattern.

#define PS5_SIGNAL_IMPLEMENTATION
#include "ps5_signal.h"

namespace {
    constexpr int32_t mode_countdown = 9;
    constexpr int32_t mode_racing = 3;
    // A tick of the countdown until one was measured: then a third of how long the last countdown lasted.
    constexpr int default_tick_ms = 1000;

    std::atomic<bool> race_signal_enabled{ true };
    int32_t race_signal_mode = -1;
    std::chrono::steady_clock::time_point countdown_started;
    int tick_ms = default_tick_ms;

    // The requests go out on a thread of their own: each can take up to 2 seconds when the service hangs.
    std::mutex signal_mutex;
    std::condition_variable signal_wake;
    std::vector<std::pair<std::string, std::string>> signal_queue; // { beep pattern, LED pattern }

    void signal_thread() {
        while (true) {
            std::pair<std::string, std::string> request;
            {
                std::unique_lock lock{ signal_mutex };
                signal_wake.wait(lock, [] { return !signal_queue.empty(); });
                request = std::move(signal_queue.front());
                signal_queue.erase(signal_queue.begin());
            }
            const int beep = ps5_signal_beep(request.first.c_str());
            const int led = ps5_signal_led(request.second.c_str(), 1);
            if (beep != 202 || led != 202) {
                std::printf("[ps5-signal] race start: beep %d, led %d (-1: ps5-signal isn't running)\n", beep, led);
            }
        }
    }

    void send_signal(std::string beep, std::string led) {
        static std::once_flag started;
        std::call_once(started, [] { std::thread(signal_thread).detach(); });
        {
            std::lock_guard lock{ signal_mutex };
            signal_queue.emplace_back(std::move(beep), std::move(led));
        }
        signal_wake.notify_one();
    }

    // 3, 2, 1: three short beeps a tick apart; an amber flash on each, ending before RUSH! needs the light.
    void signal_countdown() {
        char beep[160];
        std::snprintf(beep, sizeof(beep), "[{\"tone\":1,\"pause_ms\":%d},{\"tone\":1,\"pause_ms\":%d},{\"tone\":1}]",
                      tick_ms, tick_ms);
        const int dark = std::max(tick_ms - 250, 250);
        char led[256];
        std::snprintf(led, sizeof(led),
                      "[{\"color\":\"orange\",\"ms\":250},{\"color\":\"off\",\"ms\":%d},"
                      "{\"color\":\"orange\",\"ms\":250},{\"color\":\"off\",\"ms\":%d},{\"color\":\"orange\",\"ms\":250}]",
                      dark, dark);
        send_signal(beep, led);
    }

    // RUSH!: a long beep and the LEDs cycling twice.
    void signal_go() {
        send_signal("[{\"tone\":3}]",
                    "[{\"color\":\"blue\",\"ms\":250},{\"color\":\"white\",\"ms\":250},{\"color\":\"orange\",\"ms\":250},"
                    "{\"color\":\"blue\",\"ms\":250},{\"color\":\"white\",\"ms\":250},{\"color\":\"orange\",\"ms\":250}]");
    }
}

void rush2::ps5::set_race_signal(bool enabled) {
    race_signal_enabled.store(enabled, std::memory_order_relaxed);
}

void rush2::ps5::update_race_signal(int32_t game_mode) {
    const int32_t previous = race_signal_mode;
    race_signal_mode = game_mode;
    if (game_mode == previous) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (game_mode == mode_countdown) {
        countdown_started = now;
        if (race_signal_enabled.load(std::memory_order_relaxed)) {
            signal_countdown();
        }
    }
    else if (game_mode == mode_racing && previous == mode_countdown) {
        // The countdown shows three numbers: the next race's tick is a third of this one's length.
        const auto lasted = std::chrono::duration_cast<std::chrono::milliseconds>(now - countdown_started).count();
        std::printf("[ps5-signal] the countdown lasted %lld ms\n", (long long)lasted);
        if (lasted >= 900 && lasted <= 9000) {
            tick_ms = std::clamp((int)(lasted / 3), 300, 2000);
        }
        if (race_signal_enabled.load(std::memory_order_relaxed)) {
            signal_go();
        }
    }
}
