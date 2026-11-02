#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <cmath>
#include <linux/input.h>
#include "imgui.h"
#include "Touch.hpp"

#define maxE 5
#define maxF 10

namespace touch {

struct touchObj {
    bool isDown = false;
    int x = 0;
    int y = 0;
    int id = 0;
};

struct targ {
    int fdNum;
    float S2TX;
    float S2TY;
};

static uint32_t orientation = 0;
static float screenHeight = 0, screenWidth = 0;
static targ targF[maxE];
static touchObj Finger[maxE][maxF];
static int fdNum = 0, origfd[maxE];
static float scale_x = 1.0f, scale_y = 1.0f;
static bool Touch_initialized = false;
static int screenX = 0, screenY = 0;

static bool checkDeviceIsTouch(int fd) {
    uint8_t *bits = NULL;
    ssize_t bits_size = 0;
    int res, j, k;
    bool itmp = false, itmp2 = false, itmp3 = false;
    struct input_absinfo abs{};
    while (true) {
        res = ioctl(fd, EVIOCGBIT(EV_ABS, bits_size), bits);
        if (res < bits_size)
            break;
        bits_size = res + 16;
        bits = (uint8_t *) realloc(bits, bits_size * 2);
    }
    for (j = 0; j < res; j++) {
        for (k = 0; k < 8; k++) {
            if ((bits[j] & (1 << k)) && ioctl(fd, EVIOCGABS(j * 8 + k), &abs) == 0) {
                if (j * 8 + k == ABS_MT_SLOT) {
                    itmp = true;
                    continue;
                }
                if (j * 8 + k == ABS_MT_POSITION_X) {
                    itmp2 = true;
                    continue;
                }
                if (j * 8 + k == ABS_MT_POSITION_Y) {
                    itmp3 = true;
                    continue;
                }
            }
        }
    }
    free(bits);
    return itmp2 && itmp3;
}

static void *TypeA(void *arg) {
    targ tmp = *(targ *) arg;
    int i = tmp.fdNum;
    float S2TX = tmp.S2TX;
    float S2TY = tmp.S2TY;
    int latest = 0;
    input_event inputEvent[64]{0};

    while (Touch_initialized) {
        auto readSize = (int32_t) read(origfd[i], inputEvent, sizeof(inputEvent));
        if (readSize <= 0 || (readSize % sizeof(input_event)) != 0) {
            continue;
        }
        size_t count = size_t(readSize) / sizeof(input_event);
        for (size_t j = 0; j < count; j++) {
            input_event &ie = inputEvent[j];
            if (ie.type == EV_ABS) {
                if (ie.code == ABS_MT_SLOT) {
                    if (ie.value >= 0 && ie.value < maxF)
                        latest = ie.value;
                    continue;
                }
                if (ie.code == ABS_MT_TRACKING_ID) {
                    if (ie.value == -1) {
                        Finger[i][latest].isDown = false;
                    } else {
                        Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                        Finger[i][latest].isDown = true;
                    }
                    continue;
                }
                if (ie.code == ABS_MT_POSITION_X) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].x = (int) (ie.value * S2TX);
                    continue;
                }
                if (ie.code == ABS_MT_POSITION_Y) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].y = (int) (ie.value * S2TY);
                    continue;
                }
            }
            if (ie.code == SYN_REPORT) {
                ImGuiIO &io = ImGui::GetIO();
                int activeSlot = -1;

                if (Finger[i][latest].isDown) {
                    activeSlot = latest;
                } else {
                    for (int f = 0; f < maxF; f++) {
                        if (Finger[i][f].isDown) {
                            activeSlot = f;
                            break;
                        }
                    }
                }

                if (activeSlot != -1) {
                    float x = (float)Finger[i][activeSlot].x;
                    float y = (float)Finger[i][activeSlot].y;
                    float xt = x / scale_x;
                    float yt = y / scale_y;

                    switch (orientation) {
                        case 1:
                            x = yt;
                            y = screenHeight - xt;
                            break;
                        case 2:
                            x = screenHeight - xt;
                            y = screenWidth - yt;
                            break;
                        case 3:
                            y = xt;
                            x = screenWidth - yt;
                            break;
                        default:
                            x = xt;
                            y = yt;
                            break;
                    }
                    io.MousePos = ImVec2(x, y);
                    io.MouseDown[0] = true;
                } else {
                    io.MouseDown[0] = false;
                }
            }
        }
    }
    return nullptr;
}

static void recalcScale() {
    if (orientation == 1 || orientation == 3) {
        scale_x = (float) screenX / screenHeight;
        scale_y = (float) screenY / screenWidth;
    } else {
        scale_x = (float) screenX / screenWidth;
        scale_y = (float) screenY / screenHeight;
    }
}

bool init(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
    if (Touch_initialized) {
        update(_screen_w, _screen_h, _orientation);
        return true;
    }

    char temp[128];
    DIR *dir = opendir("/dev/input/");
    if (!dir) return false;

    dirent *ptr = nullptr;
    int eventCount = 0;
    while ((ptr = readdir(dir)) != nullptr) {
        if (strstr(ptr->d_name, "event"))
            eventCount++;
    }
    closedir(dir);

    struct input_absinfo absX[maxE]{}, absY[maxE]{};
    int fd;
    int minCnt = eventCount + 1;
    fdNum = 0;

    for (int i = 0; i <= eventCount; i++) {
        sprintf(temp, "/dev/input/event%d", i);
        fd = open(temp, O_RDWR);
        if (fd < 0) {
            fd = open(temp, O_RDONLY);
        }
        if (fd < 0) continue;

        if (checkDeviceIsTouch(fd)) {
            int tmp1 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX[fdNum]);
            int tmp2 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY[fdNum]);
            if (tmp1 == 0 && tmp2 == 0) {
                origfd[fdNum] = fd;
                if (i < minCnt) {
                    screenX = absX[fdNum].maximum;
                    screenY = absY[fdNum].maximum;
                    minCnt = i;
                }
                fdNum++;
                if (fdNum >= maxE)
                    break;
            } else {
                close(fd);
            }
        } else {
            close(fd);
        }
    }

    if (minCnt > eventCount || fdNum == 0) {
        return false;
    }

    Touch_initialized = true;
    screenWidth = (float)_screen_w;
    screenHeight = (float)_screen_h;
    orientation = (uint32_t)_orientation;
    recalcScale();

    pthread_t t;
    for (int i = 0; i < fdNum; i++) {
        targF[i].fdNum = i;
        targF[i].S2TX = (float) screenX / (float) absX[i].maximum;
        targF[i].S2TY = (float) screenY / (float) absY[i].maximum;
        pthread_create(&t, nullptr, TypeA, &targF[i]);
        pthread_detach(t);
    }

    return true;
}

void update(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
    screenWidth = (float)_screen_w;
    screenHeight = (float)_screen_h;
    orientation = (uint32_t)_orientation;
    recalcScale();
}

void updateOrientation(uint8_t _orientation) {
    orientation = (uint32_t)_orientation;
    recalcScale();
}

void shutdown() {
    if (Touch_initialized) {
        Touch_initialized = false;
        for (int i = 0; i < fdNum; ++i) {
            if (origfd[i] > 0) {
                close(origfd[i]);
                origfd[i] = 0;
            }
        }
        fdNum = 0;
        memset(Finger, 0, sizeof(Finger));
    }
}

} // namespace touch
