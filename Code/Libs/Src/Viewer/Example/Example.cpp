//--------------------------------------------------------------------------//
//                                                                          //
//                               Example.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author:   JYENAWINE  $
*/			    
//--------------------------------------------------------------------------//

/*

	This is an example program to demonstrate how to use a viewer.
	SampleViewer.cpp contains the code for a simple viewer of text strings.

	In order for this program to run, you must have the following DLL's:
		DACOM.DLL
		DOCUVIEW.DLL
		DOSFILE.DLL

*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <DACOM.h>
#include <Viewer.h>
#include <Document.h>
#include <HeapObj.h>
#include <IDispatch.h>
#include "TSmartPointer.h"


COMPTR<ICOManager> DACOM;		// global pointer to DACOM manager
HINSTANCE hInstance;		// handle to our exe instance, initialized in main

//--------------------------------------------------------------------------//
// This function creates our SampleViewer factory, and registers it with the DACOM manager
//
void RegisterSampleViewer (ICOManager * DACOM);

//--------------------------------------------------------------------------//
// Prints out the current state of the heap to the debug output window.
// This is useful for finding memory leaks.
//
int PrintHeap (IHeap *pHeap);

//--------------------------------------------------------------------------//
// Sets the owner field of all allocated blocks to 0xffffffff.
// PrintHeap() does not print blocks with owner == 0xffffffff.
//
int MarkAllocatedBlocks(IHeap *pHeap);


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// clean_up()  called by atexit()
//
void clean_up (void)
{
	PrintHeap(HEAP);
	if (DACOM)
		DACOM->ShutDown();
}

//--------------------------------------------------------------------------//
//
void TestViewer (void)
{
	DOCDESC ddesc;
	COMPTR<IDocument> doc;
	char buffer[64];
	
	strcpy(buffer, "My string data");
	ddesc.memory = buffer;
	ddesc.memoryLength = sizeof(buffer);


	//
	// first create the document.
	// for this example, we create the document from memory
	//
	
	if (DACOM->CreateInstance(&ddesc, doc) == GR_OK)
	{
		COMPTR<IViewer> viewer;
		VIEWDESC vdesc;

		//
		// now create a viewer for the document.
		// our viewer responds to the className "SampleViewer"
		//

		vdesc.className = "SampleViewer";
		vdesc.doc = doc;
		
		if (DACOM->CreateInstance(&vdesc, viewer) == GR_OK)
		{
			viewer->Invoke("set_instance_name", "My Instance Name");
//			viewer->Invoke("set_display_state", 1);		// turn on the display
			viewer->set_display_state(1);		// turn on the display

			{
				char buffer[64];
				BOOL32 bAutoClose;

				viewer->Invoke("get_instance_name", buffer);

				viewer->GetProperty("auto_close", &bAutoClose);
//				viewer->SetProperty("auto_close", &bAutoClose);
//				bAutoClose++;
				viewer->SetProperty("auto_close", &bAutoClose);
			}
			//
			// Loop until the user closes the viewer window.
			//

			while (1)
			{
				U32 bVisible;
				MSG msg;

				if ((viewer->GetProperty("display_state", &bVisible)) != GR_OK || bVisible == 0)
					break;

				if (doc->IsModified())
				{
					MessageBox(0, "You've changed some data!", "Viewer Example", MB_OK);
					doc->SetModified(0);
				}

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
}	

//--------------------------------------------------------------------------//
//
int CALLBACK WinMain (HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	hInstance = _hInstance;		// initialize global instance handle

	atexit(clean_up);
	DACOM = DACOM_Acquire();

	//
	// register our viewer with the system
	//
	
	RegisterSampleViewer(DACOM);

	//
	// mark all currently allocated blocks in heap (removes them from final printout)
	//

	MarkAllocatedBlocks(HEAP);

	// run the test

	TestViewer();

	return 0;
}


//--------------------------------------------------------------------------//
//-------------------------End Example.cpp----------------------------------//
//--------------------------------------------------------------------------//
