//--------------------------------------------------------------------------//
//                                                                          //
//                                Menu_MapSelect.cpp                        //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_MapSelect.cpp 18    6/28/01 2:28p Tmauer $
*/
//--------------------------------------------------------------------------//
// New player dialog
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <wchar.h>

#include "CQGame.h"
#include "Mission.h"

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IEdit2.h"
#include "IListbox.h"

#include <DMenu1.h>

#define MAX_GAMEDESC_CHARS 31

#define LIST_RANDOM		90400
#define LIST_SUPPLIED	90401
#define LIST_SAVED		90402

//--------------------------------------------------------------------------//
//
struct Menu_MapSelect : public DAComponent<Frame>
{
	//
	// data items
	//
	GT_MAPSELECT data;

	COMPTR<IStatic> staticBackground, staticTitle;
	COMPTR<IStatic> staticRandom, staticSupplied, staticSaved;
	COMPTR<IListbox> listRandom, listSupplied, listSaved;
	COMPTR<IButton2> buttonOk, buttonCancel;

	COMPTR<IFileSystem> saveLoadDir;
	COMPTR<IFileSystem> suppliedDir;

	ICQGame & cqgame;
	wchar_t * szFileName;
	const int numChars;
	const CQGAMETYPES::MAPTYPE defaultMapType;

	bool bInitialized;
	bool bSelectionMade;

	struct SuppliedFile
	{
		U32 index;
		wchar_t fileName[MAX_PATH];
		SuppliedFile * next;
	} * suppliedFileList;
	U32 lastSuppliedFileIndex;

	//
	// instance methods
	//

	Menu_MapSelect (ICQGame & game, wchar_t * _szFileName, const int _nChars) : cqgame(game), numChars(_nChars), defaultMapType(game.mapType)
	{
		lastSuppliedFileIndex = 0;
		suppliedFileList = NULL;
		szFileName = _szFileName;
		eventPriority = EVENT_PRIORITY_CONFIRM_EXIT;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();
	}

	~Menu_MapSelect (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message, void *parm);

