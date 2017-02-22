
#include <capsule.h>

#include <d3d9.h>
#include "d3d9-vtable-helpers.h"

///////////////////////////////////////////////
// IDirect3DDevice9::Present
///////////////////////////////////////////////
typedef HRESULT (CAPSULE_STDCALL *Present_t)(
  IDirect3DDevice9   *device,
  const RECT         *pSourceRect,
  const RECT         *pDestRect,
        HWND         hDestWindowOverride,
  const RGNDATA      *pDirtyRegion
);
static Present_t Present_real;
static SIZE_T Present_hookId = 0;

HRESULT CAPSULE_STDCALL Present_hook (
  IDirect3DDevice9   *device,
  const RECT         *pSourceRect,
  const RECT         *pDestRect,
        HWND         hDestWindowOverride,
  const RGNDATA      *pDirtyRegion
) {
  capsule_log("IDirect3DDevice9::Present called");
  HRESULT res = Present_real(
    device,
    pSourceRect,
    pDestRect,
    hDestWindowOverride,
    pDirtyRegion
  );
  return res;
}

///////////////////////////////////////////////
// IDirect3DDevice9Ex::PresentEx
///////////////////////////////////////////////
typedef HRESULT (CAPSULE_STDCALL *PresentEx_t)(
  IDirect3DDevice9Ex *device,
  const RECT         *pSourceRect,
  const RECT         *pDestRect,
        HWND         hDestWindowOverride,
  const RGNDATA      *pDirtyRegion,
        DWORD        dwFlags
);
static PresentEx_t PresentEx_real;
static SIZE_T PresentEx_hookId = 0;

HRESULT CAPSULE_STDCALL PresentEx_hook (
  IDirect3DDevice9Ex *device,
  const RECT         *pSourceRect,
  const RECT         *pDestRect,
        HWND         hDestWindowOverride,
  const RGNDATA      *pDirtyRegion,
        DWORD        dwFlags
) {
  capsule_log("IDirect3DDevice9Ex::PresentEx called");
  HRESULT res = PresentEx_real(
    device,
    pSourceRect,
    pDestRect,
    hDestWindowOverride,
    pDirtyRegion,
    dwFlags
  );
  return res;
}

///////////////////////////////////////////////
// IDirect3DSwapChain9::Present
///////////////////////////////////////////////
typedef HRESULT (CAPSULE_STDCALL *PresentSwap_t)(
  IDirect3DSwapChain9   *swap,
  const RECT            *pSourceRect,
  const RECT            *pDestRect,
        HWND            hDestWindowOverride,
  const RGNDATA         *pDirtyRegion,
        DWORD           dwFlags
);
static PresentSwap_t PresentSwap_real;
static SIZE_T PresentSwap_hookId = 0;

HRESULT CAPSULE_STDCALL PresentSwap_hook (
  IDirect3DSwapChain9   *swap,
  const RECT            *pSourceRect,
  const RECT            *pDestRect,
        HWND            hDestWindowOverride,
  const RGNDATA         *pDirtyRegion,
        DWORD           dwFlags
) {
  capsule_log("IDirect3DSwapChain9::Present called");
  HRESULT res = PresentSwap_real(
    swap,
    pSourceRect,
    pDestRect,
    hDestWindowOverride,
    pDirtyRegion,
    dwFlags
  );
  return res;
}

static void install_present_hooks (IDirect3DDevice9 *device) {
  DWORD err;

  if (!Present_hookId) {
    LPVOID Present_addr = capsule_get_IDirect3DDevice9_Present_address((void*) device);
    if (!Present_addr) {
      capsule_log("Could not find IDirect3DDevice9::Present");
      return;
    }

    err = cHookMgr.Hook(
      &Present_hookId,
      (LPVOID *) &Present_real,
      Present_addr,
      Present_hook,
      0
    );
    if (err != ERROR_SUCCESS) {
      capsule_log("Hooking IDirect3DDevice9::Present derped with error %d (%x)", err, err);
    } else {
      capsule_log("Installed IDirect3DDevice9::Present hook");
    }
  }

  if (!PresentEx_hookId) {
    IDirect3D9Ex *deviceEx;
    HRESULT castRes = device->QueryInterface(__uuidof(IDirect3D9Ex), (void**) &deviceEx);
    if (SUCCEEDED(castRes)) {
      LPVOID PresentEx_addr = capsule_get_IDirect3DDevice9Ex_PresentEx_address((void*) deviceEx);
      if (!PresentEx_addr) {
        capsule_log("Could not find IDirect3DDevice9Ex::PresentEx");
        return;
      }

      err = cHookMgr.Hook(
        &PresentEx_hookId,
        (LPVOID *) &PresentEx_real,
        PresentEx_addr,
        PresentEx_hook,
        0
      );
      if (err != ERROR_SUCCESS) {
        capsule_log("Hooking IDirect3DDevice9Ex::PresentEx derped with error %d (%x)", err, err);
      } else {
        capsule_log("Installed IDirect3DDevice9Ex::PresentEx hook");
      }
    }
  }

  if (!PresentSwap_hookId) {
    IDirect3DSwapChain9 *swap;
    HRESULT getRes = device->GetSwapChain(0, &swap);
    if (SUCCEEDED(getRes)) {
      LPVOID PresentSwap_addr = capsule_get_IDirect3DSwapChain9_Present_address((void*) swap);
      if (!PresentSwap_addr) {
        capsule_log("Could not find IDirect3DSwapChain9::Present");
        return;
      }

      err = cHookMgr.Hook(
        &PresentSwap_hookId,
        (LPVOID *) &PresentSwap_real,
        PresentSwap_addr,
        PresentSwap_hook,
        0
      );
      if (err != ERROR_SUCCESS) {
        capsule_log("Hooking IDirect3DSwapChain9::Present derped with error %d (%x)", err, err);
      } else {
        capsule_log("Installed IDirect3DSwapChain9::Present hook");
      }
    }
  }
}

