#pragma once

#include <backends/imgui_impl_renderware.h>
#include <imgui_internal.h>

void InitUI();
void RenderUI();
void ShutdownUI();
bool IsTouchedOverUI(float x, float y);
bool IsInitialized();

/* user extern start */
void AddGuiLog(const char* fmt, ...);
/* user extern end */
