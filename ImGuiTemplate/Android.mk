LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    LOCAL_MODULE := ImGuiTemplate
else
    LOCAL_MODULE := ImGuiTemplate64
endif
LOCAL_SRC_FILES := main.cpp mod/logger.cpp src/core_ui.cpp
LOCAL_SRC_FILES += imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp
LOCAL_SRC_FILES += backends/imgui_impl_renderware.cpp
LOCAL_CXXFLAGS += -O2 -DNDEBUG -std=c++17
LOCAL_LDLIBS += -llog
include $(BUILD_SHARED_LIBRARY)