///////////////////////////////////////////////
// IDirect3D9::CreateDevice
///////////////////////////////////////////////
typedef HRESULT (CAPSULE_STDCALL *CreateDevice_t)(
  IDirect3D9            *obj,
  UINT                  Adapter,
  D3DDEVTYPE            DeviceType,
  HWND                  hFocusWindow,
  DWORD                 BehaviorFlags,
  D3DPRESENT_PARAMETERS *pPresentationParameters,
  IDirect3DDevice9      **ppReturnedDeviceInterface
);
static CreateDevice_t CreateDevice_real;
static SIZE_T CreateDevice_hookId = 0;

HRESULT CAPSULE_STDCALL CreateDevice_hook (
  IDirect3D9            *obj,
  UINT                  Adapter,
  D3DDEVTYPE            DeviceType,
  HWND                  hFocusWindow,
  DWORD                 BehaviorFlags,
  D3DPRESENT_PARAMETERS *pPresentationParameters,
  IDirect3DDevice9      **ppReturnedDeviceInterface
) {
  capsule_log("IDirect3D9::CreateDevice called with adapter %u", (unsigned int) Adapter);
  HRESULT res = CreateDevice_real(
    obj,
    Adapter,
    DeviceType,
    hFocusWindow,
    BehaviorFlags,
    pPresentationParameters,
    ppReturnedDeviceInterface
  );

  if (SUCCEEDED(res)) {
    install_present_hooks(*ppReturnedDeviceInterface);
  }

  return res;
}

static void install_device_hooks (IDirect3D9 *obj) {
  DWORD err;

  // CreateDevice
  if (!CreateDevice_hookId) {
    LPVOID CreateDevice_addr = capsule_get_IDirect3D9_CreateDevice_address((void *) obj);

    err = cHookMgr.Hook(
      &CreateDevice_hookId,
      (LPVOID *) &CreateDevice_real,
      CreateDevice_addr,
      CreateDevice_hook,
      0
    );
    if (err != ERROR_SUCCESS) {
      capsule_log("Hooking CreateDevice derped with error %d (%x)", err, err);
    } else {
      capsule_log("Installed CreateDevice hook");
    }
  }
}

///////////////////////////////////////////////
// Direct3DCreate9
///////////////////////////////////////////////

typedef IDirect3D9* (CAPSULE_STDCALL *Direct3DCreate9_t)(
  UINT SDKVersion
);
static Direct3DCreate9_t Direct3DCreate9_real;
static SIZE_T Direct3DCreate9_hookId = 0;

IDirect3D9 * CAPSULE_STDCALL Direct3DCreate9_hook (
  UINT SDKVersion
) {
  capsule_log("Direct3DCreate9 called with SDKVersion %u", (unsigned int) SDKVersion);
  IDirect3D9 * res = Direct3DCreate9_real(SDKVersion);
  if (res) {
    capsule_log("d3d9 device created!");
    install_device_hooks(res);
  } else {
    capsule_log("d3d9 device could not be created.");
  }

  return res;
}

void capsule_install_d3d9_hooks () {
  DWORD err;

  HMODULE d3d9 = LoadLibrary(L"d3d9.dll");
  if (!d3d9) {
    capsule_log("Could not load d3d9.dll, disabling D3D9 capture");
    return;
  }

  // Direct3DCreate9
  if (!Direct3DCreate9_hookId) {
    LPVOID Direct3DCreate9_addr = NktHookLibHelpers::GetProcedureAddress(d3d9, "Direct3DCreate9");
    if (!Direct3DCreate9_addr) {
      capsule_log("Could not find Direct3DCreate9");
      return;
    }

    err = cHookMgr.Hook(
      &Direct3DCreate9_hookId,
      (LPVOID *) &Direct3DCreate9_real,
      Direct3DCreate9_addr,
      Direct3DCreate9_hook,
      0
    );
    if (err != ERROR_SUCCESS) {
      capsule_log("Hooking Direct3DCreate9 derped with error %d (%x)", err, err);
      return;
    }
    capsule_log("Installed Direct3DCreate9 hook");
  }
}