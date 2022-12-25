//--------------------------------------------------------------------------//
//                                                                          //
//                               VfxView.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: $
*/			    
//--------------------------------------------------------------------------//
/*
	This is an example program to demonstrate how to use a viewer.
	VfxView.cpp contains the code for a simple viewer of VFX shape files.

	In order for this program to run, you must have the following DLL's:
		DACOM.DLL
		VFXView.DLL
		DOSFILE.DLL
		DOCUVIEW.DLL

*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <DACOM.h>
#include <Viewer.h>
#include <Document.h>
#include <HeapObj.h>
#include <IDispatch.h>
#include <TSmartPointer.h>

ICOManager * DACOM;		// global pointer to DACOM manager
HINSTANCE hInstance;		// handle to our exe instance, initialized in main

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// clean_up()  called by atexit()
//
void clean_up (void)
{
	if (DACOM)
		DACOM->ShutDown();
}

//--------------------------------------------------------------------------//
//
void RunViewer (LPSTR lpszCmdLine)
{
	DOCDESC ddesc;
	COMPTR<IDocument> doc;

	ddesc.lpFileName = lpszCmdLine;
	
	//
	// first create the document.
	//
	
	if (DACOM->CreateInstance(&ddesc, doc) == GR_OK)
	{
		COMPTR<IViewer> viewer;
		VIEWDESC vdesc;

		//
		// now create a viewer for the document.
		// our viewer responds to the className "VFX_SHAPE"
		//

		vdesc.className = "VFX_SHAPE";
		vdesc.doc = doc;
		
		if (DACOM->CreateInstance(&vdesc, viewer) == GR_OK)
		{
			viewer->set_display_state(1);		// turn on the display

			//
			// Loop until the user closes the viewer window.
			//

			while (1)
			{
				U32 bVisible;
				MSG msg;

				if ((viewer->GetProperty("display_state", &bVisible)) != GR_OK || bVisible == 0)
					break;

				if (GetMessage (&msg, NULL, 0, 0))
				{
					TranslateMessage (&msg);
					DispatchMessage (&msg);
				}
				else
					break;
			}
		}
	}
	else
	{
		MessageBox(0, "Could not open the shape file!\nThis program requires the following DLL's:\n\tDosfile.dll\n\tdocuview.dll\n\tvfxview.dll", "VfxViewer", MB_OK | MB_ICONSTOP);
		MessageBox(0, lpszCmdLine, "VfxViewer", MB_OK | MB_ICONSTOP);
	}
}	

//--------------------------------------------------------------------------//
//
int CALLBACK WinMain (HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	hInstance = _hInstance;		// initialize global instance handle

	atexit(clean_up);
	DACOM = DACOM_Acquire();
	DACOM->SetINIConfig("[Libraries]\nDosfile.dll\ndocuview.dll\nvfxview.dll", DACOM_INI_STRING);

	// remove quotes
	if (lpszCmdLine[0] == '"')
	{
		lpszCmdLine++;
		char * tmp;
		if ((tmp = strchr(lpszCmdLine, '"')) != 0)
			*tmp = 0;
	}

	// run the viewer
	RunViewer(lpszCmdLine);

	return 0;
}


//--------------------------------------------------------------------------//
//-------------------------End VFXView.cpp----------------------------------//
//--------------------------------------------------------------------------//
