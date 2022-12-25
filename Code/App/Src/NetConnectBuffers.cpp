//--------------------------------------------------------------------------//
//                                                                          //
//                          NetConnectBuffers.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/NetConnectBuffers.cpp 12    9/07/00 4:01p Sbarton $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "CQTrace.h"
#include "Resource.h"
#include "Cursor.h"

#include <HeapObj.h>

#include <dplobby.h>
#include <dplay.h>
#include "NetConnectBuffers.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#define BUFFER_SIZE 4096
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
SAVED_CONNECTION * CONN_BUFFER::create (const GUID & guid, 
										LPVOID lpData, 
										DWORD dwDataSize, 
										LPCDPNAME pName)
{
	SAVED_CONNECTION * result = 0;
	DWORD dwLen = (((wcslen(pName->lpszShortName) + 1)*2)+3) & ~3;
	DWORD dwNewSize = (sizeof(SAVED_CONNECTION) + dwDataSize + dwLen + 3) & ~3;

	if (dwUsed + dwNewSize + 4 > dwSize)
		goto Done;
	result = (SAVED_CONNECTION *) (((C8 *)buffer)+dwUsed);
	dwUsed += dwNewSize;
	//
	// fill in the data
	//
	CQASSERT(result->dwSize==0);		// else there is memory corruption going on
	result->dwSize = dwNewSize;
	result->guidSP = guid;
	result->lpConnection = (LPVOID) (result->name + (dwLen/sizeof(result->name[0])));
	memcpy(result->lpConnection, lpData, dwDataSize);
	result->dwConnectionSize = dwDataSize;
	memcpy(result->name, pName->lpszShortName, dwLen);

Done:
	return result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
SAVED_SESSION * SESSION_BUFFER::create (DWORD dwFlags, 
										const GUID & guid, 
										DWORD dwMaxPlayers, 
										DWORD dwCurrentPlayers,
										DWORD dwUser1,
										DWORD dwUser2,
										LPWSTR pName)
{
	SAVED_SESSION * result = 0;
	DWORD dwLen = (((wcslen(pName) + 1)*2) + 3) & ~3;
	DWORD dwNewSize = (sizeof(SAVED_SESSION) + dwLen + 4 + 3) & ~3;	// extra 4 bytes

	if (dwUsed + dwNewSize + 4 > dwSize)
		goto Done;
	result = (SAVED_SESSION *) (((C8 *)buffer)+dwUsed);
	dwUsed += dwNewSize;
	//
	// fill in the data
	//
	result->dwSize = dwNewSize;
	result->dwFlags = dwFlags;
    result->guidInstance = guid;
	result->dwMaxPlayers = dwMaxPlayers;
	result->dwCurrentPlayers = dwCurrentPlayers;
	result->dwUser1 = dwUser1;
	result->dwUser2 = dwUser2;
	memcpy(result->sessionName, pName, dwLen);

Done:
	return result;
}
//---------------------------------------------------------------------------
// save the items from the enumeration into a local list
static BOOL FAR PASCAL EnumConnectionsCallback(
	LPCGUID lpguidSP,
	LPVOID lpConnection,
	DWORD dwConnectionSize,
	LPCDPNAME lpName,
	DWORD dwFlags,
	LPVOID lpContext)
{
	CONN_BUFFER * connBuffer = (CONN_BUFFER *) lpContext;
	LPDIRECTPLAY4 lpDP4;
	BOOL result = 0;

	//
	// create a temporary directPlay provider
	//

	if (CreateDirectPlayInterface(&lpDP4) != DP_OK)
		goto Done;	// discontinue enum

	if (lpDP4->InitializeConnection(lpConnection, 0) == DP_OK)
	{
		//
		// connection is possible
		//

		if (connBuffer->create(*lpguidSP, lpConnection, dwConnectionSize, lpName))
			result = 1;
	}
	else
		result = 1;

	lpDP4->Release();

Done:
	return result;
}
//---------------------------------------------------------------------------
//
CONN_BUFFER * __stdcall EnumConnections (void)
{
	CONN_BUFFER * result = (CONN_BUFFER *) calloc(BUFFER_SIZE + sizeof(CONN_BUFFER), 1);
	
	result->buffer = (result+1);
	result->dwSize = BUFFER_SIZE;
	result->dwUsed = 0;
	
	DPLAY->EnumConnections( &APPGUID_CONQUEST, EnumConnectionsCallback, result, DPCONNECTION_DIRECTPLAY);

	return result;
}
//---------------------------------------------------------------------------
// save the items from the enumeration into a local list
//
static BOOL FAR PASCAL EnumSessionsCallback2(
	LPCDPSESSIONDESC2 lpThisSD,
	LPDWORD lpdwTimeOut,
	DWORD dwFlags,
	LPVOID lpContext)
{
	SESSION_BUFFER * sessionBuffer = (SESSION_BUFFER *) lpContext;
	BOOL result = 0;

	if (sessionBuffer && lpThisSD && sessionBuffer->create(lpThisSD->dwFlags, lpThisSD->guidInstance, lpThisSD->dwMaxPlayers, lpThisSD->dwCurrentPlayers, lpThisSD->dwUser1, lpThisSD->dwUser2, lpThisSD->lpszSessionName))
		result = 1;

	return result;
}
//---------------------------------------------------------------------------
//
HRESULT SESSION_BUFFER::ContinueEnumSessions (void)
{
	DPSESSIONDESC2 desc;
	HRESULT hresult=DP_OK;

	if (!bEnumStopped)
	{
		
		buffer = (this+1);
		dwSize = BUFFER_SIZE;
		dwUsed = 0;
		
		memset(buffer, 0, BUFFER_SIZE);
		memset(&desc, 0, sizeof(desc));
		desc.dwSize = sizeof(desc);
		desc.guidApplication = APPGUID_CONQUEST;
		
		hresult = DPLAY->EnumSessions(&desc, 0, EnumSessionsCallback2, this, DPENUMSESSIONS_ALL | DPENUMSESSIONS_ASYNC);
	}

	return hresult;
}
//---------------------------------------------------------------------------
//
SESSION_BUFFER * SESSION_BUFFER::StartEnumSessions (void)
{
	SESSION_BUFFER * result = (SESSION_BUFFER *) calloc(BUFFER_SIZE + sizeof(SESSION_BUFFER), 1);
	DPSESSIONDESC2 desc;
	HRESULT hresult;
		 
	result->buffer = (result+1);
	result->dwSize = BUFFER_SIZE;
	result->dwUsed = 0;

	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.guidApplication = APPGUID_CONQUEST;

	CURSOR->SetBusy(1);
	hresult = DPLAY->EnumSessions(&desc, 0, EnumSessionsCallback2, 0, DPENUMSESSIONS_ALL | DPENUMSESSIONS_ASYNC);
	CURSOR->SetBusy(0);

	if (hresult != DP_OK && hresult != DPERR_CONNECTING)
	{
		free(result);
		result = 0;

		if (hresult != DPERR_USERCANCEL)
		{
			CQMessageBox(IDS_HELP_JOINFAILED, IDS_APP_NAMETM, MB_OK);
		}
	}

	return result;
}
//---------------------------------------------------------------------------
//
HRESULT SESSION_BUFFER::StopEnumSessions (void)
{
	DPSESSIONDESC2 desc;
	HRESULT hresult = DP_OK;

	if (bEnumStopped == false)
	{
		memset(&desc, 0, sizeof(desc));
		desc.dwSize = sizeof(desc);
		desc.guidApplication = APPGUID_CONQUEST;

		hresult = DPLAY->EnumSessions(&desc, 0, EnumSessionsCallback2, 0, DPENUMSESSIONS_ALL | DPENUMSESSIONS_STOPASYNC);
		bEnumStopped = true;
	}

	return hresult;
}
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//
SAVED_PLAYER * PLAYER_BUFFER::create (DPID dpid, LPCDPNAME pName)
{
	SAVED_PLAYER * result = 0;
	DWORD dwLen = (((wcslen(pName->lpszShortName) + 1)*2)+3) & ~3;
	DWORD dwNewSize = (sizeof(SAVED_PLAYER) + dwLen + 3) & ~3;

	if (dwUsed + dwNewSize + 4 > dwSize)
		goto Done;
	result = (SAVED_PLAYER *) (((C8 *)buffer)+dwUsed);
	dwUsed += dwNewSize;
	//
	// fill in the data
	//
	CQASSERT(result->dwSize==0);		// else there is memory corruption going on
	result->dwSize = dwNewSize;
	result->dpid = dpid;
	memcpy(result->name, pName->lpszShortName, dwLen);

Done:
	return result;
}
//----------------------------------------------------------------------------------
//
static BOOL CALLBACK enumPlayersCallback (DPID dpId, DWORD dwPlayerType, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext)
{
	PLAYER_BUFFER * playerBuffer = (PLAYER_BUFFER *) lpContext;
	playerBuffer->create(dpId, lpName);
	return 1;
}
//----------------------------------------------------------------------------------
//
PLAYER_BUFFER * __stdcall EnumPlayers (const GUID & guidSession)
{
	PLAYER_BUFFER * result = (PLAYER_BUFFER *) calloc(BUFFER_SIZE + sizeof(PLAYER_BUFFER), 1);
	
	result->buffer = (result+1);
	result->dwSize = BUFFER_SIZE;
	result->dwUsed = 0;
	
	DPLAY->EnumPlayers( const_cast<GUID *>(&guidSession), enumPlayersCallback, result, DPENUMPLAYERS_REMOTE | DPENUMPLAYERS_SESSION);

	return result;
}

//----------------------------------------------------------------------------------
//--------------------------End NetConnectBuffers.cpp-------------------------------
//----------------------------------------------------------------------------------
