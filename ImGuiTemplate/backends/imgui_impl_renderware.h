#pragma once

#include "imgui.h"

bool ImGui_ImplRenderWare_Init();
void ImGui_ImplRenderWare_ShutDown();
void ImGui_ImplRenderWare_NewFrame();
void ImGui_ImplRenderWare_RenderDrawData(ImDrawData* draw_data);
bool ImGui_ImplRenderWare_CreateDeviceObjects();

extern int TotalDrawCalls;
