#pragma once

#include "imgui.h"

#define RwIm2DVertexSetRecipCameraZ(vert, recipz)   ((vert)->rhw = recipz)
#define RwIm2DVertexSetScreenX(vert, scrnx)         ((vert)->x = (scrnx))
#define RwIm2DVertexSetScreenY(vert, scrny)         ((vert)->y = (scrny))
#define RwIm2DVertexSetScreenZ(vert, scrnz)         ((vert)->z = (scrnz))
#define RwIm2DVertexSetU(vert, texU, recipz)        ((vert)->u = (texU))
#define RwIm2DVertexSetV(vert, texV, recipz)        ((vert)->v = (texV))

bool ImGui_ImplRenderWare_Init();
void ImGui_ImplRenderWare_ShutDown();
void ImGui_ImplRenderWare_NewFrame();
void ImGui_ImplRenderWare_RenderDrawData(ImDrawData* draw_data);
bool ImGui_ImplRenderWare_CreateDeviceObjects();
