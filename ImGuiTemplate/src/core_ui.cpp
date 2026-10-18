#include "main.h"
#include "core_ui.h"

/* user resources start */
// OWN vars
char buf[500] = "";
ImGuiTextBuffer logBuf;
bool ScrollToBottom = false;
static bool show_demo_window = true;
static bool show_another_window = false;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

// OWN funcs
void AddGuiLog(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logBuf.appendfv(fmt, args);
    logBuf.appendfv("\n", args);
    va_end(args);
    ScrollToBottom = true;
}
/* user resources end */

// ImGUI core code
namespace
{
    // ImGUI vars
    ImGuiContext* g_imguiCtx = NULL;
    bool g_bImGuiInitialized = false;
    float g_UIScale = 2.5f;
    ImVec2 displaySize;
    ImVec2 zeroVec(0,0);
    static float flScaleX, flScaleY;
    static int nDisplayX, nDisplayY;
    
    // ImGui core functions
    void BuildBase()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(g_UIScale);
        ImGui::StyleColorsDark();

        ImFontConfig config;
        config.SizePixels = roundf(13.0f * g_UIScale);
        io.Fonts->AddFontDefault(&config);
    }

    void RenderBase()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowBgAlpha(0.70f);
        ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x * 0.23f, io.DisplaySize.y * 0.36f), ImVec2(io.DisplaySize.x, io.DisplaySize.y));

        ImGui::Begin("ImGUI Template", NULL, ImGuiWindowFlags_None | ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("ImGUI FPS: %.2f", io.Framerate);
        ImGui::Text("InGUI DrawCalls: %d", TotalDrawCalls);
        ImGui::BeginChild("Scrolling", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(logBuf.begin());
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ScrollToBottom)
        {
            ImGui::SetScrollHereY(1.00f);
            ScrollToBottom = false;
        }
        ImGui::EndChild();
        
        if (ImGui::Button("Send") && buf[0] != '\0')
        {
            AddGuiLog("host: %s", buf);
            memset(buf, 0, sizeof(buf));
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Clear")) logBuf.clear();
        
        ImGui::SameLine();
        ImGui::PushItemWidth(io.DisplaySize.x * 0.15f);
        ImGui::InputText("", buf, IM_ARRAYSIZE(buf));
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        ImGui::Text("%d/%d max", (int)std::strlen(buf), (int)sizeof(buf));

        ImGui::End();
    }
}
void InitUI()
{
    if (g_bImGuiInitialized || g_imguiCtx) return;
    g_imguiCtx = ImGui::CreateContext();
    ImGui_ImplRenderWare_Init();
    nDisplayX = RsGlobal.maximumWidth;
    nDisplayY = RsGlobal.maximumHeight;
    flScaleX = nDisplayY * 0.00052083333f;
    flScaleY = nDisplayY * 0.00092592592f;
    displaySize.x = nDisplayX;
    displaySize.y = nDisplayY;
    BuildBase();
    g_bImGuiInitialized = true;
}
void RenderUI()
{
    if (!g_bImGuiInitialized || !g_imguiCtx) return;
    ImGui_ImplRenderWare_NewFrame(); 
    ImGui::NewFrame();
    RenderBase();
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());
}
void ShutdownUI()
{
    if (!g_bImGuiInitialized || !g_imguiCtx) return;
    g_bImGuiInitialized = false;
    ImGui_ImplRenderWare_ShutDown();
    ImGui::DestroyContext(g_imguiCtx);
    g_imguiCtx = NULL;
}
bool IsTouchedOverUI(float x, float y)
{
    if (!g_imguiCtx) return false;
    for (int i = g_imguiCtx->Windows.Size - 1; i >= 0; --i)
    {
        ImGuiWindow* win = g_imguiCtx->Windows[i];
        if (!win || !win->Active || win->Hidden) continue;
        if (win->Flags & (ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMouseInputs)) continue;
        const ImVec2 p = win->Pos;
        const ImVec2 s = win->Size;
        if (x >= p.x && x < p.x + s.x && y >= p.y && y < p.y + s.y) return true;
    }
    return false;
}
bool IsInitialized()
{
    return g_bImGuiInitialized;
}
