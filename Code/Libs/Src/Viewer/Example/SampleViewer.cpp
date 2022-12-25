//--------------------------------------------------------------------------//
//                                                                          //
//                             SampleViewer.cpp                             //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $
*/			    
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "SampleViewer.h"
#include "Document.h"
#include "IConnection.h"

#include <stdlib.h>


#define IDC_EDIT1	0x00000401		// child edit box control ID


extern HINSTANCE hInstance;		// initialized in some other module

static char interface_name[] = "IViewer";

union DATATYPE
{
	void *value;
	void **pHandle;
	char *string;
	RECT *rect;
	long *pLong;
};

/*

	Simple viewer of text. Creates a window with a child edit control.
	When the user changes the text in the control and presses RETURN, 
	the document is updated. Other clients are notified of the change.

*/


//--------------------------------------------------------------------------
//---------------------------SampleViewer class-------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Note that we don't bother to unregister ourselves with the document provider here.
// If dwRefs==0, then we can assume that we have already unregistered ourselves.
// (UnregisterClient causes the document to call our Release method.)
// For the above reason, we can assume that doc is NULL already.
//
SampleViewer::~SampleViewer (void)
{
	if (hMainWindow)
		DestroyWindow(hMainWindow);
	hMainWindow=0;

	if (doc)
		OnClose(doc);
}
//--------------------------------------------------------------------------
//
void * SampleViewer::operator new (size_t size)
{
	return calloc(size, 1);
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::set_display_state (BOOL32 state)
{
 	bVisible = (state != 0);
	if (hMainWindow)
	{
		ShowWindow(hMainWindow, (bVisible) ? SW_SHOW:SW_HIDE);
		if (bVisible)
		{	
			if (hEdit)
				SetFocus(hEdit);
		}
		return GR_OK;
	}
	return GR_GENERIC;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_display_state (BOOL32 *state)
{
	if (state)
	{
		*state = bVisible;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::set_display_value (const C8 *name)
{
	if (name)
	{
		strncpy(szDisplayName, name, sizeof(szInstanceName)-1);
		SetWindowText(hEdit, szDisplayName);
		bWinDataChanged=0;		// doesn't count if the application changed the data
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_display_value (C8 *name, U32 bufferLength)
{
	if (name)
	{
		if (bufferLength)
			strncpy(name, szDisplayName, bufferLength);
		else
			strcpy(name, szDisplayName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_class_name (C8 *name)
{
	if (name)
	{
		strcpy(name, szClassName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::set_instance_name (const C8 *name)
{
	if (name)
	{
		strncpy(szInstanceName, name, sizeof(szInstanceName)-1);
		SetWindowText(hMainWindow, szInstanceName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_instance_name (C8 *name)
{
	if (name)
	{
		strcpy(name, szInstanceName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_main_window (void **hwnd)
{
	if (hwnd)
	{
		*hwnd = hMainWindow;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::set_rect (const struct tagRECT *pRect)
{
	if (pRect)
	{
		rect = *pRect;
		if (hMainWindow)
			MoveWindow(hMainWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom-rect.top, TRUE);
		else
			return GR_GENERIC;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_rect (struct tagRECT *pRect)
{
	if (pRect)
	{
		*pRect = rect;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::set_read_only (BOOL32 value)
{
	bReadOnly = (value != 0);
	SendMessage(hEdit, EM_SETREADONLY, bReadOnly, 0);
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_read_only (BOOL32 *value)
{
	if (value)
	{
		*value = bReadOnly;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_string_length (U32 *value)
{
	if (value)
	{
		*value = MAX_NAME_LENGTH;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::set_auto_close (BOOL32 value)
{
	bAutoClose = (value != 0);
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_auto_close (BOOL32 *value)
{
	if (value)
	{
		*value = bAutoClose;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::set_hex_numbers (BOOL32 value)
{
	return GR_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::get_hex_numbers (BOOL32 *value)
{
	return GR_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------
// Called when a document client has modified the document.
// '_doc' may be our document, or a child document. (sub document)
//  This viewer ignores the 'message' and 'parm' parameters, and always
//  updates all of its data.
//
GENRESULT SampleViewer::OnUpdate (struct IDocument *_doc, const C8 *message, void *parm)
{
	DWORD dwRead;
	char *pData;

	if (doc != _doc)			// is this our document? (ignore updates from child docs)
		return GR_GENERIC;

	dwRead = doc->GetFileSize();
	if ((pData = (char *) malloc(dwRead+1)) == 0)
		return GR_OUT_OF_MEMORY;

	pData[dwRead] = 0;		// set terminating character

	doc->SetFilePointer(0,0);
	if (doc->ReadFile(0, pData, dwRead, &dwRead, 0) == 0)
	{
		return GR_FILE_ERROR;
	}

	// do something with data

	set_display_value(pData);

	free(pData);

	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::OnClose (struct IDocument *document)
{
	if (document == doc && doc)	//is this the right instance?
	{
		COMPTR<IDAConnectionPoint> connection;

		if (doc->QueryOutgoingInterface("IDocumentClient", connection) != GR_OK)
			return GR_GENERIC;

		if (hMainWindow)
			DestroyWindow(hMainWindow);
		hMainWindow = 0;
		//
		//	...
		//
		// calling doc->Unadvise() will result in our reference count being 
		// decremented. So add a false reference to ourselves for the duration
		// of this function call.
		// Also note that we use the BaseComponent() method to cast "this" to 
		// a generic IDAComponent pointer. This must be done because SampleViewer
		// does not define AddRef() and Release()
		//

		connection->Unadvise(connHandle);
		doc = 0;
		BaseComponent()->Release();		// release our extra reference
	}

	return GR_OK;
}
//--------------------------------------------------------------------------
// This viewer's data is the display value. Write the data to the document
// and notify everyone of the modification.
//
BOOL32 SampleViewer::WriteNewData (void)
{
	BOOL32 result = 0;
	DWORD dwWrite;

	if (doc)
	{
		doc->SetFilePointer(0,0);
		if (doc->WriteFile(0, szDisplayName, sizeof(szDisplayName), &dwWrite, 0))
			result = 1;
		doc->UpdateAllClients();
	}	

	return result;
}
//----------------------------------------------------------------------------
// Catch the ESC and RETURN button presses. Turn these into IDOK and IDCANCEL messages.
//
LONG CALLBACK SampleViewer::EditControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	SampleViewer *pViewer = (SampleViewer *) GetWindowLong(GetParent(hwnd), GWL_USERDATA);

	if (pViewer)
	switch (message)
	{
		case WM_CHAR:
			switch (LOWORD(wParam))
			{
			case 13:
				PostMessage(pViewer->hMainWindow, WM_COMMAND, IDOK, (LONG)hwnd);	// send message to parent
				return 0;
			case 27:
				PostMessage(pViewer->hMainWindow, WM_COMMAND, IDCANCEL, (LONG)hwnd);	// send message to parent
				return 0;
			}
			break;
	}

//	return CallWindowProc((int(__stdcall *)(void))pViewer->lpfnOldEditProcedure, hwnd, message, wParam, lParam);
	return CallWindowProc((WNDPROC)pViewer->lpfnOldEditProcedure, hwnd, message, wParam, lParam);
}
//--------------------------------------------------------------------------
//
LRESULT SampleViewer::MainWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	LRESULT result = 0;

	switch (message)
	{
	case WM_CREATE:
		hMainWindow = hwnd;

		hEdit = CreateWindow("edit", szDisplayName,
			WS_CHILD|WS_VISIBLE|ES_MULTILINE, 
			rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			hMainWindow,
			(HMENU) IDC_EDIT1,
			hInstance,
			this);

		// hook into window procedure

		if ((lpfnOldEditProcedure = (WNDPROC) GetWindowLong(hEdit, GWL_WNDPROC)) != 0)
			SetWindowLong(hEdit, GWL_WNDPROC, (LONG) EditControlProcedure);

		{
			RECT localrect;
			GetClientRect(hwnd, &localrect);
			MoveWindow(hwnd, rect.left, rect.top, localrect.right, localrect.bottom , FALSE);
		}
		result=1;
		break;	// end WM_CREATE case

	case WM_MOVE:
		GetWindowRect(hwnd, &rect);
		break;

	case WM_SIZE:
		{
			WORD wWidth, wHeight;

			wWidth = LOWORD(lParam);
			wHeight = HIWORD(lParam);
			GetWindowRect(hwnd, &rect);

			if (hEdit)
				MoveWindow(hEdit, 0, 0, wWidth, wHeight , TRUE);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (bReadOnly==false && bWinDataChanged)
			{
				GetWindowText((HWND)lParam, szDisplayName, sizeof(szDisplayName)-1);
				bWinDataChanged=0;
				WriteNewData();
			}

			// fall through intentional

		case IDCANCEL:
			SetWindowText(hEdit, szDisplayName);
			SendMessage(hEdit, EM_SETSEL, 0, -1);
			bWinDataChanged=0;
			if (bAutoClose)
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case IDC_EDIT1:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				bWinDataChanged=1;
				break;
			case EN_SETFOCUS:
				if (bReadOnly)
					SendMessage(hEdit, EM_SETSEL, -1, -1);
				break;
			case EN_KILLFOCUS:
				PostMessage(hwnd, WM_COMMAND, IDOK, lParam);	// send message to parent
			}
			break;

		}
		break;

	case WM_CLOSE:
		set_display_state(0);
		result = 1;
		break;

	case WM_DESTROY:
		if (hMainWindow==hwnd)
		{
			hEdit = 0;
			bVisible=0;
			hMainWindow=0;
			bWinDataChanged=0;
		}
		break;


	} // end switch (message)

	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 SampleViewer::init (void)
{
	WNDCLASS wndclass;
	BOOL32 result=0;

	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = StaticWndProc;
	wndclass.hInstance = hInstance;
	wndclass.hCursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_ARROW));
	wndclass.lpszClassName = "SampleViewer";

	if (RegisterClass(&wndclass) != 0)
	{
		hMainWindow = CreateWindow("SampleViewer", "No Name",
			WS_CAPTION|WS_POPUP|WS_THICKFRAME|WS_SYSMENU, 
			rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			hParentWindow,
			0,
			hInstance,
			this);

		result=1;
	}

	return result;
}
//--------------------------------------------------------------------------
//
GENRESULT SampleViewer::init (VIEWDESC *lpDesc)
{
	COMPTR<IDAConnectionPoint> connection;
	GENRESULT result = GR_OK;

	if (strcmp(lpDesc->className, "SampleViewer") != 0)
	{
		result = GR_INTERFACE_UNSUPPORTED;
		goto Done;
	}

	if (lpDesc->doc == 0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if (lpDesc->doc->QueryOutgoingInterface("IDocumentClient", connection) != GR_OK)
	{
		result = GR_GENERIC;
		goto Done;
	}

	// NOTE: could also check the data to make sure it is of the right form.
	// This sample viewer does not care about the form of the data.

	doc = lpDesc->doc;
	strcpy(szClassName, lpDesc->className);
	
	if (init() == 0 ||
		connection->Advise((IDADispatch *)this, &connHandle) != GR_OK)
	{
		result = GR_GENERIC;
		goto Done;
	}

	OnUpdate(doc);

	BaseComponent()->AddRef();		// add an extra reference

Done:
	return result;
}
//--------------------------------------------------------------------------
//
LRESULT CALLBACK SampleViewer::StaticWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	if (message == WM_CREATE)
		SetWindowLong(hwnd, GWL_USERDATA, 
			(long)((CREATESTRUCT *)lParam)->lpCreateParams);	// store "this" pointer in dialog data

	SampleViewer *viewer = (SampleViewer *) GetWindowLong(hwnd, GWL_USERDATA);

	if (viewer)
		viewer->MainWndProc(hwnd, message, wParam, lParam);

	return DefWindowProc(hwnd, message, wParam, lParam);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------//
// Called by the DLL startup code, registers the SampleViewerFactory with the DACO manager.
//
void RegisterSampleViewer (ICOManager * DACOM)
{
	IComponentFactory *sample;

	if ((sample = new DAComponentFactory<DAComponent<SampleViewer>,VIEWDESC>(interface_name)) != 0)
	{
		DACOM->RegisterComponent(sample, interface_name);
		sample->Release();
	}
}


//------------------------------------------------------------------------------//
//---------------------------END SampleViewer.cpp-------------------------------//
//------------------------------------------------------------------------------//

