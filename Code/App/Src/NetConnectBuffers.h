#ifndef NETCONNECTBUFFERS_H
#define NETCONNECTBUFFERS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                          NetConnectBuffers.h                             //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/NetConnectBuffers.h 5     3/18/99 6:14p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef __DPLAY_INCLUDED__
#include <dplay.h>
#endif

//---------------------------------------------------------------------------
//
struct SAVED_CONNECTION
{
	DWORD    dwSize;		// amount of memory allocated for structure (not sizeof())
	GUID     guidSP;		// guid of service provider
	LPVOID   lpConnection;
	DWORD    dwConnectionSize;
	wchar_t  name[1];
};
//---------------------------------------------------------------------------
//
struct CONN_BUFFER
{
	void   * buffer;
	DWORD 	 dwSize;
	DWORD    dwUsed;

	SAVED_CONNECTION * create (const GUID & guid, LPVOID lpData, DWORD dwDataSize, LPCDPNAME pName);

	SAVED_CONNECTION * verify (SAVED_CONNECTION * saved) const
	{
		if (saved->dwSize == 0)
			return 0;
		return saved;
	}

	SAVED_CONNECTION * getNext (SAVED_CONNECTION *saved) const
	{
		if (saved == 0)
			return verify((SAVED_CONNECTION *)buffer);
		else
			return verify((SAVED_CONNECTION *) (((C8 *)saved) + saved->dwSize));
	}

	SAVED_CONNECTION * findConnection (S32 index) const
	{
		SAVED_CONNECTION * result = getNext(0);

		while (result && index-- > 0)
			result = getNext(result);

		return result;
	}

};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
struct SAVED_SESSION
{
    DWORD   dwSize;		// amount of memory actually allocated for the struct
    DWORD   dwFlags;
    GUID    guidInstance;
    DWORD   dwMaxPlayers;
    DWORD   dwCurrentPlayers;
	DWORD	dwUser1;		// order in memory important here!!!
	DWORD	dwUser2;
    wchar_t sessionName[1];
};
//---------------------------------------------------------------------------
//
struct SESSION_BUFFER
{
	void   * buffer;
	DWORD 	 dwSize;
	DWORD    dwUsed;
	UINT     uTimerID;
	bool	 bEnumStopped;

	~SESSION_BUFFER (void)
	{
		if (bEnumStopped==false)
			StopEnumSessions();
	}

	SAVED_SESSION * create (DWORD dwFlags, const GUID & guid, DWORD dwMaxPlayers, DWORD dwCurrentPlayers, DWORD dwUser1, DWORD dwUser2, LPWSTR pName);

	SAVED_SESSION * verify (SAVED_SESSION * saved)
	{
		if (saved->dwSize == 0)
			return 0;
		return saved;
	}

	SAVED_SESSION * getNext (SAVED_SESSION *saved)
	{
		if (saved == 0)
			return verify((SAVED_SESSION *)buffer);
		else
			return verify((SAVED_SESSION *) (((C8 *)saved) + saved->dwSize));
	}

	SAVED_SESSION * findSession (S32 index)
	{
		SAVED_SESSION * result = getNext(0);

		while (result && index-- > 0)
			result = getNext(result);

		return result;
	}

	
	
	static SESSION_BUFFER * StartEnumSessions (void);

	HRESULT ContinueEnumSessions (void);

	HRESULT StopEnumSessions (void);
};
//---------------------------------------------------------------------------
//
struct SAVED_PLAYER
{
	DWORD    dwSize;		// amount of memory allocated for structure (not sizeof())
	DPID     dpid;			// dpid of player
	wchar_t  name[1];		// name of the player
};
//---------------------------------------------------------------------------
//
struct PLAYER_BUFFER
{
	void   * buffer;
	DWORD 	 dwSize;
	DWORD    dwUsed;

	SAVED_PLAYER * create (DPID dpid, LPCDPNAME pName);

	SAVED_PLAYER * verify (SAVED_PLAYER * saved) const
	{
		if (saved->dwSize == 0)
			return 0;
		return saved;
	}

	SAVED_PLAYER * getNext (SAVED_PLAYER *saved) const
	{
		if (saved == 0)
			return verify((SAVED_PLAYER *)buffer);
		else
			return verify((SAVED_PLAYER *) (((C8 *)saved) + saved->dwSize));
	}

	SAVED_PLAYER * findConnection (S32 index) const
	{
		SAVED_PLAYER * result = getNext(0);

		while (result && index-- > 0)
			result = getNext(result);

		return result;
	}
};
//---------------------------------------------------------------------------
//
CONN_BUFFER * __stdcall EnumConnections (void);		// create a list of connections
PLAYER_BUFFER * __stdcall EnumPlayers (const GUID & guidSession);		// create a list of players




//----------------------------------------------------------------------------------
//--------------------------End NetConnectBuffers.h---------------------------------
//----------------------------------------------------------------------------------
#endif