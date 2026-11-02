LOCAL_PATH := $(call my-dir)

# ============================================================
# ShadowHook — static
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libshadowhook
LOCAL_SRC_FILES := ../src/shadowhook/libshadowhook.a
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS := optional
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# MAIN MODULE — client.sh
# ============================================================
include $(CLEAR_VARS)

LOCAL_MODULE := client.sh

# ─── SOURCE FILES ───
# Auto-collect semua *.cpp di src/ dan includes/draw/
# Kecuali: imgui_demo, imgui_impl_vulkan, stb_image
LOCAL_SRC_FILES := $(filter-out \
    ../includes/draw/ImGui/imgui_demo.cpp \
    ../includes/draw/ImGui/backends/imgui_impl_vulkan.cpp \
    ../includes/draw/ImGui/stb_image.cpp \
    ../src/shadowhook/libshadowhook.a, \
    $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH)/../src -name "*.cpp")) \
    $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH)/../includes/draw -name "*.cpp")))

# ─── INCLUDE DIRS ───
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../includes \
    $(LOCAL_PATH)/../includes/font \
    $(LOCAL_PATH)/../includes/internal \
    $(LOCAL_PATH)/../includes/internal/ImGui \
    $(LOCAL_PATH)/../includes/internal/ImGui/backends \
    $(LOCAL_PATH)/../includes/internal/Android_draw \
    $(LOCAL_PATH)/../includes/internal/Android_touch \
    $(LOCAL_PATH)/../includes/internal/native_surface \
    $(LOCAL_PATH)/../includes/draw/ImGui \
    $(LOCAL_PATH)/../includes/draw/ImGui/backends \
    $(LOCAL_PATH)/../includes/protect \
    $(LOCAL_PATH)/../src \
    $(LOCAL_PATH)/../src/ui \
    $(LOCAL_PATH)/../src/ui/png \
    $(LOCAL_PATH)/../src/ui/png/images \
    $(LOCAL_PATH)/../src/ui/styles \
    $(LOCAL_PATH)/../src/ui/widgets \
    $(LOCAL_PATH)/../src/func \
    $(LOCAL_PATH)/../src/game \
    $(LOCAL_PATH)/../src/other \
    $(LOCAL_PATH)/../src/protect \
    $(LOCAL_PATH)/../src/shadowhook

# ═══════════════════════════════════════════════════════════
#  C++ FLAGS
# ═══════════════════════════════════════════════════════════
LOCAL_CPPFLAGS := \
    -std=c++17 \
    -fexceptions \
    -frtti \
    -fvisibility=hidden \
    -fvisibility-inlines-hidden \
    -Oz \
    -ffunction-sections \
    -fdata-sections \
    -fomit-frame-pointer \
    -Wno-error=format-security \
    -fno-color-diagnostics \
    -fmerge-all-constants \
    -fno-ident

# ─── LINKER FLAGS ───
LOCAL_LDFLAGS := \
    -Wl,--gc-sections \
    -Wl,--strip-all \
    -Wl,--build-id=none \
    -Wl,--no-undefined \
    -pie \
    -Wl,-z,relro \
    -Wl,-z,now \
    -Wl,-z,noexecstack

# ─── STATIC LIBS ───
LOCAL_STATIC_LIBRARIES := \
    libshadowhook

# ─── SYSTEM LIBS ───
LOCAL_LDLIBS := \
    -llog \
    -landroid \
    -lEGL \
    -lGLESv3 \
    -lz \
    -ldl

include $(BUILD_EXECUTABLE)
