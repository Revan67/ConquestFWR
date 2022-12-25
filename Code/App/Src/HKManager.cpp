//--------------------------------------------------------------------------//
//                                                                          //
//                               HKManager.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/HKManager.cpp 9     4/17/00 8:34a Jasony $
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "Startup.h"
#include "CQTrace.h"
#include "Resource.h"

#include <FileSys.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TComponent.h>
#include <WindowManager.h>
#include <HKEvent.h>
#include <MemFile.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE HKManager : IEventMessageFilter, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(HKManager)
	DACOM_INTERFACE_ENTRY(IEventMessageFilter)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()



 	//------------------------
	//
	U32 eventHandle, filterHandle;
	bool bDebugEnabled;

	HKManager (void);

	~HKManager (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEventMessageFilter methods */

	GENRESULT __stdcall PreMessageFilter (S32 * hwnd,    
                                 S32 * message,    
                                 S32 * wParam,
                                 S32 * lParam);

	GENRESULT __stdcall PostMessageFilter (S32   hwnd,    
                                 S32   message,    
                                 S32   wParam,
                                 S32   lParam);


	/* IEventCallback methods */

	GENRESULT __stdcall Notify (U32 message, void *param);

	/* HKManager methods */

	static void createHotkeyTable (U32 resID, U32 eventID, IHotkeyEvent ** hkevent);

	void init (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventMessageFilter *>(this);
	}
};
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
HKManager::HKManager (void)
{
}
//--------------------------------------------------------------------------//
//
HKManager::~HKManager (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GS && GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	if (DBHOTKEY && DBHOTKEY->QueryOutgoingInterface("IEventMessageFilter", connection) == GR_OK)
		connection->Unadvise(filterHandle);
}
//--------------------------------------------------------------------------//
//
GENRESULT HKManager::PreMessageFilter (S32 * hwnd, S32 * message, S32 * wParam, S32 * lParam)
{
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT HKManager::PostMessageFilter (S32 hwnd, S32 message, S32 wParam, S32 lParam)
{
	HOTKEY->SystemMessage(hwnd, message, wParam, lParam);
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT HKManager::Notify (U32 message, void *param)
{
	MSG * msg = (MSG *) param;
	
	if (message == WM_SYSKEYDOWN && msg->wParam == VK_F10)
		message = WM_KEYDOWN;
	if (message == WM_SYSKEYUP)
	{
		if (msg->wParam == VK_F10)
			message = WM_KEYUP;
	}


	if (message <= WM_USER && param && DBHOTKEY && (message != WM_SYSKEYDOWN || GetMenu(hMainWindow)==0))
	{
		if (bDebugEnabled)
			DBHOTKEY->SystemMessage((LONG)msg->hwnd, message, msg->wParam, msg->lParam);
		else
			HOTKEY->SystemMessage((LONG)msg->hwnd, message, msg->wParam, msg->lParam);
	}

	return GR_OK;
}
//--------------------------------------------------------------------------
//
void HKManager::createHotkeyTable (U32 resID, U32 eventID, IHotkeyEvent ** hkevent)
{
	COMPTR<IFileSystem> pFile;
	COMPTR<IDAConnectionPoint> connection;
	HKEVENTDESC hkdesc;
	HRSRC hRes;

	*hkevent =0;

	if ((hRes = FindResource(hResource, MAKEINTRESOURCE(resID), "DAHOTKEY")) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hResource, hRes)) != 0)
		{
			LPVOID pData;

			if ((pData = LockResource(hGlobal)) != 0)
			{
				MEMFILEDESC mdesc = "hotkey file";
 				mdesc.lpBuffer = pData;
				mdesc.dwBufferSize = SizeofResource(hResource, hRes);
				mdesc.dwFlags = CMF_DONT_COPY_MEMORY;

				CreateUTFMemoryFile(mdesc, pFile);
			}
		}
	}

	if (pFile == 0)
	{
	 	CQBOMB0("Could not open hotkey file.");
	}

	hkdesc.file = pFile;
	hkdesc.hotkeyMessage = eventID;
	hkdesc.joyMessage = CQE_JOYSTICK;
	
	if (DACOM->CreateInstance(&hkdesc, (void **) hkevent) != GR_OK)
	{
		CQBOMB0("Could not start hotkey system");
	}

	if ((*hkevent)->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		U32 handle;
		connection->Advise(EVENTSYS, &handle);
	}
}
//--------------------------------------------------------------------------//
//
void HKManager::init (void)
{
	COMPTR<IDAConnectionPoint> connection;
	COMPTR<IProfileParser> parser;

	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);

	createHotkeyTable(IDR_DAHOTKEY1, CQE_HOTKEY, &HOTKEY);
	createHotkeyTable(IDR_DAHOTKEY2, CQE_DEBUG_HOTKEY, &DBHOTKEY);
	AddToGlobalCleanupList(&HOTKEY);
	AddToGlobalCleanupList(&DBHOTKEY);

	if (DBHOTKEY->QueryOutgoingInterface("IEventMessageFilter", connection) == GR_OK)
		connection->Advise(getBase(), &filterHandle);

	if (CQFLAGS.bNoGDI==0)
	{
		bDebugEnabled = true;

		if (DACOM->QueryInterface("IProfileParser", parser) == GR_OK)
		{
			HANDLE hSection;

			if ((hSection = parser->CreateSection("Hotkey")) != 0)
			{
				U32 len;
				char buffer[256];

				if ((len = parser->ReadKeyValue(hSection, "Debug", buffer, sizeof(buffer))) != 0)
				{
					if (strncmp(buffer, "0", 1) == 0 || strnicmp(buffer, "off", 2)==0 || strnicmp(buffer, "false", 4)==0)
						bDebugEnabled = false;
				}

				parser->CloseSection(hSection);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _hkmanager : GlobalComponent
{
	HKManager * manager;

	virtual void Startup (void)
	{
		manager = new DAComponent<HKManager>;
		AddToGlobalCleanupList(&manager);
	}

	virtual void Initialize (void)
	{
		manager->init();
	}
};

static _hkmanager hkmanager;

//--------------------------------------------------------------------------//
//----------------------------End HKManager.cpp-------------------------------//
//--------------------------------------------------------------------------//
