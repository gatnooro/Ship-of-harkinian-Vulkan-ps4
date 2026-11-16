// PS4: process start (from the Wind Waker HD PS4 port's port/ps4_platform.cpp, MIT; adapted for Ship of Harkinian).
//
//   - loads the system modules the port calls into (calling into a module that isn't loaded ends the app silently)
//   - /data/sohvk is the working folder: saves (save/), settings, controls and shader caches (user/), logs and crash
//     reports (captures/, wwhd.log), and env.txt (WWHD_ settings, one NAME=value per line, # comments)
//   - the game files: /app0/game inside the package when the Maker bundled them, else /data/sohvk/game (FTP)
//   - the PS4's defaults (env.txt overrides any of them)
#ifndef PS4_DATA_DIR
#define PS4_DATA_DIR "/data/sohvk"  // the app's folder (logs, crash reports, settings)
#endif
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

extern "C" {
int32_t sceSysmoduleLoadModuleInternal(uint32_t id);  // libSceSysmodule
void ps4_boot_trace(const char* line);                // port/ps4_crashlog.cpp
}

namespace {

const char* const kData = PS4_DATA_DIR;

void trace(const char* fmt, const char* a = "", const char* b = "") {
    char line[512];
    snprintf(line, sizeof line, fmt, a, b);
    ps4_boot_trace(line);
}

bool exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

void load_system_modules() {
    const struct {
        const char* name;
        uint32_t id;
    } modules[] = {
        {"SystemService", 0x80000010}, {"UserService", 0x80000011}, {"VideoOut", 0x80000022},
        {"AudioOut", 0x80000001},      {"Pad", 0x80000024},
    };
    for (const auto& m : modules) {
        int32_t r = sceSysmoduleLoadModuleInternal(m.id);
        if (r != 0) {
            char hex[16];
            snprintf(hex, sizeof hex, "%x", unsigned(r));
            trace("startup: loading system module %s failed (%s)", m.name, hex);
        }
    }
}

// /data/sohvk/env.txt: NAME=value per line into the environment (as Android's env.txt). Absolute paths: in 0.1.5 the
// relative "env.txt" was not found after chdir (nor captures/wwhd.log written). Windows hides known extensions, so a
// file made there as "env.txt" may really be env.txt.txt: that name is read too.
void load_env_txt() {
    const char* used = PS4_DATA_DIR "/env.txt";
    FILE* f = fopen(used, "r");
    if (!f) f = fopen(used = PS4_DATA_DIR "/env.txt.txt", "r");
    if (!f) {
        trace("startup: no env.txt (the defaults)");
        return;
    }
    trace("startup: reading %s", used);
    char line[512];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char* eq = strchr(line, '=');
        if (line[0] == '#' || !eq || eq == line) continue;
        *eq = 0;
        setenv(line, eq + 1, 1);
        trace("startup: env.txt %s=%s", line, eq + 1);
    }
    fclose(f);
}

extern "C" void ps4_heap_set_checks(int on);  // port/ps4_runtime.cpp
void start_watchdog();  // below
void start_profile_window();

}  // namespace

