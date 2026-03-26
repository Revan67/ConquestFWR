#pragma once
//
// dplobby.h - DirectPlay Lobby compatibility stub
// DirectPlay was removed from the Windows SDK. This stub provides enough
// type definitions to compile legacy code. Multiplayer requires a full
// replacement.
//
#ifndef __DPLOBBY_INCLUDED__
#define __DPLOBBY_INCLUDED__

#include <windows.h>
#include <objbase.h>
#include "dplay.h"

// -------------------------------------------------------------------------
// GUIDs (defined inline; DirectPlay GUIDs are not in any import library)
// -------------------------------------------------------------------------
#pragma push_macro("DEFINE_GUID")
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C __declspec(selectany) const GUID name = \
        { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
DEFINE_GUID(CLSID_DirectPlayLobby,  0x2fe8f810,0xb2a5,0x11d0,0xa7,0x87,0x00,0x00,0xf8,0x03,0xab,0xfc);
DEFINE_GUID(IID_IDirectPlayLobby3,  0x2db72490,0x652c,0x11d1,0xa7,0xa8,0x00,0x00,0xf8,0x05,0x57,0x8a);
#pragma pop_macro("DEFINE_GUID")

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------
#define DPLMSG_STANDARD          0x00000001
#define DPLWAIT_CANCEL           0x00000001
#define DPL_NOCONFIRMATION       0x00000001

#define DPLMSG_SETPROPERTY_MSGID    0x0006
#define DPLMSG_SYSTEMMESSAGE_MSGID  0x8000

// Lobby system message types
#define DPLSYS_CONNECTIONSETTINGSREAD   0x00000001
#define DPLSYS_DPLAYCONNECTFAILED       0x00000002
#define DPLSYS_CONNECTED                0x00000003
#define DPLSYS_LOBBYCLIENTRELEASE       0x00000004
#define DPLSYS_GETPROPERTY              0x00000005
#define DPLSYS_SETPROPERTY              0x00000006
#define DPLSYS_GETPROPERTYRESPONSE      0x00000007
#define DPLSYS_SETPROPERTYRESPONSE      0x00000008
#define DPLSYS_NEWSESSIONHOST           0x00000009
#define DPLSYS_NEWCONNECTIONSETTINGS    0x0000000A

typedef const DPLCONNECTION FAR *LPCDPLCONNECTION;

// -------------------------------------------------------------------------
// Structures
// -------------------------------------------------------------------------
struct _DPLCONNECTION {
    DWORD            dwSize;
    DWORD            dwFlags;
    LPDPSESSIONDESC2 lpSessionDesc;
    LPDPNAME         lpPlayerName;
    GUID             guidSP;
    LPVOID           lpAddress;
    DWORD            dwAddressSize;
};
typedef const DPLCONNECTION FAR *LPCDPLCONNECTION;

typedef struct _DPLMSG_GENERIC {
    DWORD dwType;
} DPLMSG_GENERIC, FAR *LPDPLMSG_GENERIC;

typedef struct _DPLMSG_SETPROPERTY {
    DWORD dwType;
    DWORD dwRequestID;
    GUID  guidPlayer;
    GUID  guidPropertyTag;
    DWORD dwDataSize;
    DWORD dwPropertyData[1];
} DPLMSG_SETPROPERTY, FAR *LPDPLMSG_SETPROPERTY;

typedef struct _DPLMSG_SYSTEMMESSAGE {
    DWORD dwType;
    GUID  guidInstance;
} DPLMSG_SYSTEMMESSAGE, FAR *LPDPLMSG_SYSTEMMESSAGE;

// -------------------------------------------------------------------------
// Callback typedef
// -------------------------------------------------------------------------
typedef BOOL (FAR PASCAL *LPDPENUMADDRESSCALLBACK)(
    REFGUID guidDataType, DWORD dwDataSize,
    LPCVOID lpData, LPVOID lpContext);

// -------------------------------------------------------------------------
// IDirectPlayLobby3 interface
// -------------------------------------------------------------------------
#undef  INTERFACE
#define INTERFACE IDirectPlayLobby3
DECLARE_INTERFACE_(IDirectPlayLobby3, IUnknown) {
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(Connect)(THIS_ DWORD dwFlags, LPDIRECTPLAY2 FAR *lplpDP, IUnknown FAR *pUnk) PURE;
    STDMETHOD(CreateAddress)(THIS_ REFGUID guidSP, REFGUID guidDataType, LPCVOID lpData, DWORD dwDataSize, LPVOID lpAddress, LPDWORD lpdwAddressSize) PURE;
    STDMETHOD(EnumAddress)(THIS_ LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, LPCVOID lpAddress, DWORD dwAddressSize, LPVOID lpContext) PURE;
    STDMETHOD(EnumAddressTypes)(THIS_ LPDPENUMADDRESSCALLBACK lpEnumAddressTypeCallback, REFGUID guidSP, LPVOID lpContext, DWORD dwFlags) PURE;
    STDMETHOD(EnumLocalApplications)(THIS_ LPDPENUMPLAYERSCALLBACK2 lpEnumLocalAppCallback, LPVOID lpContext, DWORD dwFlags) PURE;
    STDMETHOD(GetConnectionSettings)(THIS_ DWORD dwAppID, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
    STDMETHOD(ReceiveLobbyMessage)(THIS_ DWORD dwFlags, DWORD dwAppID, LPDWORD lpdwMessageFlags, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
    STDMETHOD(RunApplication)(THIS_ DWORD dwFlags, LPDWORD lpdwAppID, LPDPLCONNECTION lpConn, HANDLE hReceiveEvent) PURE;
    STDMETHOD(SendLobbyMessage)(THIS_ DWORD dwFlags, DWORD dwAppID, LPVOID lpData, DWORD dwDataSize) PURE;
    STDMETHOD(SetConnectionSettings)(THIS_ DWORD dwFlags, DWORD dwAppID, LPDPLCONNECTION lpConn) PURE;
    STDMETHOD(SetLobbyMessageEvent)(THIS_ DWORD dwFlags, DWORD dwAppID, HANDLE hReceiveEvent) PURE;
    STDMETHOD(CreateCompoundAddress)(THIS_ LPDPCOMPOUNDADDRESSELEMENT lpElements, DWORD dwElementCount, LPVOID lpAddress, LPDWORD lpdwAddressSize) PURE;
    STDMETHOD(ConnectEx)(THIS_ DWORD dwFlags, REFIID riid, LPVOID FAR *lplpDP, IUnknown FAR *pUnk) PURE;
    STDMETHOD(RegisterApplication)(THIS_ DWORD dwFlags, LPVOID lpApplicationDesc) PURE;
    STDMETHOD(UnregisterApplication)(THIS_ DWORD dwFlags, REFGUID guidApplication) PURE;
    STDMETHOD(WaitForConnectionSettings)(THIS_ DWORD dwFlags) PURE;
};
typedef IDirectPlayLobby3 FAR *LPDIRECTPLAYLOBBY3;

#endif /* __DPLOBBY_INCLUDED__ */
