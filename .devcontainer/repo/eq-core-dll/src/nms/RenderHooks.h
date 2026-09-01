#pragma once

#include "../core_floatingtext.h"   // g_deviceAcquired, g_pFtm
#include <d3d9.h>
#include "FloatingTextManager.h"
#include "MQ2Main.h"

extern FloatingTextManager* g_pFtm;
 // the global direct3d device that we are "borrowing"
extern IDirect3DDevice9* g_pDevice;

// represents whether the device has been acquired and is good to use.
extern bool g_deviceAcquired;

extern HMODULE g_d3d9Module;

// Address of the Reset() function
extern DWORD g_resetDeviceAddress;

template <typename T>
void InstallDetour(DWORD address, const T& detour, const T& trampoline, PCHAR name);

class RenderHooks
{
public:
	//------------------------------------------------------------------------
	// d3d9 hooks

	// this is only valid during a d3d9 hook detour
	IDirect3DDevice9* GetThisDevice() { return reinterpret_cast<IDirect3DDevice9*>(this); }

	// Install hooks on actual instance of the device once we have it.
	bool DetectResetDeviceHook()
	{
		bool changed = false;

		// IDirect3DDevice9 virtual function hooks
		DWORD* d3dDevice_vftable = *(DWORD * *)this;

		DWORD resetDevice = d3dDevice_vftable[0x10];

		if (resetDevice != g_resetDeviceAddress)
		{
			if (g_resetDeviceAddress != 0)
			{
				//SPDLOG_WARN("Detected a change in the rendering device. Attempting to recover.");
			}
			g_resetDeviceAddress = resetDevice;

			InstallDetour(d3dDevice_vftable[0x10],
				&RenderHooks::Reset_Detour,
				&RenderHooks::Reset_Trampoline,
				"d3dDevice_Reset");
			changed = true;
		}

		return changed;
	}

	HRESULT WINAPI Reset_Trampoline(D3DPRESENT_PARAMETERS* pPresentationParameters);
	HRESULT WINAPI Reset_Detour(D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		g_pFtm->Cleanup();

		if (g_pDevice != GetThisDevice())
		{
			return Reset_Trampoline(pPresentationParameters);
		}

		g_deviceAcquired = false;

		return Reset_Trampoline(pPresentationParameters);
	}

	HRESULT WINAPI BeginScene_Trampoline();
	HRESULT WINAPI BeginScene_Detour()
	{
		g_pDevice = GetThisDevice();
		return BeginScene_Trampoline();
	}

	HRESULT WINAPI EndScene_Trampoline();
	HRESULT WINAPI EndScene_Detour()
	{
		if (GetThisDevice() != g_pDevice)
		{
			return EndScene_Trampoline();
		}

		// When TestCooperativeLevel returns all good, then we can reinitialize.
		if (!g_deviceAcquired)
		{
			HRESULT result = GetThisDevice()->TestCooperativeLevel();

			if (result == D3D_OK)
			{
				g_deviceAcquired = true;
				g_pFtm->Initialize();
				DetectResetDeviceHook();
			}
			else
			{
				return EndScene_Trampoline();
			}
		}

		// Perform the render within a stateblock so we don't upset the
		// rest of the rendering pipeline
		if (g_deviceAcquired)
		{
			// AoTv4: a failed state block must NOT skip the render.
			// \u26a0\u26a0 UPSTREAM PUT Render() INSIDE THE SUCCESS BRANCH, so any frame where
			// CreateStateBlock failed drew no numbers at all -- silently, with the next frame fine. That is
			// a per-frame device allocation, so it fails exactly when the client is busiest, which is
			// during a fight. It is a prime suspect for "the numbers appear sometimes".
			// \u26a0 The state block is belt-and-braces anyway: ID3DXSprite::Begin saves the pipeline state
			// itself unless D3DXSPRITE_DONOTSAVESTATE is passed, and Text::Render passes only
			// D3DXSPRITE_ALPHABLEND. So rendering without it is the same protection minus one allocation.
			IDirect3DStateBlock9* stateBlock = nullptr;
			const bool haveBlock = SUCCEEDED(g_pDevice->CreateStateBlock(D3DSBT_ALL, &stateBlock)) && stateBlock;

			g_pFtm->Render();


			if (haveBlock)
			{
				stateBlock->Apply();
				stateBlock->Release();
			}
		}

		return EndScene_Trampoline();
	}

	//------------------------------------------------------------------------
	// EQGraphicsDX9.dll hooks
	void ZoneRender_Injection_Trampoline();
	void ZoneRender_Injection_Detour()
	{
		// Perform the render within a stateblock so we don't upset the
		// rest of the rendering pipeline
		if (g_deviceAcquired)
		{
			IDirect3DStateBlock9* stateBlock = nullptr;
			if (SUCCEEDED(g_pDevice->CreateStateBlock(D3DSBT_ALL, &stateBlock)) && stateBlock)
			{
				stateBlock->Apply();
				stateBlock->Release();
			}
		}

		ZoneRender_Injection_Trampoline();
	}
};
