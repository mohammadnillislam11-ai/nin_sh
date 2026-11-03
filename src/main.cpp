// ZqwClient — ninmods PROJECT
#include "Android_draw/draw.h"
#include "ui/styles/styles.h"
#include "ui/menu.hpp"
#include "other/memory.hpp"
#include "game/data.hpp"
#include "game/AimSilent.h"
#include "game/Offsets.h"
#include "func/esp.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <android/log.h>
#include <iostream>
#include <string>

#define LOG_TAG "ninmods"
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

// ═══════════════════════════════════════════════════════════
// BYPASS
// ═══════════════════════════════════════════════════════════
extern "C" void init_bypass();
extern float global_anim_time;

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
// BANNER
// ═══════════════════════════════════════════════════════════
static void print_banner() {
    printf("\n");
    printf(FG_BRED BOLD);
    printf("  ███╗   ██╗██╗███╗   ██╗███╗   ███╗ ██████╗ ██████╗ ███████╗\n");
    printf("  ████╗  ██║██║████╗  ██║████╗ ████║██╔═══██╗██╔══██╗██╔════╝\n");
    printf("  ██╔██╗ ██║██║██╔██╗ ██║██╔████╔██║██║   ██║██║  ██║███████╗\n");
    printf("  ██║╚██╗██║██║██║╚██╗██║██║╚██╔╝██║██║   ██║██║  ██║╚════██║\n");
    printf("  ██║ ╚████║██║██║ ╚████║██║ ╚═╝ ██║╚██████╔╝██████╔╝███████║\n");
    printf("  ╚═╝  ╚═══╝╚═╝╚═╝  ╚═══╝╚═╝     ╚═╝ ╚═════╝ ╚═════╝ ╚══════╝\n");
    printf(RST);
    printf(FG_BCYAN BOLD "  ╔═══════════════════════════════════════════════╗\n" RST);
    printf(FG_BCYAN "  ║" RST FG_BYELLOW BOLD "        ninmods PROJECT — FREE FIRE         " RST FG_BCYAN "║\n" RST);
    printf(FG_BCYAN "  ║" RST FG_BGREEN "              v1.0 VIP EDITION                 " RST FG_BCYAN "║\n" RST);
    printf(FG_BCYAN "  ╚═══════════════════════════════════════════════╝\n" RST);
    printf("\n");
}

