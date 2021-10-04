#include "plugin.h"
#include "..\injector\assembly.hpp"
#include "CTimeCycle.h"

using namespace plugin;
using namespace injector;

float* p2dfxFarClip = nullptr;
uintptr_t pRenderHook = 0;
uintptr_t pProject2dfx = 0;

void __cdecl HookedCameraUpdateZShiftScale(RwCamera* camera) {
    if (camera->farPlane == CTimeCycle::m_CurrentColours.m_fFarClip) {
        float newFarClip = *p2dfxFarClip;

        if (newFarClip < 200.0f) newFarClip = 200.0f;
        //else if (newFarClip > 2000.0f) newFarClip = 2000.0f;

        camera->farPlane = newFarClip;
        *p2dfxFarClip = newFarClip;
        CTimeCycle::m_CurrentColours.m_fFarClip = newFarClip;
    }
    Call<0x7EE200, RwCamera*>(camera);
}

class RenderHookFix {
public:
    RenderHookFix() {

        Events::initRwEvent += [] {

            pRenderHook = (uintptr_t)GetModuleHandleA("_gtaRenderHook.asi");
            if (!pRenderHook) pRenderHook = (uintptr_t)GetModuleHandleA("gtaRenderHook.asi");
            if (!pRenderHook) pRenderHook = (uintptr_t)GetModuleHandleA("$gtaRenderHook.asi");

            if (pRenderHook) {

                pProject2dfx = (uintptr_t)GetModuleHandleA("SALodLights.asi");
                if (pProject2dfx) {

                    p2dfxFarClip = ReadMemory<float*>(0x40C524, true);

                    // redirect hook, because RenderHook redirects Idle
                    uintptr_t p = ReadMemory<uintptr_t>(0x53EBE4 + 1) + 0x53EBE4;
                    WriteMemory<uintptr_t>(0x53C215 + 1, p - 0x53C215);

                    // update far clip
                    patch::RedirectCall(0x7EE2B0, HookedCameraUpdateZShiftScale);
                }

                // update ms_aVisibleEntityPtrs
                injector::MakeInline<0x55352E, 0x55352E + 5>([](injector::reg_pack& regs)
                {
                    uintptr_t ms_aVisibleEntityPtrs = ReadMemory<uintptr_t>(0x553529, true);
                    WriteMemory<uintptr_t>(pRenderHook + 0xEDF30 + 3, ms_aVisibleEntityPtrs, true);
                    *(unsigned int*)0xB76844 = regs.eax;
                });

                // update ms_aVisibleLodPtrs
                injector::MakeInline<0x5534FA, 0x5534FA + 5>([](injector::reg_pack& regs)
                {
                    uintptr_t ms_aVisibleEntityPtrs = ReadMemory<uintptr_t>(0x5534F5, true);
                    WriteMemory<uintptr_t>(pRenderHook + 0xEDF74 + 3, ms_aVisibleEntityPtrs, true);
                    *(unsigned int*)0xB76840 = regs.eax;
                });
            }

        };

    }
} renderHookFix;
