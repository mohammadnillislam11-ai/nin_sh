// ZqwClient — ninmods PROJECT
#include "Android_draw/draw.h"
#include "ui/menu.hpp"
#include "protect/license.hpp"
#include "other/memory.hpp"
#include "game/data.hpp"
#include "game/Offsets.h"
#include "func/esp.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <android/log.h>
#include <iostream>
#include <string>
#include <atomic>

#define LOG_TAG "gemini"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

// ═══════════════════════════════════════════════════════════
// ANSI COLOR
// ═══════════════════════════════════════════════════════════
#define RST        "\033[0m"
#define BOLD       "\033[1m"
#define FG_BRED    "\033[91m"
#define FG_BGREEN  "\033[92m"
#define FG_BYELLOW "\033[93m"
#define FG_BBLUE   "\033[94m"
#define FG_BCYAN   "\033[96m"
#define FG_BWHITE  "\033[97m"

extern float global_anim_time;
std::atomic<bool> main_thread_flag{true};

// ═══════════════════════════════════════════════════════════
// TARGET GAME
// ═══════════════════════════════════════════════════════════
enum TargetGame {
    GAME_FF     = 0,
    GAME_FF_MAX = 1
};

static TargetGame g_targetGame = GAME_FF;

static const char* kFFActivity    = "com.dts.freefireth/com.dts.freefireth.FFMainActivity";
static const char* kFFMaxActivity = "com.dts.freefiremax/com.dts.freefiremax.FFMainActivity";
static const char* kFFPackage     = "com.dts.freefireth";
static const char* kFFMaxPackage  = "com.dts.freefiremax";

static const char* GetTargetActivity() {
    return (g_targetGame == GAME_FF) ? kFFActivity : kFFMaxActivity;
}
static const char* GetTargetPackage() {
    return (g_targetGame == GAME_FF) ? kFFPackage : kFFMaxPackage;
}

