//--------------------------------------------------------------------------//
//                                                                          //
//                               CommTrack.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/CommTrack.cpp 9     9/30/00 10:52p Jasony $
*/			    
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include "CommTrack.h"
#include "Startup.h"
#include "ObjSet.h"
#include "UserDefaults.h"
#include "ObjList.h"
#include "MPart.h"
#include "OpAgent.h"

#include <Heapobj.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <TComponent.h>
#include <IConnection.h>

#include <commctrl.h>

#define TRACKING_LISTS  256

#define DEF_NODE_LIFETIME   20000		// 20 seconds
#define DEF_COMMAND_LIMIT   10			// 10 commands in 20 seconds?
#define TRACK_VERSION 2

static char szRegKey[] = "CommTrackData";
//----------------------------------------------------------------------------------------------
//
struct TrackingNode
{
	TrackingNode * next;

	PACKET_TYPE type;
	U32  dwMissionID;
	
	U32  timeStamp;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}


#define TRACKTYPECASE(x)   \
		case x:	\
			result = #x;	\
			break;
	

	static const char * getTypeName (PACKET_TYPE type)
	{
		const char * result = "?";

		switch (type)
		{
			TRACKTYPECASE(PT_USRMOVE);
			TRACKTYPECASE(PT_USRFORMATIONMOVE);
			TRACKTYPECASE(PT_FORMATIONATTACK);
			TRACKTYPECASE(PT_USRJUMP);
			TRACKTYPECASE(PT_USRATTACK);
			TRACKTYPECASE(PT_USRSPATTACK);
			TRACKTYPECASE(PT_USRWORMATTACK);
			TRACKTYPECASE(PT_USRAOEATTACK);
			TRACKTYPECASE(PT_USEARTIFACTTARGETED);
			TRACKTYPECASE(PT_USRPROBE);
			TRACKTYPECASE(PT_USRRECOVER);
			TRACKTYPECASE(PT_USRDROPOFF);
			TRACKTYPECASE(PT_USRMIMIC);
			TRACKTYPECASE(PT_USRCREATEWORMHOLE);
			TRACKTYPECASE(PT_USRSTOP);
			TRACKTYPECASE(PT_USRBUILD);
			TRACKTYPECASE(PT_USRFAB);
			TRACKTYPECASE(PT_USRFABJUMP);
			TRACKTYPECASE(PT_USRFABPOS);
			TRACKTYPECASE(PT_USRHARVEST);
			TRACKTYPECASE(PT_USRRALLY);
			TRACKTYPECASE(PT_USRESCORT);
			TRACKTYPECASE(PT_USRDOCKFLAGSHIP);
			TRACKTYPECASE(PT_USRUNDOCKFLAGSHIP);
			TRACKTYPECASE(PT_USRRESUPPLY);
			TRACKTYPECASE(PT_USRSHIPREPAIR);
			TRACKTYPECASE(PT_USRFABREPAIR);
			TRACKTYPECASE(PT_USRCAPTURE);
			TRACKTYPECASE(PT_USRSPABILITY);
			TRACKTYPECASE(PT_USRCLOAK);
			TRACKTYPECASE(PT_USR_EJECT_ARTIFACT);
			TRACKTYPECASE(PT_USRFABSALVAGE);
			TRACKTYPECASE(PT_USRKILLUNIT);
			TRACKTYPECASE(PT_PARTRENAME);
			TRACKTYPECASE(PT_NETTEXT);
			TRACKTYPECASE(PT_STANCECHANGE);
			TRACKTYPECASE(PT_SUPPLYSTANCECHANGE);
			TRACKTYPECASE(PT_HARVESTSTANCECHANGE);
			TRACKTYPECASE(PT_PATROL);
			TRACKTYPECASE(PT_FLEETDEF);
		}

		return result;
	}

	const char * getTypeName (void) const
	{
		return getTypeName(type);
	}
};
//----------------------------------------------------------------------------------------------
//
struct TRACKDATA
{
	U32 version;
	bool bEnabled;
	U32  NODE_LIFETIME;
	U32  COMMAND_LIMIT;
};
//----------------------------------------------------------------------------------------------
//
struct CommTrack : ICommTrack, IEventCallback, TRACKDATA
{
	// data items
	TrackingNode * trackingList[TRACKING_LISTS];
	TrackingNode * trackingListEnd[TRACKING_LISTS];
	U32 timeStamp;

	

	U32 eventHandle;
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(CommTrack)
	DACOM_INTERFACE_ENTRY(ICommTrack)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	~CommTrack();

	/* ICommTrack methods */

	virtual void AddCommand (PACKET_TYPE type, U32 objectID[MAX_SELECTED_UNITS], int numObjects);