// ═══════════════════════════════════════════════════════════
// START OVERLAY
// ═══════════════════════════════════════════════════════════
static int start_overlay() {
    LOGI("=== ninmods starting ===");

    // ─── INIT GUI ───
    printf(FG_BBLUE "  [1/3] " RST FG_BWHITE "Init GUI...\n" RST);

    screen_config();

    int max_size = (displayInfo.height > displayInfo.width)
                   ? displayInfo.height : displayInfo.width;
    int min_size = (displayInfo.height < displayInfo.width)
                   ? displayInfo.height : displayInfo.width;

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);
    native_window_screen_x = max_size;
    native_window_screen_y = min_size;

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        printf(FG_BRED "  [!] initGUI FAILED!\n" RST);
        return -1;
    }
    printf(FG_BGREEN "  [✓] GUI ready\n" RST);

    Touch_Init(displayInfo.width, displayInfo.height, displayInfo.orientation, true);

    // ─── BYPASS ───
    printf(FG_BBLUE "  [2/3] " RST FG_BWHITE "Init bypass...\n" RST);
    init_bypass();

    // ─── ATTACH KE GAME (MANUAL OPEN) ───
    if (wait_for_game() <= 0) {
        printf(FG_BRED "  [!] Failed to detect game process\n" RST);
        return -1;
    }

    printf(FG_BCYAN "  [2/4] " RST FG_BWHITE "Waiting for libil2cpp.so to load in memory...\n" RST);
    fflush(stdout);

    int waitCount = 0;
    while (true) {
        proc::lib = get_lib();
        if (proc::lib > 0) break;

        // Check if game process exited
        if (get_pid() <= 0) {
            printf(FG_BRED "\n  [!] Game process closed. Waiting for game to re-open...\n" RST);
            while (true) {
                proc::pid = get_pid();
                if (proc::pid > 0) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
            }
            printf(FG_BGREEN "  [✓] Game process re-detected! PID: %d\n" RST, proc::pid);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        waitCount++;
        if (waitCount % 3 == 0) {
            printf(FG_BYELLOW "        ...waiting for libil2cpp.so (%ds)\n" RST, waitCount);
            fflush(stdout);
        }
    }

    printf(FG_BGREEN "  [✓] libil2cpp.so loaded! Base: " FG_BYELLOW "0x%lx\n" RST, (unsigned long)proc::lib);

    // Verify memory read
    uint32_t elfMagic = rpm<uint32_t>(proc::lib);
    if (elfMagic == 0x464C457F) {
        printf(FG_BGREEN "  [✓] Process memory access: OK (ELF magic verified)\n" RST);
    } else {
        printf(FG_BYELLOW "  [!] Warning: Memory read test returned 0x%08X (Check root/SELinux)\n" RST, elfMagic);
    }
    fflush(stdout);

    // ═══════════════════════════════════════════════════════
    //  INIT AIM SILENT HOOK (DEBUG)
    // ═══════════════════════════════════════════════════════
    printf(FG_BBLUE "  [~] " RST FG_BWHITE "Init Aim Silent Hook...\n" RST);

    printf(FG_BYELLOW "  [DEBUG] lib base       = 0x%lx\n" RST, (unsigned long)proc::lib);
    printf(FG_BYELLOW "  [DEBUG] RVA TakeDamage_9 = 0x%lx\n" RST, (unsigned long)Offsets::Player_TakeDamage_9);
    printf(FG_BYELLOW "  [DEBUG] RVA TakeDamage_4 = 0x%lx\n" RST, (unsigned long)Offsets::Player_TakeDamage_4);
    printf(FG_BYELLOW "  [DEBUG] RVA HeadCollider = 0x%lx\n" RST, (unsigned long)Offsets::Player_get_HeadCollider);
    printf(FG_BYELLOW "  [DEBUG] Hook addr 9    = 0x%lx\n" RST, (unsigned long)(proc::lib + Offsets::Player_TakeDamage_9));
    printf(FG_BYELLOW "  [DEBUG] Hook addr 4    = 0x%lx\n" RST, (unsigned long)(proc::lib + Offsets::Player_TakeDamage_4));
    printf(FG_BYELLOW "  [DEBUG] Hook addr HC   = 0x%lx\n" RST, (unsigned long)(proc::lib + Offsets::Player_get_HeadCollider));
    fflush(stdout);

    InitAimSilentHook();

    printf(FG_BGREEN "  [✓] Aim Silent Hook installed\n" RST);
    printf(FG_BYELLOW "  [DEBUG] AimSilentEnabled = %d\n" RST, (int)AimSilentEnabled);
    printf(FG_BYELLOW "  [DEBUG] AimSilentNobita  = %d\n" RST, (int)AimSilentNobita);
    fflush(stdout);

    // ─── START DATA THREAD ───
    data::thread_start();
    printf(FG_BGREEN "  [✓] Data scanner thread started\n" RST);
    fflush(stdout);

    printf(FG_BGREEN BOLD "  ⭐ Overlay & Scanner Active! ⭐\n" RST);
    printf(FG_BCYAN "  [INFO] Real-time scanner logs will display below:\n" RST);
    fflush(stdout);

    // ═══════════════════════════════════════════════════════
    //  MAIN LOOP — MENU CHEAT + LIVE LOGS
    // ═══════════════════════════════════════════════════════
    int dbgCounter = 0;
    while (true) {
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
    return 0;
}

// ═══════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════
int main(int argc, char** argv) {
    print_banner();

    if (argc >= 2) {
        if (strcmp(argv[1], "ffmax") == 0) {
            g_targetGame = GAME_FF_MAX;
            printf(FG_BGREEN "  [+] Target: " FG_BYELLOW "Free Fire MAX\n" RST);
        }
    }

    return start_overlay();
}
