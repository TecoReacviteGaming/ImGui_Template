#include <mod/amlmod.h>
#include <mod/logger.h>
#include <sautils.h>
#include <main.h>
#include "src/core_ui.h"
#include <aml-psdk/game_sa/Events.h>
#include <aml-psdk/game_sa/entity/Camera.h>
#include <aml-psdk/game_sa/base/Timer.h>

MYMOD(net.ocornut.teco.imgui, ImGui Template, 1.0, ocornut & TecoReacvite)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.4.0)
    ADD_DEPENDENCY_VER(net.rusjj.gtasa.utils, 1.6)
END_DEPLIST()

// OWN vars
uintptr_t pGame = 0;
void* hGame = nullptr;
ISAUtils* sautils = nullptr;
eLoadedGame curGame = Unknown;

// touch system
static char nClearMousePos = 0;
static bool g_bIgnoredFingers[10] = {false};
static bool g_fingerOnUI[10] = {false};
static uint8_t g_fingerAsMouse = 0xFF;
DECL_HOOKv(AND_TouchEvent, int actionType, int trackNum, int x, int y)
{
    if (!IsInitialized() || trackNum < 0 || trackNum >= 10)
    {
        AND_TouchEvent(actionType, trackNum, x, y);
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    //AddGuiLog("Touch-log: %d trackNum | %d actionType", trackNum, actionType);

    switch (actionType)
    {
        case 1: // touch release
        {
            if (g_fingerAsMouse == trackNum)
            {
                nClearMousePos = 3;
                io.AddMouseButtonEvent(0, false);
                g_fingerAsMouse = 0xFF;
            }
            break;
        }
        
        case 2: // touch push
        {
            const bool bCanUseUI = (CTimer::m_UserPause != true && TheCamera.GetScreenFadeStatus() != 2);
            g_fingerOnUI[trackNum] = bCanUseUI && (io.WantTextInput || IsTouchedOverUI((float)x, (float)y));
            if (g_fingerOnUI[trackNum] && g_fingerAsMouse == 0xFF)
            {
                io.AddMousePosEvent((float)x, (float)y);
                io.AddMouseButtonEvent(0, true);
                g_fingerAsMouse = trackNum;
            }
            break;
        }

        case 3: // touch move
        {
            if (g_fingerAsMouse == trackNum)
                io.AddMousePosEvent((float)x, (float)y);
            break;
        }
    }
    
    const bool swallow = g_fingerOnUI[trackNum];
    if (actionType == 1) g_fingerOnUI[trackNum] = false;
    if (!swallow) AND_TouchEvent(actionType, trackNum, x, y);
}

// keyboard system
void (*OS_KeyboardRequest)(int);
int* NVtoKK;
char *KKtoChar, *KKtoShiftedChar;
DECL_HOOKv(AND_KeyboardEvent, bool pushed, int keyNum, int ctrl_or_shift, int alwaysZero)
{
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput)
    {
        AND_KeyboardEvent(pushed, keyNum, ctrl_or_shift, alwaysZero);
        return;
    }

    static bool bLoggedPreviously = false;
    if (!bLoggedPreviously)
    {
        AddGuiLog("Keyboard-log: %d keyNum", keyNum);
        bLoggedPreviously = true;
    }
    else
    {
        bLoggedPreviously = false;
    }

    switch (keyNum)
    {
        case 3:
            io.AddKeyEvent(ImGuiKey_Enter, pushed);
            break;
        
        case 4:
            io.AddKeyEvent(ImGuiKey_Backspace, pushed);
            break;

        default:
            if (pushed && keyNum >= 0)
            {
                char sym = 0;
                if (ctrl_or_shift <= 0) sym = KKtoChar[NVtoKK[keyNum]];
                else sym = KKtoShiftedChar[NVtoKK[keyNum]];
                if (sym != 0) io.AddInputCharacter(sym);
            }
            break;
    }
}

// ImGui rendering core
static bool g_bLastTextInput = false;
DECL_HOOKv(Render2DStuff)
{
    Render2DStuff();
    RenderUI();
    ImGuiIO& io = ImGui::GetIO();
    if (nClearMousePos > 0 && --nClearMousePos == 0) io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    if (io.WantTextInput != g_bLastTextInput)
    {
        OS_KeyboardRequest(io.WantTextInput ? 1 : 0);
        g_bLastTextInput = io.WantTextInput;
    }
}

// main
ON_MOD_LOAD()
{
    logger->SetTag("ImGui Template");
    sautils = (ISAUtils*)GetInterface("SAUtils");
    if (!sautils)
    {
        logger->Error("SAUtils failed to load!");
        return;
    }
    
    curGame = sautils->GetLoadedGame();
    if (curGame == GTASA_2_00 || curGame == GTASA_2_10)
    {
        pGame = aml->GetLib("libGTASA.so");
        hGame = aml->GetLibHandle("libGTASA.so");
    }
    else
    {
        logger->Error("Unsupported game, expected GTA SA game!");
        return;
    }

    // game vars
    SET_TO(OS_KeyboardRequest, aml->GetSym(hGame, "_Z18OS_KeyboardRequesti"));
    SET_TO(NVtoKK, aml->GetSym(hGame, "NVtoKK"));
    SET_TO(KKtoChar, aml->GetSym(hGame, "KKtoChar"));
    SET_TO(KKtoShiftedChar, aml->GetSym(hGame, "KKtoShiftedChar"));
    
    // hooks/events
    Events::initRwEvent.after += InitUI;
    Events::shutdownRwEvent.before += ShutdownUI;
    HOOK(Render2DStuff, aml->GetSym(hGame, "_Z13Render2dStuffv"));
    HOOK(AND_TouchEvent, aml->GetSym(hGame, "_Z14AND_TouchEventiiii"));
    HOOK(AND_KeyboardEvent, aml->GetSym(hGame, "_Z17AND_KeyboardEventbiib"));
}