	/* IEventCallback methods */

	virtual GENRESULT __stdcall Notify (U32 message, void *param);

	/* CommTrack methods */

	U32 getCommandCount (U32 dwMissionID);

	void reset (void);

	void addCommand (PACKET_TYPE type, U32 dwMissionID);

	void update (U32 dt);

	static void CommTrack::printCmdQueue (U32 dwMissionID, TrackingNode * node, char *buffer);

	IDAComponent * getBase (void)
	{
		return static_cast<ICommTrack *>(this);
	}


	static BOOL CALLBACK dlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
};
//----------------------------------------------------------------------------------------------
//
CommTrack::~CommTrack (void)
{
	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	DEFAULTS->SetDataInRegistry(szRegKey, static_cast<TRACKDATA *>(this), sizeof(TRACKDATA));
}
//----------------------------------------------------------------------------------------------
//
GENRESULT CommTrack::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_UPDATE:
		update(U32(param) >> 10);
		break;

	case CQE_MISSION_CLOSE:
		if (param!=0)
			reset();
		break;

	case WM_COMMAND:
		switch (LOWORD(msg->wParam))
		{
		case IDM_COMMAND_TRACKING:
			DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG14), hMainWindow, dlgProc, (LPARAM) this);
			break;
		}
		break;  // end case WM_COMMAND
	}

	return GR_OK;
}
//----------------------------------------------------------------------------------------------
// remove all nodes that are too old
//
void CommTrack::update (U32 dt)
{
	int i;

	// need to scale dt if constant update is turned on
	USER_DEFAULTS * defaults = DEFAULTS->GetDefaults();
	if (defaults->bConstUpdateRate && defaults->bNoFrameLimit==0)
	{
		double gameSpeed = double(DEFAULTS->GetDefaults()->gameSpeed);
		double mul = pow(2, (gameSpeed * (1/5.0)));

		U32 newdt = dt * mul;

		if (newdt==0 && dt)
			dt = 1;
		else
			dt = newdt;
	}

	timeStamp += dt;
	
	for (i = 0; i < TRACKING_LISTS; i++)
	{
		TrackingNode * node;

		node = trackingList[i];
		while (node)
		{
			if (node->timeStamp + NODE_LIFETIME < timeStamp)
			{
				if ((trackingList[i] = node->next) == 0)
					trackingListEnd[i] = 0;
				delete node;
				node = trackingList[i];
			}
			else
				break;		// nothing to delete this time (list is in increasing order)
		}
	}
}
//----------------------------------------------------------------------------------------------
//
void CommTrack::AddCommand (PACKET_TYPE type, U32 objectID[MAX_SELECTED_UNITS], int numObjects)
{
	CQASSERT(THEMATRIX->IsMaster());
	if (bEnabled)
	{
		while (numObjects-- > 0)
		{
			if (getCommandCount(objectID[numObjects]) + 1 == COMMAND_LIMIT)
			{
				MPart part = OBJLIST->FindObject(objectID[numObjects]);
				const char * name = (part.isValid()) ? ((const char *)part->partName) : "???";
				char buffer[256];
				U32 key = (objectID[numObjects] >> 4) % TRACKING_LISTS;
				buffer[0] = 0;
				printCmdQueue(objectID[numObjects], trackingList[key], buffer);

				CQERROR3("Command thrashing detected for unit '%s' (Ignorable)\r\n   %s<-%s", name, TrackingNode::getTypeName(type), buffer);
				reset();
			}
			addCommand(type, objectID[numObjects]);
		}
	}
}
//----------------------------------------------------------------------------------------------
//
void CommTrack::printCmdQueue (U32 dwMissionID, TrackingNode * node, char *buffer)
{
	while (node)
	{
		if (node->dwMissionID == dwMissionID)
		{
			// recursion
			printCmdQueue(dwMissionID, node->next, buffer);
			if (buffer[0] != 0)
				strcat(buffer, "<-");
			strcat(buffer, node->getTypeName());
			break;
		}

		node = node->next;
	}
}
//----------------------------------------------------------------------------------------------
//
void CommTrack::reset (void)
{
	int i;

	timeStamp = 0;
	
	for (i = 0; i < TRACKING_LISTS; i++)
	{
		TrackingNode * node, * next;

		node = trackingList[i];
		while (node)
		{
			next = node->next;
			delete node;
			node = next;
		}
		trackingList[i] = trackingListEnd[i] = 0;
	}
}
//----------------------------------------------------------------------------------------------
//
void CommTrack::addCommand (PACKET_TYPE type, U32 dwMissionID)
{
	TrackingNode * node = new TrackingNode;
	U32 key = (dwMissionID >> 4) % TRACKING_LISTS;

	node->dwMissionID = dwMissionID;
	node->type = type;
	node->next = 0;
	node->timeStamp = timeStamp;
	if (trackingListEnd[key] == 0)
	{
		CQASSERT(trackingList[key] == 0);
		trackingList[key] = trackingListEnd[key] = node;
	}
	else	// add to the end of the list
	{
		trackingListEnd[key]->next = node;
		trackingListEnd[key] = node;
	}
}
//----------------------------------------------------------------------------------------------
//
U32 CommTrack::getCommandCount (U32 dwMissionID)
{
	U32 key = (dwMissionID >> 4) % TRACKING_LISTS;
	TrackingNode * node = trackingList[key];
	U32 result=0;

	while (node)
	{
		if (node->dwMissionID == dwMissionID)
			result++;
		node = node->next;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL CommTrack::dlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	CommTrack * track = (CommTrack *) GetWindowLong(hwnd, DWL_USER);
//	NM_UPDOWN * pnmud;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hItem;
			SetWindowLong(hwnd, DWL_USER, lParam);
			track = (CommTrack *) GetWindowLong(hwnd, DWL_USER);

			SetDlgItemInt(hwnd, IDC_EDIT1, track->NODE_LIFETIME / 1000, 0);
			hItem = GetDlgItem(hwnd, IDC_SPIN1);
			SendMessage(hItem, UDM_SETRANGE, 0, MAKELONG(60,1));			// from 1 to 60

			SetDlgItemInt(hwnd, IDC_EDIT2, track->COMMAND_LIMIT, 0);
			hItem = GetDlgItem(hwnd, IDC_SPIN3);
			SendMessage(hItem, UDM_SETRANGE, 0, MAKELONG(60,1));			// from 1 to 60

			if (track->bEnabled)
			{
				CheckDlgButton(hwnd, IDC_CHECK1, BST_CHECKED);
				SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
			}
			else
			{
				SetFocus(GetDlgItem(hwnd, IDC_CHECK1));
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT1),0);
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT2),0);
				EnableWindow(GetDlgItem(hwnd, IDC_SPIN1),0);
				EnableWindow(GetDlgItem(hwnd, IDC_SPIN3),0);
			}
		}
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHECK1:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (IsDlgButtonChecked(hwnd, IDC_CHECK1) == BST_CHECKED)
				{
					EnableWindow(GetDlgItem(hwnd, IDC_SPIN1),1);
					EnableWindow(GetDlgItem(hwnd, IDC_EDIT1),1);
					EnableWindow(GetDlgItem(hwnd, IDC_SPIN3),1);
					EnableWindow(GetDlgItem(hwnd, IDC_EDIT2),1);
				}
				else
				{
					EnableWindow(GetDlgItem(hwnd, IDC_SPIN1),0);
					EnableWindow(GetDlgItem(hwnd, IDC_EDIT1),0);
					EnableWindow(GetDlgItem(hwnd, IDC_SPIN3),0);
					EnableWindow(GetDlgItem(hwnd, IDC_EDIT2),0);
				}
			}
			break;

		case IDOK:
			{
				int result;

				result = GetDlgItemInt(hwnd, IDC_EDIT1, 0, 0);
				if (result)
					track->NODE_LIFETIME = result * 1000;
				result = GetDlgItemInt(hwnd, IDC_EDIT2, 0, 0);
				track->COMMAND_LIMIT = result;
				track->bEnabled = (IsDlgButtonChecked(hwnd, IDC_CHECK1) == BST_CHECKED);
			}
			// fall through intentional
		case IDCANCEL:
			EndDialog(hwnd, 1);
			break;
		}
	}

	return result;
}
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//
struct _commtrack : GlobalComponent
{
	CommTrack * ct;

	virtual void Startup (void)
	{
		COMMTRACK = ct = new DAComponent<CommTrack>;
		AddToGlobalCleanupList(&COMMTRACK);
	}
	
	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(ct->getBase(), &ct->eventHandle);

		if (DEFAULTS->GetDataFromRegistry(szRegKey, static_cast<TRACKDATA *>(ct), sizeof(TRACKDATA)) != sizeof(TRACKDATA) || ct->version != TRACK_VERSION)
		{
			ct->version = TRACK_VERSION;
			ct->bEnabled = false;//default this to off unless we are debugging and update problem.
			ct->NODE_LIFETIME = DEF_NODE_LIFETIME;
			ct->COMMAND_LIMIT = DEF_COMMAND_LIMIT;
		}
	}
};

static _commtrack commtrack;
//----------------------------------------------------------------------------
//-------------------------END CommTrack.cpp-------------------------------------
//----------------------------------------------------------------------------