	/* Menu_MapSelect methods */

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID);

	virtual void onListCaretMove (S32 listControlID);

	virtual void onListSelection (S32 listID);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		onButtonCancel();
		return true;
	}

	void init (void);

	void initMapLists (void);

	void addMultiFile (const WIN32_FIND_DATA & data);

	void addMultiFile (HANDLE handle);

	void addSavedFile (const WIN32_FIND_DATA & data);

	void addSavedFile (HANDLE handle);

	void getFileFromList (IListbox * list);

	void onButtonCancel (void);
};
//----------------------------------------------------------------------------------//
//
Menu_MapSelect::~Menu_MapSelect (void)
{
	//clean up old supplied maps if any
	while(suppliedFileList)
	{
		SuppliedFile * search = suppliedFileList->next;
		delete suppliedFileList;
		suppliedFileList = search;
	}
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_MapSelect::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void Menu_MapSelect::setStateInfo (void)
{
	//screenRect = data.screenRect;
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	staticBackground->InitStatic(data.staticBackground, this);
	staticTitle->InitStatic(data.staticTitle, this);
	staticRandom->InitStatic(data.staticRandom, this); 
	staticSupplied->InitStatic(data.staticSupplied, this);
	staticSaved->InitStatic(data.staticSaved, this);
	
	listRandom->InitListbox(data.listRandom, this);
	listRandom->SetControlID(LIST_RANDOM);

	listSupplied->InitListbox(data.listSupplied, this);
	listSupplied->SetControlID(LIST_SUPPLIED);

	listSaved->InitListbox(data.listSaved, this);
	listSaved->SetControlID(LIST_SAVED);
	
	buttonOk->InitButton(data.buttonOk, this);
	buttonOk->SetTransparent(true);

	buttonCancel->InitButton(data.buttonCancel, this);
	buttonCancel->SetTransparent(true);
	
	initMapLists();

	bInitialized = true;

	if (childFrame)
	{
		childFrame->setStateInfo();
	}
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::init (void)
{
	data = 	*((GT_MAPSELECT *) GENDATA->GetArchetypeData("Menu_MapSelect"));

	//
	// create members
	//
	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.staticBackground.staticType , pComp);
	pComp->QueryInterface("IStatic", staticBackground);

	GENDATA->CreateInstance(data.staticTitle.staticType , pComp);
	pComp->QueryInterface("IStatic", staticTitle);

	GENDATA->CreateInstance(data.staticRandom.staticType , pComp);
	pComp->QueryInterface("IStatic", staticRandom);	

	GENDATA->CreateInstance(data.staticSupplied.staticType , pComp);
	pComp->QueryInterface("IStatic", staticSupplied);	

	GENDATA->CreateInstance(data.staticSaved.staticType , pComp);
	pComp->QueryInterface("IStatic", staticSaved);	
	
	GENDATA->CreateInstance(data.listRandom.listboxType, pComp);
	pComp->QueryInterface("IListbox", listRandom);

	GENDATA->CreateInstance(data.listSupplied.listboxType, pComp);
	pComp->QueryInterface("IListbox", listSupplied);

	GENDATA->CreateInstance(data.listSaved.listboxType, pComp);
	pComp->QueryInterface("IListbox", listSaved);

	GENDATA->CreateInstance(data.buttonOk.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonOk);

	GENDATA->CreateInstance(data.buttonCancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonCancel);
	
	saveLoadDir = SAVEDIR;
	suppliedDir = MPMAPDIR;
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::initMapLists (void)
{
	//clean up old supplied maps if any
	while(suppliedFileList)
	{
		SuppliedFile * search = suppliedFileList->next;
		delete suppliedFileList;
		suppliedFileList = search;
	}

	//
	// add all the random types
	//
	U32 index = 0;
	listRandom->ResetContent();
	
	index = listRandom->AddString(_localLoadStringW(IDS_MAPTYPE_RANDOM));
	listRandom->SetDataValue(index, IDS_MAPTYPE_RANDOM);
		
	index = listRandom->AddString(_localLoadStringW(IDS_MAPTYPE_RING));
	listRandom->SetDataValue(index, IDS_MAPTYPE_RING);
	
	index = listRandom->AddString(_localLoadStringW(IDS_MAPTYPE_STAR));
	listRandom->SetDataValue(index, IDS_MAPTYPE_STAR);
	
	index = listRandom->AddString(_localLoadStringW(IDS_MAPTYPE_NEW_RANDOM));
	listRandom->SetDataValue(index, IDS_MAPTYPE_NEW_RANDOM);

	//
	// add all the maps supplied by the conquest mission designers
	//
	HANDLE handle;
	WIN32_FIND_DATA data;

	listSupplied->ResetContent();

	if (suppliedDir != 0 && (handle = suppliedDir->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
	{
		addMultiFile(handle);		// recursion
		addMultiFile(data);
		
		suppliedDir->FindClose(handle);

		// if there are no designer maps available then make the control invisible
		if (listSupplied->GetNumberOfItems() == 0)
		{
			listSupplied->SetVisible(false);
			staticSupplied->SetVisible(false);
		}
	}

	//
	// add all the multiplayer saved games we have present
	//
	listSaved->ResetContent();

	if (saveLoadDir != 0 && (handle = saveLoadDir->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
	{
		addSavedFile(handle);		// recursion
		addSavedFile(data);
		
		saveLoadDir->FindClose(handle);

		// okay, now initialize all the listboxes
		listSupplied->EnableListbox(listSupplied->GetNumberOfItems() > 0);
		listSaved->EnableListbox(listSaved->GetNumberOfItems() > 0);
	}

	// selete the default map type
	if (cqgame.mapType == CQGAMETYPES::RANDOM_MAP)
	{
		U32 index = 0;

		// which map type is selected by default
		switch (cqgame.templateType)
		{
		case CQGAMETYPES::TEMPLATE_RANDOM:
			index = 0;
			break;

		case CQGAMETYPES::TEMPLATE_RING:
			index = 1;
			break;

		case CQGAMETYPES::TEMPLATE_STAR:
			index = 2;
			break;
		case CQGAMETYPES::TEMPLATE_NEW_RANDOM:
			index = 3;
			break;
		}

		setFocus(listRandom);
		listRandom->SetCurrentSelection(index);
	}
	else if (cqgame.mapType == CQGAMETYPES::USER_MAP)
	{
		// we have a saved game, load its description
		wchar_t szDesc[MAX_PATH];
		char fileAnsi[MAX_PATH];
		_localWideToAnsi(cqgame.szMapName, fileAnsi, sizeof(fileAnsi));
		MISSION->GetFileDescription(fileAnsi, szDesc, sizeof(szDesc)/sizeof(wchar_t));

		// selete the map
		S32 index = listSaved->FindStringExact(szDesc);
		if (index != -1)
		{
			setFocus(listSaved);
			listSaved->SetCurrentSelection(index);
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::getFileFromList (IListbox * list)
{
	wchar_t buffer[MAX_PATH];

	U32 num = list->GetDataValue(list->GetCurrentSelection());
	if(num) //if I am displaying a description and not a file name
	{
		SuppliedFile * supFile = suppliedFileList;
		while(supFile)
		{
			if(supFile->index == num)
			{
				wcsncpy(szFileName, supFile->fileName, numChars);
				return;
			}
			supFile = supFile->next;
		}
	}
	else
	{
		list->GetString(list->GetCurrentSelection(), buffer, sizeof(buffer));
		wcsncpy(szFileName, buffer, numChars);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::onListCaretMove (S32 listID)
{
	if (bInitialized)
	{
		bSelectionMade = true;
	}

	if (listID == LIST_RANDOM && listRandom->GetCurrentSelection() != -1)
	{
		cqgame.mapType = CQGAMETYPES::RANDOM_MAP;
	}
	else if (listID == LIST_SUPPLIED && listSupplied->GetCurrentSelection() != -1)
	{
		cqgame.mapType = CQGAMETYPES::SELECTED_MAP;

		// get the file name
		getFileFromList(listSupplied);
	}
	else if (listID == LIST_SAVED && listSaved->GetCurrentSelection() != -1)
	{
		cqgame.mapType = CQGAMETYPES::USER_MAP;
		
		// get the actual file name back out of the list...
		U32 num = listSaved->GetDataValue(listSaved->GetCurrentSelection());
		swprintf(szFileName, L"m%07d.mission", num);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::onListSelection (S32 listID)
{
	if (bInitialized)
	{
		bSelectionMade = true;
	}

	if (listID == LIST_RANDOM && listRandom->GetCurrentSelection() != -1)
	{
		cqgame.mapType = CQGAMETYPES::RANDOM_MAP;
	}
	else if (listID == LIST_SUPPLIED && listSupplied->GetCurrentSelection() != -1)
	{
		cqgame.mapType = CQGAMETYPES::SELECTED_MAP;

		// get the file name
		getFileFromList(listSupplied);
	}
	else if (listID == LIST_SAVED && listSaved->GetCurrentSelection() != -1)
	{
		cqgame.mapType = CQGAMETYPES::USER_MAP;
		
		// get the actual file name back out of the list...
		U32 num = listSaved->GetDataValue(listSaved->GetCurrentSelection());
		swprintf(szFileName, L"m%07d.mission", num);
	}

	onButtonPressed(IDS_OK);
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::onButtonCancel (void)
{
	// set us back to the old map type
	cqgame.mapType = defaultMapType;
	endDialog(0);
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::onButtonPressed (U32 buttonID)
{
	if (buttonID == IDS_OK)
	{
		if (bSelectionMade)
		{

			U32 result = 1;
			
			if (listRandom->GetCurrentSelection() != -1)
			{
				result = listRandom->GetDataValue(listRandom->GetCurrentSelection());
			}
			endDialog(result);
		}
		else
		{
			onButtonCancel();
		}
	}
	else if (buttonID == IDS_CANCEL)
	{
		onButtonCancel();
	}
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::addMultiFile (const WIN32_FIND_DATA & data)
{
	// ignore directories
	if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		// does the filename end with 'mission' (ie. map1.qmission or map1.dmission or dmap1.mission)?
		if (strlen(data.cFileName) > strlen("mission"))
		{
			const char * fileType = (data.cFileName + strlen(data.cFileName)) - strlen("mission");

			if (strcmp(fileType, "mission") == 0)
			{

				wchar_t buffer[256];

				_localAnsiToWide(data.cFileName, buffer, sizeof(buffer));

				// get the game description string out of the file
				wchar_t string[MAX_GAMEDESC_CHARS];

				bool bAdded = false;
				if (MISSION->GetFileDescription(suppliedDir, data.cFileName, string, sizeof(string)/sizeof(wchar_t)))
				{
					if(string[0] != 0)
					{
						bAdded = true;
						U32 index;

						index = listSupplied->AddString(string);

						lastSuppliedFileIndex++;

						listSupplied->SetDataValue(index, lastSuppliedFileIndex);

						SuppliedFile * newFile = new SuppliedFile;
						newFile->index = lastSuppliedFileIndex;
						_localAnsiToWide(data.cFileName, newFile->fileName, sizeof(newFile->fileName));
						newFile->next = suppliedFileList;
						suppliedFileList = newFile;
					}
				}

				if(!bAdded)
				{
					_localAnsiToWide(data.cFileName, buffer, sizeof(buffer));

					// get the game description string out of the file
					U32 index = listSupplied->AddString(buffer);
					listSupplied->SetDataValue(index, 0);
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::addMultiFile (HANDLE handle)
{
	WIN32_FIND_DATA data;

	if (suppliedDir->FindNextFile(handle, &data))
	{
		addMultiFile(handle);		// recursion
		addMultiFile(data);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::addSavedFile (const WIN32_FIND_DATA & data)
{
	// ignore directories
	if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		wchar_t buffer[256];

		_localAnsiToWide(data.cFileName, buffer, sizeof(buffer));

		// get the game description string out of the file
		wchar_t string[MAX_GAMEDESC_CHARS];

		if (data.cFileName[0] != 'm')
		{
			// all files in multiplayer are of the form m0000000d.mission where d is a whole number
			return;
		}

		if (MISSION->GetFileDescription(data.cFileName, string, sizeof(string)/sizeof(wchar_t)))
		{
			U32 number, index;
			char szNumber[7];

			index = listSaved->AddString(string);

			// get the number associated with the file
			memcpy(szNumber, &data.cFileName[1], sizeof(char)*7);
			number = atol(szNumber);
			listSaved->SetDataValue(index, number);
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_MapSelect::addSavedFile (HANDLE handle)
{
	WIN32_FIND_DATA data;

	if (saveLoadDir->FindNextFile(handle, &data))
	{
		addSavedFile(handle);		// recursion
		addSavedFile(data);
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_MapSelect (BaseHotRect * parent, ICQGame & cqgame, wchar_t * szFileName, const int numChars)
{
	Menu_MapSelect * menu = new Menu_MapSelect(cqgame, szFileName, numChars);

	menu->createViewer("\\GT_MAPSELECT\\Menu_MapSelect", "GT_MAPSELECT", IDS_VIEWMAPSELECT);
	menu->beginModalFocus();

	U32 result = CQDoModal(menu);
	delete menu;

	return result;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_MapSelect.cpp-----------------------//
//--------------------------------------------------------------------------//