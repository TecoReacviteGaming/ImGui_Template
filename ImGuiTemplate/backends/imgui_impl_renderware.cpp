#include "imgui_impl_renderware.h"
#include "main.h"
#include <new>
#include <cstring>
#include <algorithm>

static_assert(sizeof(ImDrawIdx) == sizeof(RwImVertexIndex), "ImDrawIdx must be 16-bit to match RwImVertexIndex");
RwRaster*     g_FontRaster       = nullptr;
RwIm2DVertex* g_pVB              = nullptr;
unsigned int  g_VertexBufferSize = 0;

static bool EnsureVertexBuffer(unsigned int needed)
{
    if (g_pVB && g_VertexBufferSize >= needed) return true;

    unsigned int newSize = std::max(needed + needed / 2, 5000u);
    RwIm2DVertex* newVB = new (std::nothrow) RwIm2DVertex[newSize];
    if (!newVB) return false;

    delete[] g_pVB;
    g_pVB = newVB;
    g_VertexBufferSize = newSize;
    return true;
}
static inline void SetupRenderState()
{
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,            (void*)0);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,           (void*)0);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,      (void*)1);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,               (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,              (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,              (void*)0);
    RwRenderStateSet(rwRENDERSTATECULLMODE,               (void*)rwCULLMODECULLNONE);
    RwRenderStateSet(rwRENDERSTATEBORDERCOLOR,            (void*)0);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION,      (void*)rwALPHATESTFUNCTIONGREATER);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF,   (void*)2);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,          (void*)rwFILTERLINEAR);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS,         (void*)rwTEXTUREADDRESSCLAMP);
}

bool ImGui_ImplRenderWare_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)RsGlobal.maximumWidth, (float)RsGlobal.maximumHeight);
    return true;
}
void ImGui_ImplRenderWare_ShutDown()
{
    ImGuiIO& io = ImGui::GetIO();
    if (g_FontRaster) RwRasterDestroy(g_FontRaster);
    g_FontRaster = nullptr;
    io.Fonts->TexID = nullptr;

    delete[] g_pVB;
    g_pVB = nullptr;
    g_VertexBufferSize = 0;
}
void ImGui_ImplRenderWare_NewFrame()
{
    if (!g_FontRaster)
        ImGui_ImplRenderWare_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)RsGlobal.maximumWidth, (float)RsGlobal.maximumHeight);
}
void ImGui_ImplRenderWare_RenderDrawData(ImDrawData* draw_data)
{
    if (!draw_data || draw_data->CmdListsCount == 0 || draw_data->TotalVtxCount <= 0) return;
    if (!EnsureVertexBuffer((unsigned int)draw_data->TotalVtxCount)) return;

    const float screenZ      = CSprite2d::NearScreenZ;
    const float recipCameraZ = CSprite2d::RecipNearClip;

    RwIm2DVertex* vtx_dst = g_pVB;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_src  = cmd_list->VtxBuffer.Data;
        const int count            = cmd_list->VtxBuffer.Size;

        for (int i = 0; i < count; i++, vtx_dst++, vtx_src++)
        {
            RwIm2DVertexSetScreenX(vtx_dst, vtx_src->pos.x);
            RwIm2DVertexSetScreenY(vtx_dst, vtx_src->pos.y);
            RwIm2DVertexSetScreenZ(vtx_dst, screenZ);
            RwIm2DVertexSetRecipCameraZ(vtx_dst, recipCameraZ);
            vtx_dst->emissiveColor = vtx_src->col;
            RwIm2DVertexSetU(vtx_dst, vtx_src->uv.x, recipCameraZ);
            RwIm2DVertexSetV(vtx_dst, vtx_src->uv.y, recipCameraZ);
        }
    }

    void* boundTex = (void*)-1;
    CRect lastScissor;
    bool  scissorValid = false;

    auto ResetState = [&]()
    {
        SetupRenderState();
        boundTex     = (void*)-1;
        scissorValid = false;
    };
    ResetState();

    int vtx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx*  idx_base = cmd_list->IdxBuffer.Data;
        const int         vtx_cnt  = cmd_list->VtxBuffer.Size;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            if (pcmd->UserCallback)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ResetState();
                else
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                    ResetState();
                }
                continue;
            }

            if (pcmd->ElemCount == 0 ||
                pcmd->ClipRect.z <= pcmd->ClipRect.x ||
                pcmd->ClipRect.w <= pcmd->ClipRect.y)
                continue;

            CRect scissor;
            scissor.left   = pcmd->ClipRect.x;
            scissor.bottom = pcmd->ClipRect.w;
            scissor.right  = pcmd->ClipRect.z;
            scissor.top    = pcmd->ClipRect.y;
            if (!scissorValid ||
                scissor.left != lastScissor.left   || scissor.right != lastScissor.right ||
                scissor.top  != lastScissor.top    || scissor.bottom != lastScissor.bottom)
            {
                CWidget::SetScissor(scissor);
                lastScissor  = scissor;
                scissorValid = true;
            }

            void* tex = (void*)pcmd->GetTexID();
            if (tex != boundTex)
            {
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, tex);
                boundTex = tex;
            }

            RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,
                &g_pVB[vtx_offset], (RwInt32)vtx_cnt,
                (RwImVertexIndex*)(idx_base + pcmd->IdxOffset), (RwInt32)pcmd->ElemCount);
        }
        vtx_offset += vtx_cnt;
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)0);
    CRect zero;
    zero.left = zero.bottom = zero.right = zero.top = 0;
    CWidget::SetScissor(zero);
}
bool ImGui_ImplRenderWare_CreateDeviceObjects()
{
    if (g_FontRaster) return true;

    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pxs;
    int width, height, bpp;
    io.Fonts->GetTexDataAsRGBA32(&pxs, &width, &height, &bpp);

    if (!pxs || width <= 0 || height <= 0 || bpp != 4) return false;
    RwImage* img = RwImageCreate(width, height, bpp * 8);
    if (!img) return false;
    
    RwImageAllocatePixels(img);
    if (!RwImageAllocatePixels(img) || !img->cpPixels)
    {
        RwImageDestroy(img);
        return false;
    }

    const size_t srcStride = (size_t)width * bpp;
    if ((size_t)img->stride == srcStride)
    {
        memcpy(img->cpPixels, pxs, srcStride * height);
    }
    else
    {
        const size_t rowBytes = std::min(srcStride, (size_t)img->stride);
        RwUInt8* dst = img->cpPixels;
        for (int y = 0; y < height; y++, dst += img->stride)
            memcpy(dst, pxs + srcStride * y, rowBytes);
    }

    RwInt32 w, h, d, flags;
    if (!RwImageFindRasterFormat(img, 4, &w, &h, &d, &flags))
    {
        RwImageDestroy(img);
        return false;
    }
    RwRaster* raster = RwRasterCreate(w, h, d, flags);
    if (!raster) { RwImageDestroy(img); return false; }

    RwRaster* result = RwRasterSetFromImage(raster, img);
    RwImageDestroy(img);
    if (!result) { RwRasterDestroy(raster); return false; }

    g_FontRaster = result;
    io.Fonts->TexID = (ImTextureID)g_FontRaster;
    return true;
}