// The app's start: system modules, the working folder (PS4_DATA_DIR), env.txt and the PS4 defaults. Called first thing
// in the app's main (app_main).
extern "C" void ps4_app_startup(void) {
    load_system_modules();
    mkdir(kData, 0777);
    for (const char* sub : {"gpu", "mesa"}) mkdir((std::string(kData) + "/" + sub).c_str(), 0777);
    if (chdir(kData) != 0) trace("startup: cannot enter %s", kData);
    {
        char cwd[256] = "?";
        if (!getcwd(cwd, sizeof cwd)) snprintf(cwd, sizeof cwd, "(getcwd failed)");
        const bool relative = access("boot-trace.log", F_OK) == 0;
        trace("startup: working folder %s, relative paths %s", cwd, relative ? "work" : "DO NOT work");
    }
    load_env_txt();
    // the heap's control-block check on every malloc/free (port/ps4_runtime.cpp): SOHVK_HEAP_CHECK=1
    if (const char* h = getenv("SOHVK_HEAP_CHECK"); h && !strcmp(h, "1")) {
        ps4_heap_set_checks(1);
        trace("startup: heap checks on");
    }
    setenv("SDL_VIDEO_DRIVER", "ps5", 0);  // the console drivers keep the PS5 port's names (ps4/sdl)
    setenv("SDL_AUDIO_DRIVER", "ps5", 0);
    // RADV's shader cache, kept between starts: the driver's compiler is slow on this CPU
    setenv("MESA_SHADER_CACHE_DIR", PS4_DATA_DIR "/mesa", 0);
    setenv("MESA_DISK_CACHE_SINGLE_FILE", "1", 0);
    // RADV's flight recorder (every GPU submission in PS4_DATA_DIR/gpu/flight): SOHVK_GPU_TRACE=1 in env.txt
    if (const char* g = getenv("SOHVK_GPU_TRACE"); g && !strcmp(g, "1")) setenv("RADV_PS4_TRACE", "1", 0);
    start_watchdog();
    start_profile_window();
}

// The startup watchdog. 0.1.0 and 0.1.1 stopped in the Vulkan driver's start with the console frozen and nothing
// after it in the logs: whether the app was stuck (and where) or the whole system froze could not be told. Until the
// game has shown 120 frames, every second this thread
//   - puts every file the app writes on the disk (fsync: boot-trace.log, captures/wwhd.log, gpu/radv-ps4.log and the
//     flight recorder's files): what the kernel still held was lost when the console was switched off;
//   - once startup stops advancing (no boot-trace line for 5 s), writes a line every 5 s: lines that keep coming
//     mean the app is stuck but the system runs; lines that stop mean the console itself froze at that time;
//   - after 20 s and again after 60 s without progress, writes every thread's stack to /data/sohvk/hang.log
//     (symbolize with llvm-symbolizer --obj=wwhd.elf, the Maker's output folder), and a short line here.
extern "C" volatile unsigned ps4_trace_lines;     // port/ps4_crashlog.cpp
extern "C" volatile unsigned ps4_wsi_presents;    // port/ps4_vk_wsi.cpp
extern "C" void ps4_dump_thread_stacks(const char* path, const char* reason);  // port/ps4_sampler.cpp
#include <pthread.h>
#include <time.h>

namespace {
double seconds_now() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return double(ts.tv_sec) + double(ts.tv_nsec) / 1e9;
}

void sync_all_files() {
    // regular files only: the GPU and VideoOut device descriptors of the system libraries are left alone
    struct stat st;
    for (int fd = 3; fd < 1024; fd++)
        if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) fsync(fd);
}

void* watchdog(void*) {
    const double start = seconds_now();
    unsigned seen = ps4_trace_lines;
    double changed = start;
    int dumps = 0;
    char line[256];
    for (int tick = 1;; tick++) {
        const timespec second = {1, 0};
        nanosleep(&second, nullptr);
        sync_all_files();
        const double now = seconds_now();
        if (ps4_wsi_presents >= 120) {
            snprintf(line, sizeof line, "watchdog: %u frames shown, the game runs: watchdog off", ps4_wsi_presents);
            ps4_boot_trace(line);
            sync_all_files();
            return nullptr;
        }
        if (now - start > 30 * 60) return nullptr;
        if (ps4_trace_lines != seen) {
            seen = ps4_trace_lines;
            changed = now;
            continue;
        }
        const double idle = now - changed;
        if ((dumps == 0 && idle >= 20) || (dumps == 1 && idle >= 60)) {
            dumps++;
            snprintf(line, sizeof line, "no startup progress for %.0f s (%u frames shown)", idle, ps4_wsi_presents);
            ps4_dump_thread_stacks(PS4_DATA_DIR "/hang.log", line);
            snprintf(line, sizeof line, "watchdog: no progress for %.0f s: every thread's stack in hang.log (%d)",
                     idle, dumps);
            ps4_boot_trace(line);
        } else if (idle >= 5 && tick % 5 == 0) {
            snprintf(line, sizeof line, "watchdog: alive, %.0f s since the last step, %u frames shown", idle,
                     ps4_wsi_presents);
            ps4_boot_trace(line);
        } else {
            continue;
        }
        seen = ps4_trace_lines;  // its own lines are not progress
        sync_all_files();
    }
}

