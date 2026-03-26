// ddrawex.h stub — IDirectDrawFactory (ddrawex) is not available on modern Windows.
// VidStream.cpp includes this header but uses no types from it directly.
#pragma once
#ifndef __DDRAWEX_H__
#define __DDRAWEX_H__

#include <ddraw.h>

// Forward declaration of IDirectDrawFactory (defined but not usable on Windows 11)
#undef INTERFACE
#define INTERFACE IDirectDrawFactory
DECLARE_INTERFACE_(IDirectDrawFactory, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(CreateDirectDraw)(THIS_ GUID *pGUID, HWND hWnd, DWORD dwCoopLevelFlags,
                                DWORD dwReserved, IUnknown *pUnkOuter, IDirectDraw **ppDirectDraw) PURE;
    STDMETHOD(DirectDrawEnumerate)(THIS_ LPDDENUMCALLBACK lpCallback, LPVOID lpContext) PURE;
};

#endif // __DDRAWEX_H__
