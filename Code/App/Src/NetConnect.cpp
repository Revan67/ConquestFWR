//--------------------------------------------------------------------------//
//                                                                          //
//                             NetConnect.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/Src/NetConnect.cpp 19    5/10/01 10:25a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "NetBuffer.h"
#include "NetFileTransfer.h"
#include "NetPacket.h"
#include "CQTrace.h"

#include <EventSys.h>
#include <TSmartPointer.h>
#include <HeapObj.h>

#include <winsock2.h>
#include <dplobby.h>
#include <dplay.h>

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const IID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#include "ZoneLobby.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
static HRESULT CreateDirectPlayInterface( LPDIRECTPLAY4 *lplpDirectPlay4 )
{
   HRESULT         hr;
   LPDIRECTPLAY4  lpDirectPlay4 = NULL;

   // Create an IDirectPlay4 interface
   hr = CoCreateInstance( CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IDirectPlay4, (LPVOID*)&lpDirectPlay4);

   // Return interface created
   *lplpDirectPlay4 = lpDirectPlay4;

   return (hr);
}
//---------------------------------------------------------------------------
//
static inline HRESULT CreateDirectPlayLobbyInterface( LPDIRECTPLAYLOBBY3 *lplpDirectPlayLobby3 )
{
   HRESULT         hr;
   LPDIRECTPLAYLOBBY3 lpDirectPlayLobby3 = NULL;

   // Create an IDirectPlayLobby3 interface
   hr = CoCreateInstance( CLSID_DirectPlayLobby, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IDirectPlayLobby3, (LPVOID*)&lpDirectPlayLobby3);

   // Return interface created
   *lplpDirectPlayLobby3 = lpDirectPlayLobby3;

   return (hr);
}
//---------------------------------------------------------------------------
//
static inline HRESULT CreateZoneScoreInterface (IZoneScore ** lplpZoneScore)
{
	HRESULT hr;
	IZoneScore * lpZoneScore = NULL;

	hr = CoCreateInstance(CLSID_ZoneScore, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IZoneScore, (void**)&lpZoneScore);

   // Return interface created
	*lplpZoneScore = lpZoneScore;
	return hr;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
BOOL32 __stdcall StopNetConnection (bool bStopAll)
{
	EVENTSYS->Send(CQE_NETSHUTDOWN, 0);		// announce to everyone that we are stopping a net session
	NETBUFFER->Initialize();
	FILETRANSFER->Initialize();

	if (DPLAY)
	{
		DWORD dwOldHost = HOSTID;

		DPLAY->Close();
		if (bStopAll)
		{
			DPLAY->Release();
			DPLAY = 0;
		}
		PLAYERID = 0;
		HOSTID = 0;

		if (dwOldHost)
			EVENTSYS->Send(CQE_NEWHOST, (void *)dwOldHost);
	}
	return 1;
}
//------------------------------------------------------------------------------
//
bool __stdcall SendZoneHostChange (void)
{
	char buffer[ sizeof(DPLMSG_SETPROPERTY) ];
	DPLMSG_SETPROPERTY * pProperty = (DPLMSG_SETPROPERTY *) buffer;

	memset(pProperty, 0, sizeof(*pProperty));
	pProperty->dwType = DPLSYS_SETPROPERTY;
	pProperty->dwRequestID = DPL_NOCONFIRMATION;
	pProperty->guidPlayer = GUID_NULL;
	pProperty->guidPropertyTag = ZONEPROPERTY_GameNewHost;

	// send message to the lobby

	if (DPLAY && DPLOBBY)
	{
		HRESULT hr;
		hr = DPLOBBY->SendLobbyMessage(DPLMSG_STANDARD, 0, buffer, sizeof(DPLMSG_SETPROPERTY));
		return SUCCEEDED(hr);
	}
	return true;
}
//------------------------------------------------------------------------------
//
BOOL32 StartNetConnection (BOOL32 & bLobbied)
{
	BOOL32 result = 0;
	COMPTR<IDirectPlay2> lpDP2=0;
	DWORD dwDataSize=0;
	DPLCONNECTION * dpConn = 0;
	HRESULT hresult=0;

	StopNetConnection();

	if (DPLOBBY == 0 && CreateDirectPlayLobbyInterface(&DPLOBBY) != DP_OK)
		goto Done;

	if (DPLOBBY!=0 && ZONESCORE == 0)
		CreateZoneScoreInterface(&ZONESCORE);

	bLobbied = 0;
	//
	// If started via the lobby, check connection settings beforehand, make adjustments.
	// Create a player based on connection settings
	//

	dwDataSize = 0;
	if (CQFLAGS.bDPLobby && (hresult=DPLOBBY->GetConnectionSettings(0, 0, &dwDataSize)) == DPERR_BUFFERTOOSMALL)
	{
		CQASSERT(dwDataSize);
		dpConn = (DPLCONNECTION *) calloc(dwDataSize, 1);

		hresult = DPLOBBY->GetConnectionSettings(0, dpConn, &dwDataSize);
		if (hresult==DP_OK)
		{
			if (dpConn->lpSessionDesc->dwFlags & (DPSESSION_NOPRESERVEORDER|DPSESSION_NOMESSAGEID|DPSESSION_DIRECTPLAYPROTOCOL|DPSESSION_CLIENTSERVER))
				CQERROR1("Questionable flags 0x%X used in session description!", dpConn->lpSessionDesc->dwFlags);

			if ((dpConn->lpSessionDesc->dwFlags & (DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE)) != (DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE))
			{
//				CQERROR1("Session description flags 0x%X missing important values!", dpConn->lpSessionDesc->dwFlags);
				dpConn->lpSessionDesc->dwFlags |= (DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE);
				hresult = DPLOBBY->SetConnectionSettings(0, 0, dpConn);

				if (hresult != DP_OK)
				{
					CQTRACE11("DPLOBBY::SetConnectionSettings() returned error 0x%X", hresult);
				}
			}

			if ((hresult = DPLOBBY->Connect(0, lpDP2, 0)) == DP_OK)
			{
				if (lpDP2->QueryInterface( IID_IDirectPlay4, (LPVOID*)&DPLAY) == DP_OK)
				{
					hresult = DPLAY->CreatePlayer(&PLAYERID, dpConn->lpPlayerName, 0, 0, 0, 0);
					CQASSERT(hresult==DP_OK);

					result = bLobbied = 1;
				}
			}
			else
			{
				CQTRACE11("DPLOBBY::Connect() returned error 0x%X", hresult);
			}
		}
		else
		{
			CQTRACE11("GetConnectionSettings() returned error 0x%X", hresult);
		}
	}
	else
	if (CQFLAGS.bDPLobby)
	{
		CQTRACE11("GetConnectionSettings() returned error 0x%X", hresult);
	}
	
	if (DPLAY == 0)	// not lobbied
	{
		if (CreateDirectPlayInterface(&DPLAY) != DP_OK)
			goto Done;

		result = 1;
		bLobbied = 0;
	}

Done:
	free(dpConn);
	return result;
}
//------------------------------------------------------------------------------
//
struct AddressEnumStruct
{
	bool bOnTwo;
	wchar_t * ipaddress;
	wchar_t * ipaddress2;
};

static BOOL CALLBACK EnumAddressCallback (REFGUID guidDataType, DWORD dwDataSize, LPCVOID lpData, LPVOID lpContext)
{
	if (IsEqualGUID(guidDataType,DPAID_INetW))
	{
		U32 bufferSize;

		AddressEnumStruct * aEnum = ((AddressEnumStruct *) lpContext);
		if(aEnum->bOnTwo)
		{

			bufferSize = __min(256, dwDataSize);
			memcpy(aEnum->ipaddress2, lpData, bufferSize);
			return 0;
		}
		else
		{
			aEnum->bOnTwo = true;

			bufferSize = __min(256, dwDataSize);
			memcpy(aEnum->ipaddress, lpData, bufferSize);
			return 1;
		}
	}

	return 1;
}
//------------------------------------------------------------------------------
//
U32 __stdcall GetHostIPAddress (wchar_t * ipaddress, wchar_t * ipaddress2, U32 bufferSize)
{
	U32 result = 0;
	DWORD dwSize = 0;
	void * buffer = 0;

	*ipaddress = 0;

	if (DPLAY->GetPlayerAddress(HOSTID, NULL, &dwSize) != DPERR_BUFFERTOOSMALL || dwSize==0)
		goto Done;

	buffer = malloc(dwSize);
	if (DPLAY->GetPlayerAddress(HOSTID, buffer, &dwSize) != DP_OK)
		goto Done;

	AddressEnumStruct aEnum;
	aEnum.ipaddress = ipaddress;
	aEnum.ipaddress2 = ipaddress2;
	aEnum.bOnTwo = false;

	if (DPLOBBY->EnumAddress(EnumAddressCallback, buffer, dwSize, &aEnum) != DP_OK)
		goto Done;

	result = wcslen(ipaddress);

Done:
	free(buffer);
	return result;
}
//----------------------------------------------------------------------------------
//--------------------------End NetConnect.cpp--------------------------------------
//----------------------------------------------------------------------------------
