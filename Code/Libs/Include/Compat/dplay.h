#pragma once
//
// dplay.h - DirectPlay compatibility stub
// DirectPlay was removed from the Windows SDK. This stub provides enough
// type definitions to compile legacy code. Multiplayer requires a full
// replacement (e.g. ENet, Steam Networking, or a custom Winsock layer).
//
#ifndef __DPLAY_INCLUDED__
#define __DPLAY_INCLUDED__

#include <windows.h>
#include <objbase.h>

// -------------------------------------------------------------------------
// GUIDs
// DirectPlay GUIDs are not in any import library (DirectPlay was removed from
// the Windows and DXSDK), so we define them directly as selectany globals so
// each TU gets a definition and the linker merges duplicates.
// -------------------------------------------------------------------------
#pragma push_macro("DEFINE_GUID")
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C __declspec(selectany) const GUID name = \
        { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
DEFINE_GUID(CLSID_DirectPlay,       0xd1eb6d20,0x8923,0x11d0,0x9d,0x97,0x00,0xa0,0xc9,0x0a,0x43,0xcb);
DEFINE_GUID(IID_IDirectPlay2,       0x2b74f7c0,0x9154,0x11d0,0x9d,0x97,0x00,0xa0,0xc9,0x0a,0x43,0xcb);
DEFINE_GUID(IID_IDirectPlay4,       0xab1c530b,0x4745,0x11d1,0xa7,0xa1,0x00,0x00,0xf8,0x05,0x57,0x8a);
DEFINE_GUID(DPSPGUID_TCPIP,         0x36E95EE0,0x8577,0x11cf,0x96,0x0C,0x00,0x80,0xC7,0x53,0x4E,0x82);
DEFINE_GUID(DPSPGUID_IPX,           0x685BC400,0x9D2C,0x11cf,0xA9,0xCD,0x00,0xAA,0x00,0x68,0x86,0xE3);
DEFINE_GUID(DPSPGUID_MODEM,         0x44EAA760,0xCB68,0x11cf,0x9C,0x4E,0x00,0xA0,0xC9,0x05,0x42,0x5E);
DEFINE_GUID(DPSPGUID_SERIAL,        0xCA9C3B00,0xE750,0x11cf,0xA9,0xCD,0x00,0xAA,0x00,0x68,0x86,0xE3);
DEFINE_GUID(DPAID_ServiceProvider,  0x07D916C0,0xE0AF,0x11cf,0x9C,0x4E,0x00,0xA0,0xC9,0x05,0x42,0x5E);
DEFINE_GUID(DPAID_INetW,            0x4e812f80,0x93e5,0x11d0,0x9d,0x97,0x00,0xa0,0xc9,0x0a,0x43,0xcb);
#pragma pop_macro("DEFINE_GUID")

// -------------------------------------------------------------------------
// Typedefs
// -------------------------------------------------------------------------
typedef DWORD DPID, FAR *LPDPID;

// -------------------------------------------------------------------------
// Return codes
// -------------------------------------------------------------------------
#define DP_OK                    S_OK
#define DPERR_BUFFERTOOSMALL     MAKE_HRESULT(1, FACILITY_WIN32, ERROR_INSUFFICIENT_BUFFER)
#define DPERR_CONNECTING         MAKE_HRESULT(1, 0x877, 0x0604)
#define DPERR_USERCANCEL         MAKE_HRESULT(1, 0x877, 0x0410)
#define DPERR_NOMESSAGES         MAKE_HRESULT(1, 0x877, 0x0607)
#define DPERR_PENDING            MAKE_HRESULT(1, 0x877, 0x0040)
#define DPERR_UNSUPPORTED        E_NOTIMPL

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------
#define DPOPEN_JOIN              0x00000001
#define DPOPEN_CREATE            0x00000002

#define DPSEND_GUARANTEED        0x00000001
#define DPSEND_ASYNC             0x00000200
#define DPSEND_NOSENDCOMPLETEMSG 0x00000400
#define DPSEND_SIGNED            0x00000800
#define DPSEND_ENCRYPTED         0x00001000

#define DPRECEIVE_ALL            0x00000001
#define DPRECEIVE_FROMPLAYER     0x00000002
#define DPRECEIVE_PEEK           0x00000008

#define DPENUMSESSIONS_ALL       0x00000001
#define DPENUMSESSIONS_ASYNC     0x00000002
#define DPENUMSESSIONS_STOPASYNC 0x00000004

#define DPENUMPLAYERS_REMOTE     0x00000002
#define DPENUMPLAYERS_SESSION    0x00000004

#define DPSESSION_MIGRATEHOST         0x00000004
#define DPSESSION_KEEPALIVE           0x00000010
#define DPSESSION_JOINDISABLED        0x00000020
#define DPSESSION_NOPRESERVEORDER     0x00000080
#define DPSESSION_NOMESSAGEID         0x00000100
#define DPSESSION_DIRECTPLAYPROTOCOL  0x00000800
#define DPSESSION_CLIENTSERVER        0x00001000

#define DPCAPS_ISHOST            0x00000002

#define DPCONNECTION_DIRECTPLAY  0x00000001

#define DPPLAYERTYPE_PLAYER      0x00000000
#define DPPLAYERTYPE_GROUP       0x00000001

#define DPID_ALLPLAYERS          ((DPID)0)
#define DPID_SERVERPLAYER        ((DPID)1)
#define DPID_SYSMSG              ((DPID)0)

// System message type constants
#define DPSYS_CREATEPLAYERORGROUP   0x0003
#define DPSYS_DESTROYPLAYERORGROUP  0x0005
#define DPSYS_ADDPLAYERTOGROUP      0x0007
#define DPSYS_DELETEPLAYERFROMGROUP 0x0021
#define DPSYS_SESSIONLOST           0x0031
#define DPSYS_HOST                  0x0101
#define DPSYS_SETPLAYERORGROUPDATA  0x0102
#define DPSYS_SETPLAYERORGROUPNAME  0x0103
#define DPSYS_SETSESSIONDESC        0x0104
#define DPSYS_SENDCOMPLETE          0x0105

// -------------------------------------------------------------------------
// Structures
// -------------------------------------------------------------------------
typedef struct _DPNAME {
    DWORD  dwSize;
    DWORD  dwFlags;
    LPWSTR lpszShortName;
    LPWSTR lpszLongName;
} DPNAME, FAR *LPDPNAME;
typedef const DPNAME FAR *LPCDPNAME;

typedef struct _DPSESSIONDESC2 {
    DWORD  dwSize;
    DWORD  dwFlags;
    GUID   guidInstance;
    GUID   guidApplication;
    DWORD  dwMaxPlayers;
    DWORD  dwCurrentPlayers;
    union {
        LPWSTR lpszSessionName;
        DWORD  lpszSessionNameA;
    };
    union {
        LPWSTR lpszPassword;
        DWORD  lpszPasswordA;
    };
    DWORD_PTR dwReserved1;
    DWORD_PTR dwReserved2;
    DWORD_PTR dwUser1;
    DWORD_PTR dwUser2;
    DWORD_PTR dwUser3;
    DWORD_PTR dwUser4;
} DPSESSIONDESC2, FAR *LPDPSESSIONDESC2;
typedef const DPSESSIONDESC2 FAR *LPCDPSESSIONDESC2;

typedef struct _DPCAPS {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwMaxBufferSize;
    DWORD dwMaxQueueSize;
    DWORD dwMaxPlayers;
    DWORD dwHundredBaud;
    DWORD dwLatency;
    DWORD dwMaxLocalPlayers;
    DWORD dwHeaderLength;
    DWORD dwTimeout;
} DPCAPS, FAR *LPDPCAPS;

// System message structures
typedef struct _DPMSG_GENERIC {
    DWORD dwType;
} DPMSG_GENERIC, FAR *LPDPMSG_GENERIC;

typedef struct _DPMSG_CREATEPLAYERORGROUP {
    DWORD  dwType;
    DWORD  dwPlayerType;
    DPID   dpId;
    DWORD  dwCurrentPlayers;
    LPVOID lpData;
    DWORD  dwDataSize;
    DPNAME dpnName;
    DPID   dpIdParent;
    DWORD  dwFlags;
} DPMSG_CREATEPLAYERORGROUP, FAR *LPDPMSG_CREATEPLAYERORGROUP;

typedef struct _DPMSG_DESTROYPLAYERORGROUP {
    DWORD  dwType;
    DWORD  dwPlayerType;
    DPID   dpId;
    LPVOID lpLocalData;
    DWORD  dwLocalDataSize;
    LPVOID lpRemoteData;
    DWORD  dwRemoteDataSize;
    DPNAME dpnName;
    DPID   dpIdParent;
    DWORD  dwFlags;
} DPMSG_DESTROYPLAYERORGROUP, FAR *LPDPMSG_DESTROYPLAYERORGROUP;

typedef struct _DPCOMPOUNDADDRESSELEMENT {
    GUID   guidDataType;
    DWORD  dwDataSize;
    LPVOID lpData;
} DPCOMPOUNDADDRESSELEMENT, FAR *LPDPCOMPOUNDADDRESSELEMENT;

// -------------------------------------------------------------------------
// Callback typedefs
// -------------------------------------------------------------------------
typedef BOOL (FAR PASCAL *LPDPENUMCONNECTIONSCALLBACK)(
    LPCGUID lpguidSP, LPVOID lpConnection, DWORD dwConnectionSize,
    LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext);

typedef BOOL (FAR PASCAL *LPDPENUMSESSIONSCALLBACK2)(
    LPCDPSESSIONDESC2 lpThisSD, LPDWORD lpdwTimeOut,
    DWORD dwFlags, LPVOID lpContext);

typedef BOOL (FAR PASCAL *LPDPENUMPLAYERSCALLBACK2)(
    DPID dpId, DWORD dwPlayerType, LPCDPNAME lpName,
    DWORD dwFlags, LPVOID lpContext);

typedef BOOL (FAR PASCAL *LPDPENUMADDRESSCALLBACK)(
    REFGUID guidDataType, DWORD dwDataSize,
    LPCVOID lpData, LPVOID lpContext);

// Legacy callback alias
typedef LPDPENUMPLAYERSCALLBACK2 LPDPENUMPLAYERSCALLBACK;

// -------------------------------------------------------------------------
// Forward declarations for types used in IDirectPlay4 interface below
// -------------------------------------------------------------------------
typedef struct _DPSECURITYDESC  DPSECURITYDESC,  FAR *LPDPSECURITYDESC;
typedef const DPSECURITYDESC FAR *LPCDPSECURITYDESC;
typedef struct _DPCREDENTIALS   DPCREDENTIALS,   FAR *LPDPCREDENTIALS;
typedef const DPCREDENTIALS FAR *LPCDPCREDENTIALS;
typedef struct _DPCHAT          DPCHAT,          FAR *LPDPCHAT;
// DPLCONNECTION is defined in dplobby.h; forward-declare only
typedef struct _DPLCONNECTION   DPLCONNECTION,   FAR *LPDPLCONNECTION;

// -------------------------------------------------------------------------
// IDirectPlay2 interface (minimal - used for QueryInterface only)
// -------------------------------------------------------------------------
#undef  INTERFACE
#define INTERFACE IDirectPlay2
DECLARE_INTERFACE_(IDirectPlay2, IUnknown) {
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    // Remaining methods omitted - add as needed
};
typedef IDirectPlay2 FAR *LPDIRECTPLAY2;

// -------------------------------------------------------------------------
// IDirectPlay4 interface
// -------------------------------------------------------------------------
#undef  INTERFACE
#define INTERFACE IDirectPlay4
DECLARE_INTERFACE_(IDirectPlay4, IUnknown) {
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(AddPlayerToGroup)(THIS_ DPID idGroup, DPID idPlayer) PURE;
    STDMETHOD(Close)(THIS) PURE;
    STDMETHOD(CreateGroup)(THIS_ LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
    STDMETHOD(CreatePlayer)(THIS_ LPDPID lpidPlayer, LPDPNAME lpPlayerName, HANDLE hEvent, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
    STDMETHOD(DeletePlayerFromGroup)(THIS_ DPID idGroup, DPID idPlayer) PURE;
    STDMETHOD(DestroyGroup)(THIS_ DPID idGroup) PURE;
    STDMETHOD(DestroyPlayer)(THIS_ DPID idPlayer) PURE;
    STDMETHOD(EnumGroupPlayers)(THIS_ DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
    STDMETHOD(EnumGroups)(THIS_ LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
    STDMETHOD(EnumLocalApplications)(THIS_ LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
    STDMETHOD(EnumPlayers)(THIS_ LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
    STDMETHOD(EnumSessions)(THIS_ LPDPSESSIONDESC2 lpsd, DWORD dwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDPCAPS lpDPCaps, DWORD dwFlags) PURE;
    STDMETHOD(GetGroupData)(THIS_ DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags) PURE;
    STDMETHOD(GetGroupName)(THIS_ DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
    STDMETHOD(GetMessageCount)(THIS_ DPID idPlayer, LPDWORD lpdwCount) PURE;
    STDMETHOD(GetPlayerAddress)(THIS_ DPID idPlayer, LPVOID lpAddress, LPDWORD lpdwAddressSize) PURE;
    STDMETHOD(GetPlayerCaps)(THIS_ DPID idPlayer, LPDPCAPS lpPlayerCaps, DWORD dwFlags) PURE;
    STDMETHOD(GetPlayerData)(THIS_ DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize, DWORD dwFlags) PURE;
    STDMETHOD(GetPlayerName)(THIS_ DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
    STDMETHOD(GetSessionDesc)(THIS_ LPVOID lpData, LPDWORD lpdwDataSize) PURE;
    STDMETHOD(Initialize)(THIS_ LPGUID lpGUID) PURE;
    STDMETHOD(Open)(THIS_ LPDPSESSIONDESC2 lpsd, DWORD dwFlags) PURE;
    STDMETHOD(Receive)(THIS_ LPDPID lpidFrom, LPDPID lpidTo, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
    STDMETHOD(Send)(THIS_ DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize) PURE;
    STDMETHOD(SetGroupData)(THIS_ DPID idGroup, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
    STDMETHOD(SetGroupName)(THIS_ DPID idGroup, LPDPNAME lpGroupName, DWORD dwFlags) PURE;
    STDMETHOD(SetPlayerData)(THIS_ DPID idPlayer, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
    STDMETHOD(SetPlayerName)(THIS_ DPID idPlayer, LPDPNAME lpPlayerName, DWORD dwFlags) PURE;
    STDMETHOD(SetSessionDesc)(THIS_ LPDPSESSIONDESC2 lpSessDesc, DWORD dwFlags) PURE;
    STDMETHOD(AddGroupToGroup)(THIS_ DPID idParentGroup, DPID idGroup) PURE;
    STDMETHOD(CreateGroupInGroup)(THIS_ DPID idParentGroup, LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags) PURE;
    STDMETHOD(DeleteGroupFromGroup)(THIS_ DPID idParentGroup, DPID idGroup) PURE;
    STDMETHOD(EnumConnections)(THIS_ LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags) PURE;
    STDMETHOD(EnumGroupsInGroup)(THIS_ DPID idGroup, LPGUID lpguidInstance, LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext, DWORD dwFlags) PURE;
    STDMETHOD(GetGroupConnectionSettings)(THIS_ DWORD dwFlags, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
    STDMETHOD(InitializeConnection)(THIS_ LPVOID lpConnection, DWORD dwFlags) PURE;
    STDMETHOD(SecureOpen)(THIS_ LPCDPSESSIONDESC2 lpsd, DWORD dwFlags, LPCDPSECURITYDESC lpSecurity, LPCDPCREDENTIALS lpCredentials) PURE;
    STDMETHOD(SendChatMessage)(THIS_ DPID idFrom, DPID idTo, DWORD dwFlags, LPDPCHAT lpChatMessage) PURE;
    STDMETHOD(SetGroupConnectionSettings)(THIS_ DWORD dwFlags, DPID idGroup, LPDPLCONNECTION lpConnection) PURE;
    STDMETHOD(StartSession)(THIS_ DWORD dwFlags, DPID idGroup) PURE;
    STDMETHOD(GetGroupFlags)(THIS_ DPID idGroup, LPDWORD lpdwFlags) PURE;
    STDMETHOD(GetGroupParent)(THIS_ DPID idGroup, LPDPID lpidParent) PURE;
    STDMETHOD(GetPlayerAccount)(THIS_ DPID idPlayer, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize) PURE;
    STDMETHOD(GetPlayerFlags)(THIS_ DPID idPlayer, LPDWORD lpdwFlags) PURE;
    STDMETHOD(GetGroupOwner)(THIS_ DPID idGroup, LPDPID lpidGroupOwner) PURE;
    STDMETHOD(SetGroupOwner)(THIS_ DPID idGroup, DPID idGroupOwner) PURE;
    STDMETHOD(SendEx)(THIS_ DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout, LPVOID lpContext, LPDWORD lpdwMsgID) PURE;
    STDMETHOD(GetMessageQueue)(THIS_ DPID idFrom, DPID idTo, DWORD dwFlags, LPDWORD lpdwNumMsgs, LPDWORD lpdwNumBytes) PURE;
    STDMETHOD(CancelMessage)(THIS_ DWORD dwMsgID, DWORD dwFlags) PURE;
    STDMETHOD(CancelPriority)(THIS_ DWORD dwMinPriority, DWORD dwMaxPriority, DWORD dwFlags) PURE;
};
typedef IDirectPlay4 FAR *LPDIRECTPLAY4;

#endif /* __DPLAY_INCLUDED__ */