// ═══════════════════════════════════════════════════════════
// WAIT FOR GAME (MANUAL LAUNCH)
// ═══════════════════════════════════════════════════════════
static int wait_for_game() {
    printf(FG_BCYAN "\n  [1/4] " RST FG_BWHITE "Waiting for Free Fire to open manually...\n" RST);
    printf(FG_BYELLOW "        Please open Free Fire on your device (terminal will detect automatically).\n" RST);
    fflush(stdout);

    while (true) {
        proc::pid = get_pid();
        if (proc::pid > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    printf(FG_BGREEN "  [✓] Game process detected! PID: " FG_BYELLOW "%d\n" RST, proc::pid);
    fflush(stdout);
    return proc::pid;
}

// ═══════════════════════════════════════════════════════════
// VIP DASHBOARD & LICENSE SECURITY GATE
// ═══════════════════════════════════════════════════════════
static void print_dashboard() {
    printf(FG_BCYAN BOLD "\n  ╔═══════════════════════════════════════════════════════════╗\n" RST);
    printf(FG_BCYAN BOLD "  ║" FG_BWHITE BOLD "              ninmods External Panel | VIP                 " FG_BCYAN BOLD "║\n" RST);
    printf(FG_BCYAN BOLD "  ╠═══════════════════════════════════════════════════════════╣\n" RST);
    printf(FG_BCYAN BOLD "  ║" FG_BWHITE "  Target Game    : " FG_BYELLOW "%-41s" FG_BCYAN BOLD "║\n" RST,
           (g_targetGame == GAME_FF) ? "Free Fire (com.dts.freefireth)" : "Free Fire MAX (com.dts.freefiremax)");
    printf(FG_BCYAN BOLD "  ║" FG_BWHITE "  License Status : " FG_BGREEN BOLD "%-41s" FG_BCYAN BOLD "║\n" RST,
           lic::status_text());
    printf(FG_BCYAN BOLD "  ║" FG_BWHITE "  Active Window  : " FG_BWHITE "%-41s" FG_BCYAN BOLD "║\n" RST,
           "2026-11-04 to 2026-11-06 (UTC)");
    printf(FG_BCYAN BOLD "  ║" FG_BWHITE "  Expiry Date    : " FG_BYELLOW "%-41s" FG_BCYAN BOLD "║\n" RST,
           "2026-11-07 00:00:00 UTC");
    printf(FG_BCYAN BOLD "  ║" FG_BWHITE "  Official TG    : " FG_BCYAN "%-41s" FG_BCYAN BOLD "║\n" RST,
           "https://t.me/ninmods");
    printf(FG_BCYAN BOLD "  ╚═══════════════════════════════════════════════════════════╝\n\n" RST);
    fflush(stdout);
}

static bool check_license_gate() {
    lic::init();

    printf(FG_BCYAN BOLD "\n  ═══════════════════════════════════════════════════════════\n" RST);
    printf(FG_BWHITE BOLD "               ninmods VIP — Security Gate\n" RST);
    printf(FG_BCYAN BOLD "  ═══════════════════════════════════════════════════════════\n" RST);

    time_t now = time(nullptr);
    time_t t_start = lic::start_date();
    time_t t_exp   = lic::expiry_date();

    if (now < t_start) {
        printf(FG_BYELLOW BOLD "\n  [!] STATUS: COMING SOON\n" RST);
        printf(FG_BWHITE "      Access opens on: 2026-11-04 00:00:00 UTC\n" RST);
        printf(FG_BCYAN "      Official Telegram: https://t.me/ninmods\n\n" RST);
        return false;
    }

    if (now >= t_exp) {
        printf(FG_BRED BOLD "\n  [!] STATUS: EXPIRED\n" RST);
        printf(FG_BWHITE "      Access window closed on: 2026-11-07 00:00:00 UTC\n" RST);
        printf(FG_BCYAN "      Official Telegram: https://t.me/ninmods\n\n" RST);
    }

    // Always prompt for key in terminal
    printf(FG_BWHITE "\n  [?] Enter License Key: " RST);
    fflush(stdout);

    char key_input[128] = {0};
    if (fgets(key_input, sizeof(key_input), stdin)) {
        size_t len = strlen(key_input);
        while (len > 0 && (key_input[len - 1] == '\n' || key_input[len - 1] == '\r' || key_input[len - 1] == ' ' || key_input[len - 1] == '\t')) {
            key_input[len - 1] = '\0';
            len--;
        }
    }

    bool key_ok = false;
    if (strlen(key_input) > 0) {
        key_ok = lic::set_key(key_input);
    } else if (lic::is_valid()) {
        key_ok = true;
    }

    if (!key_ok || !lic::is_valid()) {
        if (now >= t_exp) {
            printf(FG_BRED "\n  [-] License has expired!\n" RST);
            printf(FG_BCYAN "  [*] For renewals & updates: " FG_BYELLOW "https://t.me/ninmods\n\n" RST);
        } else {
            printf(FG_BRED "\n  [-] Authentication failed: Invalid license key.\n" RST);
            printf(FG_BCYAN "  [*] Official Telegram: " FG_BYELLOW "https://t.me/ninmods\n\n" RST);
        }
        return false;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════
// START OVERLAY
// ═══════════════════════════════════════════════════════════
static int start_overlay() {
    LOGI("=== ninmods starting ===");

    screen_config();

    int max_size = (displayInfo.height > displayInfo.width)
                   ? displayInfo.height : displayInfo.width;
    int min_size = (displayInfo.height < displayInfo.width)
                   ? displayInfo.height : displayInfo.width;

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);
    native_window_screen_x = max_size;
    native_window_screen_y = min_size;

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true, false)) {
        printf(FG_BRED "[-] Failed to initialize overlay GUI\n" RST);
        return -1;
    }

    Touch_Init(displayInfo.width, displayInfo.height, displayInfo.orientation, true);

    // ─── ATTACH KE GAME (MANUAL OPEN) ───
    if (wait_for_game() <= 0) {
        printf(FG_BRED "[-] Game process detection aborted\n" RST);
        return -1;
    }

    printf(FG_BWHITE "[*] Waiting for libil2cpp.so to load...\n" RST);
    fflush(stdout);

    while (true) {
        proc::lib = get_lib();
        if (proc::lib > 0) break;

        if (get_pid() <= 0) {
            printf(FG_BYELLOW "[*] Game closed. Waiting for relaunch...\n" RST);
            while (true) {
                proc::pid = get_pid();
                if (proc::pid > 0) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
            }
            printf(FG_BGREEN "[+] Game re-detected (PID: %d)\n" RST, proc::pid);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    printf(FG_BGREEN "[+] libil2cpp.so loaded: " FG_BYELLOW "0x%lx\n" RST, (unsigned long)proc::lib);

    // Verify memory read
    uint32_t elfMagic = rpm<uint32_t>(proc::lib);
    if (elfMagic == 0x464C457F) {
        printf(FG_BGREEN "[+] Process memory read: OK\n" RST);
    } else {
        printf(FG_BYELLOW "[!] Warning: Memory read returned 0x%08X (Check root permissions)\n" RST, elfMagic);
    }
    fflush(stdout);

    // Start background scanner
    data::thread_start();
    printf(FG_BGREEN "[+] Overlay & ESP ready. Menu active.\n\n" RST);
    fflush(stdout);

    // ═══════════════════════════════════════════════════════
    //  MAIN LOOP — MENU CHEAT + LIVE LOGS
    // ═══════════════════════════════════════════════════════
    int dbgCounter = 0;
    while (main_thread_flag) {
        global_anim_time += 0.016f;
        drawBegin();
        ui::menu::render();
        drawEnd();
        usleep(16000);

        // ─── REAL-TIME SCANNER LOG (Every ~3 seconds at 60 FPS) ───
        if (++dbgCounter >= 180) {
            dbgCounter = 0;

            bool inM = data::inMatch.load(std::memory_order_relaxed);
            size_t nPlayers = 0;
            {
                std::lock_guard<std::mutex> lk(data::players_mutex);
                nPlayers = data::players.size();
            }

            if (inM) {
                printf(FG_BGREEN "[MATCH ACTIVE] " RST
                       FG_BWHITE "Enemies: " FG_BYELLOW "%zu" RST
                       FG_BWHITE " | LocalPlayer: " FG_BCYAN "0x%lx" RST
                       FG_BWHITE " | Tracer: " FG_BGREEN "%s" RST
                       FG_BWHITE " | Box: " FG_BGREEN "%s" RST "\n",
                       nPlayers,
                       (unsigned long)data::localPlayerAddr.load(std::memory_order_relaxed),
                       cfg::esp::tracer ? "ON" : "OFF",
                       cfg::esp::box ? "ON" : "OFF");
            } else {
                printf(FG_BYELLOW "[LOBBY] " RST
                       FG_BWHITE "Waiting for Match | Lib: " FG_BCYAN "0x%lx" RST
                       FG_BWHITE " | PID: " FG_BYELLOW "%d" RST
                       FG_BWHITE " | Tracer: " FG_BWHITE "%s" RST "\n",
                       (unsigned long)proc::lib,
                       proc::pid,
                       cfg::esp::tracer ? "ON" : "OFF");
            }
            fflush(stdout);
        }
    }

    shutdown();
    Touch_Close();
    data::thread_stop();
    return 0;
}

// ═══════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════
static void mask_process_name(int argc, char** argv, const char* fake_name) {
    if (!argv || !argv[0] || !fake_name) return;

    // 1. Update /proc/self/comm via prctl (max 16 chars)
    prctl(PR_SET_NAME, fake_name, 0, 0, 0);

    // 2. Overwrite /proc/self/cmdline via argv[0] memory space
    // Determine contiguous length of original command line arguments
    size_t total_len = 0;
    for (int i = 0; i < argc; ++i) {
        if (argv[i]) {
            total_len += strlen(argv[i]) + 1;
        }
    }

    if (total_len > 0) {
        memset(argv[0], 0, total_len);
        strncpy(argv[0], fake_name, total_len - 1);
    }
}

int main(int argc, char** argv) {
    // Mask process name immediately upon entry to gemini-cli
    mask_process_name(argc, argv, "gemini-cli");

    if (argc >= 2) {
        if (strcmp(argv[1], "ffmax") == 0) {
            g_targetGame = GAME_FF_MAX;
        }
    }

    if (!check_license_gate()) {
        return 1;
    }

    print_dashboard();

    return start_overlay();
}
