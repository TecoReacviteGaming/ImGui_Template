#include <mod/amlmod.h>
#include <mod/logger.h>
#include <sautils.h>

// source files
#include "main.h"
#include "src/core_ui.h"

/*
-- define this if you don't want to show ImGui in-menu
#define IMGUI_EXCLUDE_MENU

-- define this if you don't want to show ImGui in-game
#define IMGUI_EXCLUDE_GAME
*/

// AML PSDK
#include "aml-psdk/game_sa/Events.h"
#include "aml-psdk/game_sa/entity/Camera.h"
#include "aml-psdk/game_sa/base/Timer.h"
#include "aml-psdk/game_sa/engine/OS.h"
#include "aml-psdk/game_sa/engine/MobileMenu.h"

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
static char g_nClearMousePos = 0;
static bool g_bIgnoredFingers[10] = {false};
static bool g_bFingerOnUI[10] = {false};
static uint8_t g_bFingerAsMouse = 0xFF;
static bool g_bAllowGuiTouchOnMenu = false;
DECL_HOOKv(TouchEvent, int actionType, int trackNum, int x, int y)
{
    if (!IsInitialized() || trackNum < 0 || trackNum >= 10)
    {
        TouchEvent(actionType, trackNum, x, y);
        return;
    }
    ImGuiIO& io = ImGui::GetIO();

    switch (actionType)
    {
        case 1: // touch release
        {
            if (g_bFingerAsMouse == trackNum)
            {
                g_nClearMousePos = 3;
                io.AddMouseButtonEvent(0, false);
                g_bFingerAsMouse = 0xFF;
            }
            break;
        }
        
        case 2: // touch push
        {
            const bool bCanUseUI = (CTimer::IsPaused() == g_bAllowGuiTouchOnMenu && TheCamera.GetScreenFadeStatus() != 2);
            g_bFingerOnUI[trackNum] = bCanUseUI && (io.WantTextInput || IsTouchedOverUI((float)x, (float)y));
            if (g_bFingerOnUI[trackNum] && g_bFingerAsMouse == 0xFF)
            {
                io.AddMousePosEvent((float)x, (float)y);
                io.AddMouseButtonEvent(0, true);
                g_bFingerAsMouse = trackNum;
            }
            break;
        }

        case 3: // touch move
        {
            if (g_bFingerAsMouse == trackNum)
                io.AddMousePosEvent((float)x, (float)y);
            break;
        }
    }
    
    const bool swallow = g_bFingerOnUI[trackNum];
    if (actionType == 1) g_bFingerOnUI[trackNum] = false;
    if (!swallow) TouchEvent(actionType, trackNum, x, y);
}

// keyboard system
int* NVtoKK;
char *KKtoChar, *KKtoShiftedChar;
DECL_HOOKv(KeyboardEvent, bool pushed, int keyNum, int ctrl_or_shift, int alwaysZero)
{
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput)
    {
        AND_KeyboardEvent(pushed, keyNum, ctrl_or_shift, alwaysZero);
        return;
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
void RenderImGui()
{
    RenderUI();
    ImGuiIO& io = ImGui::GetIO();
    if (g_nClearMousePos > 0 && --g_nClearMousePos == 0) io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
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
    SET_TO(NVtoKK, aml->GetSym(hGame, "NVtoKK"));
    SET_TO(KKtoChar, aml->GetSym(hGame, "KKtoChar"));
    SET_TO(KKtoShiftedChar, aml->GetSym(hGame, "KKtoShiftedChar"));
    
    // hooks/events
    Events::initRwEvent.after += InitUI;
#ifndef IMGUI_EXCLUDE_GAME
    Events::drawingEvent.after += []()
    {
        g_bAllowGuiTouchOnMenu = false;
        RenderImGui();
    };
#endif
#ifndef IMGUI_EXCLUDE_MENU
    Events::menuDrawingEvent.after += []()
    {
        g_bAllowGuiTouchOnMenu = true;
        RenderImGui();
    };
#endif
    Events::shutdownRwEvent.before += ShutdownUI;
    HOOK(TouchEvent, AND_TouchEvent);
    HOOK(KeyboardEvent, AND_KeyboardEvent);
}
