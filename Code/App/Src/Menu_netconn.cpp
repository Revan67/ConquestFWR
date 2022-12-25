//--------------------------------------------------------------------------//
//                                                                          //
//                            Menu_netconn.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_netconn.cpp 39    6/26/01 5:06p Tmauer $
*/
//--------------------------------------------------------------------------//
// network connection options
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include <wchar.h>
#include <DMenu1.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IEdit2.h"
#include "IListbox.h"
#include "MusicManager.h"

#include "NetConnectBuffers.h"
#include "UserDefaults.h"

#include "ZoneLobby.h"
#include <shellapi.h>

#define MAX_PLAYER_CHARS 32
#define NAME_REG_KEY   "CQPlayerName"

U32 __stdcall DoMenu_sess (Frame * parent, const GT_MENU1 & data, const SAVED_CONNECTION * conn, U32 flags, wchar_t * szPlayerName);
U32 __stdcall DoMenu_zone (Frame * parent, const GT_MENU1 & data);

// flags for next menu
#define SESSFLAG_LAN	  0x00000001
#define SESSFLAG_TCP	  0x00000002
#define SESSFLAG_MODEM	  0x00000004
#define SESSFLAG_SERIAL	  0x00000008
#define SESSFLAG_2PLAYER  0x00000010
//--------------------------------------------------------------------------//
//
struct Menu_nc : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1 & menu1;
	const GT_MENU1::NET_CONNECTIONS & data;

	COMPTR<IStatic> background, description;
	COMPTR<IListbox> list;
	COMPTR<IButton2> next, back;

	COMPTR<IStatic>  staticTitle;
	COMPTR<IButton2> buttonZone,buttonWeb;

	wchar_t szPlayerName[MAX_PLAYER_CHARS];

	//
	// net connection data
	//
	bool bLobbied;
	CONN_BUFFER *conn;

	S32 currentSelection;
	//
	// instance methods
	//

	Menu_nc (Frame * _parent, const GT_MENU1 & _data) : data(_data.netConnections), menu1(_data)
	{
		initializeFrame(_parent);
		init();
	}

	~Menu_nc (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* Menu_nc methods */

	virtual void setStateInfo (void);

	virtual void onListSelection (S32 listID)		// user has selected a list item
	{
		SetVisible(false);
		U32 id = list->GetDataValue(list->GetCurrentSelection());
//		if (id == IDS_ZONEGAME)
//		{
//			U32 result;
//			if ((result = DoMenu_zone(this, menu1)) != 0)
//				endDialog(result);
//		}
//		else
		if (DoMenu_sess(this, menu1, getSavedConn(id), getFlags(id), szPlayerName))
			endDialog(1);

		SetVisible(true);
		setFocus(list);
	}

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case IDS_BACK:
			endDialog(0);
			break;

		case IDS_NEXT:
			onListSelection(0);
			break;

		case IDS_ZONEGAME:
			onZoneSelection();
			break;

		case IDS_WEB_BUTTON:
			onWebSelection();
			break;
		}
	}

	virtual void onListCaretMove (S32 listID)		// user has moved the caret
	{
		S32 sel = list->GetCurrentSelection();
		if (sel != currentSelection)
		{
			next->EnableButton(true);
			currentSelection = sel;
			setDescriptionForConnection(list->GetDataValue(sel));
		}
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		onButtonPressed(IDS_BACK);
		return true;
	}

	void onZoneSelection (void)
	{
		SetVisible(false);
		U32 result;
		if ((result = DoMenu_zone(this, menu1)) != 0)
			endDialog(result);

		SetVisible(true);
		setFocus(list);
	}


	void onWebSelection (void)
	{	
		ShowWindow(hMainWindow, SW_MINIMIZE);

		SHELLEXECUTEINFO info;
		memset(&info, 0, sizeof(info));

		info.cbSize = sizeof(info);
		info.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
		info.lpVerb = "open";
		info.lpFile = "http://www.ubisoft.com/conquestfrontierwars";
		info.nShow = SW_SHOWNORMAL;

		ShellExecuteEx(&info);	
	}
	void init (void);
	void initNet (void);

	// find the connection within the connection buffer
	SAVED_CONNECTION * getSavedConn (U32 resourceID);

	void setDescriptionForConnection (U32 connection)
	{
		switch (connection)
		{
		case IDS_TCP_LAN:
			description->SetText(_localLoadStringW(IDS_HELP_TCP_LAN));
			break;
		case IDS_INTERNETGAME:
			description->SetText(_localLoadStringW(IDS_HELP_INTERNET));
			break;
		case IDS_NETWORKGAME:
			description->SetText(_localLoadStringW(IDS_HELP_NETWORK));
			break;
		case IDS_MODEMGAME:
			description->SetText(_localLoadStringW(IDS_HELP_MODEM));
			break;
		case IDS_SERIALGAME:
			description->SetText(_localLoadStringW(IDS_HELP_SERIAL));
			break;
		case IDS_ZONEGAME:
			description->SetText(_localLoadStringW(IDS_HELP_ZONE));
			break;
		case IDS_WEB_BUTTON:
			description->SetText(_localLoadStringW(IDS_HELP_WEB));
			break;
		default:
			description->SetText(_localLoadStringW(IDS_HELP_UNK_PROVIDER));
			break;
		}
	}

	virtual void onFocusChanged (void)
	{
		if (focusControl!=0)
		{
			S32 id = focusControl->GetControlID();

			switch (id)
			{
			case IDS_BACK:
				description->SetText(_localLoadStringW(IDS_HELP_GOBACK));
				break;

			case IDS_ZONEGAME:
				setDescriptionForConnection(IDS_ZONEGAME);
				break;

			default:	// assume it's the listbox
				{
					S32 sel = list->GetCurrentSelection();
					if (sel >= 0)
						setDescriptionForConnection(list->GetDataValue(sel));
					else
						description->SetText(_localLoadStringW(IDS_HELP_NETCONNECT));
				}
			}
		}
		else
			description->SetText(NULL);
	}

	U32 getFlags (U32 resourceID)
	{
		U32 result = 0;

		switch (resourceID)
		{
		case IDS_TCP_LAN:
			result = SESSFLAG_LAN | SESSFLAG_TCP;
			break;
		case IDS_INTERNETGAME:
			result = SESSFLAG_TCP;
			break;
		case IDS_NETWORKGAME:
			result = SESSFLAG_LAN;
			break;
		case IDS_MODEMGAME:
			result = SESSFLAG_MODEM | SESSFLAG_2PLAYER;
			break;
		case IDS_SERIALGAME:
			result = SESSFLAG_SERIAL | SESSFLAG_2PLAYER;
			break;
		}

		return result;
	}

	bool setMPSaveDir (void);
};
//----------------------------------------------------------------------------------//
//
Menu_nc::~Menu_nc (void)
{
	delete conn;
}
//--------------------------------------------------------------------------//
//
bool Menu_nc::setMPSaveDir (void)
{
	char buffer[256];
	char name[128];

	U32 result = DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, name, sizeof(name));
	CQASSERT(result && "We Must have a Multi Player Name set by now");
	
	wsprintf(buffer, "SavedGame\\%s", name);
	_localAnsiToWide(name, szPlayerName, sizeof(szPlayerName));

	DAFILEDESC fdesc = buffer;

	if (SAVEDIR)
	{
		SAVEDIR->Release();
		SAVEDIR = 0;
	}

	if (DACOM->CreateInstance(&fdesc, (void **)&SAVEDIR) != GR_OK)
	{
		CQERROR1("Could not create save directory '%s'", fdesc.lpFileName);
		return false;
	}

	return true;
}
//----------------------------------------------------------------------------------//
//
void Menu_nc::setStateInfo (void)
{
	//screenRect = data.screenRect;
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	//
	// initialize in draw-order
	//

	background->InitStatic(data.background, this);
	description->InitStatic(data.description, this);
	buttonZone->InitButton(data.buttonZone, this);
	buttonWeb->InitButton(data.buttonWeb,this);
	list->InitListbox(data.list, this);
	back->InitButton(data.back, this);
	next->InitButton(data.next, this);
	staticTitle->InitStatic(data.staticTitle, this);

	buttonZone->SetTransparent(true);

	setMPSaveDir();

	if (childFrame)
		childFrame->setStateInfo();
	else
		setFocus(buttonZone);
}
//--------------------------------------------------------------------------//
//
void Menu_nc::initNet (void)
{
	S32 lobbied;
	
	CURSOR->SetBusy(1);

	StartNetConnection(lobbied);

	CQASSERT(lobbied == 0 && DPLAY);

	{
		SAVED_CONNECTION *connection = NULL;
		S32 index, count=0;

//		index = list->AddString(_localLoadStringW(IDS_ZONEGAME));	
//		list->SetDataValue(index, IDS_ZONEGAME);

		conn = EnumConnections();

		while ((connection = conn->getNext(connection)) != 0)
		{
			if (IsEqualGUID(connection->guidSP,DPSPGUID_TCPIP))
			{
				index = list->AddString(_localLoadStringW(IDS_TCP_LAN));	// TCP
				list->SetDataValue(index, IDS_TCP_LAN);
				index = list->AddString(_localLoadStringW(IDS_INTERNETGAME));	// TCP
				list->SetDataValue(index, IDS_INTERNETGAME);
			}
			else
			if (IsEqualGUID(connection->guidSP,DPSPGUID_IPX))
			{
				index = list->AddString(_localLoadStringW(IDS_NETWORKGAME));	// IPX
				list->SetDataValue(index, IDS_NETWORKGAME);
			}
			else
			if (IsEqualGUID(connection->guidSP,DPSPGUID_MODEM))
			{
				if (CQFLAGS.bLimitDPConnections == 0)
				{
					index = list->AddString(_localLoadStringW(IDS_MODEMGAME));		// modem (2 player)
					list->SetDataValue(index, IDS_MODEMGAME);
				}
			}
			else
			if (IsEqualGUID(connection->guidSP,DPSPGUID_SERIAL))
			{
				if (CQFLAGS.bLimitDPConnections == 0)
				{
					index = list->AddString(_localLoadStringW(IDS_SERIALGAME));	// direct modem (2 player)
					list->SetDataValue(index, IDS_SERIALGAME);
				}
			}
			else
			{
				if (CQFLAGS.bLimitDPConnections == 0)
				{
					CQTRACE10("Adding non-standard dplay provider to list.");
					index = list->AddString(connection->name);
					list->SetDataValue(index, -count);
				}
			}
			count++;
		}

//		for (int i = 0; i < 8; i++)
//		{
//			list->AddString(L"Testing the scrollbar");
//		}
	}

	CURSOR->SetBusy(0);
}
//--------------------------------------------------------------------------//
//
SAVED_CONNECTION * Menu_nc::getSavedConn (U32 resourceID)
{
	const GUID * pGUID = 0;
	SAVED_CONNECTION *connection = NULL;

	switch (resourceID)
	{
	case IDS_TCP_LAN:
	case IDS_INTERNETGAME:
		pGUID = &DPSPGUID_TCPIP;
		break;
	case IDS_NETWORKGAME:
		pGUID = &DPSPGUID_IPX;
		break;
	case IDS_MODEMGAME:
		pGUID = &DPSPGUID_MODEM;
		break;
	case IDS_SERIALGAME:
		pGUID = &DPSPGUID_SERIAL;
		break;
	default:
		{
			S32 count =  -S32(resourceID);
			while ((connection = conn->getNext(connection)) != 0)
			{
				if (count-- <= 0)
					break;
			}
			return connection;
		}
		break;
	}

	while ((connection = conn->getNext(connection)) != 0)
	{
		if (IsEqualGUID(connection->guidSP,*pGUID))
			break;
	}

	return connection;
}
//--------------------------------------------------------------------------//
//
void Menu_nc::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.description.staticType, pComp);
	pComp->QueryInterface("IStatic", description);

	GENDATA->CreateInstance(data.list.listboxType, pComp);
	pComp->QueryInterface("IListbox", list);

	GENDATA->CreateInstance(data.next.buttonType, pComp);
	pComp->QueryInterface("IButton2", next);

	GENDATA->CreateInstance(data.back.buttonType, pComp);
	pComp->QueryInterface("IButton2", back);

	GENDATA->CreateInstance(data.buttonZone.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonZone);

	GENDATA->CreateInstance(data.buttonWeb.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonWeb);

	GENDATA->CreateInstance(data.staticTitle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticTitle);

	initNet();

	currentSelection = -1;
	description->SetText(_localLoadStringW(IDS_HELP_NETCONNECT));
	next->EnableButton(false);

	setStateInfo();

	if (CQFLAGS.bInsideOutZoneLaunch)
		this->PostMessage(CQE_BUTTON, (void *)IDS_ZONEGAME);
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_nc (Frame * parent, const GT_MENU1 & data)
{
	Menu_nc * dlg = new Menu_nc(parent, data);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	return result;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_netconn.cpp-------------------------//
//--------------------------------------------------------------------------//