// The sampling profiler (port/ps4_sampler.cpp) for one minute, from 45 s after the start (past the loading): every
// thread's busiest code every 10 s in /data/sohvk/samples.log, named with wwhd-symbols.txt (the Maker's output
// folder). 0.1.5 ran at 2-3 frames a second with the game's sound normal: where the time goes. SOHVK_PS4_PROFILE=0
// turns it off, =<s> starts it after that many seconds instead.
extern "C" void ps4_sampler_start();
extern "C" void ps4_sampler_set_enabled(int on);

// Sleeps until `seconds` have passed: this thread is sampled too (every thread is), and the profiler's signal ends a
// plain nanosleep at once (0.1.6 stopped the profiler the moment it started).
void sleep_through_signals(double seconds) {
    const double end = seconds_now() + seconds;
    for (double left; (left = end - seconds_now()) > 0;) {
        const timespec t = {time_t(left), long((left - double(time_t(left))) * 1e9)};
        nanosleep(&t, nullptr);
    }
}

void* profile_window(void* p) {
    const long after = reinterpret_cast<long>(p);
    sleep_through_signals(double(after));
    ps4_sampler_start();
    sleep_through_signals(60);
    ps4_sampler_set_enabled(0);
    ps4_boot_trace("sampler: one minute recorded in samples.log, stopped");
    return nullptr;
}

void start_profile_window() {
    // off since 0.1.10 (its two profiles are in; its signals skew the renderer's own report below):
    // SOHVK_PS4_PROFILE=1 (from 45 s) or =<s>
    const char* e = getenv("SOHVK_PS4_PROFILE");
    if (!e || !strcmp(e, "0")) return;
    const long after = atol(e) > 1 ? atol(e) : 45;
    pthread_t t;
    if (pthread_create(&t, nullptr, profile_window, reinterpret_cast<void*>(after)) == 0) pthread_detach(t);
}

void start_watchdog() {
    if (const char* off = getenv("SOHVK_WATCHDOG"); off && !strcmp(off, "0")) return;
    pthread_t t;
    if (pthread_create(&t, nullptr, watchdog, nullptr) == 0) pthread_detach(t);
    else ps4_boot_trace("watchdog: no thread");
}
}  // namespace

// port/ps4_vk_wsi.cpp's lines (swapchain, flips): into boot-trace.log
namespace wwhd::ps4 {
void wsi_log(const char* line) { ps4_boot_trace(line); }
}  // namespace wwhd::ps4

// The process entry: the game's main() (main.cpp, renamed app_main on the PS4) on a thread with a 32 MiB stack. The
// PS4's main thread has a small one (the process parameters of port/ps4_crt1.S leave the size to the system), and
// that thread runs the renderer: shader translation (Cemu's decompiler, glslang) recurses deeply.
#include <pthread.h>

extern "C" int app_main(int argc, char** argv);

namespace {
struct MainArgs {
    int argc;
    char** argv;
    int result;
};
void* run_main(void* p) {
    auto* a = static_cast<MainArgs*>(p);
    a->result = app_main(a->argc, a->argv);
    return nullptr;
}
}  // namespace

int main(int argc, char** argv) {
    static char name[] = "eboot.bin";
    static char* fallback[] = {name, nullptr};
    MainArgs args{argc > 0 ? argc : 1, argc > 0 && argv ? argv : fallback, 0};
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32u << 20);
    pthread_t t;
    if (pthread_create(&t, &attr, run_main, &args) != 0) {
        ps4_boot_trace("startup: no thread for the game's main (running it on the system's main thread)");
        return app_main(args.argc, args.argv);
    }
    pthread_attr_destroy(&attr);
    pthread_join(t, nullptr);
    return args.result;
}
