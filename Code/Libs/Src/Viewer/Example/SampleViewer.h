#ifndef SAMPLEVIEWER_H
#define SAMPLEVIEWER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               SampleViewer.H                             //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Pbleisch $
*/			    
//--------------------------------------------------------------------------//


#ifndef VIEWER_H
#include "Viewer.h"
#endif

#ifndef IDOCCLIENT_H
#include "IDocClient.h"
#endif

#ifndef TCOMPONENT_H
#include "TComponent.h"
#endif

#ifndef TDISPATCH_H
#include "TDispatch.h"
#endif

#ifndef TSMARTPOINTER_H
#include "TSmartPointer.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define MAX_NAME_LENGTH 32

//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE SampleViewer : public Dispatch<SampleViewer, IViewer>, IDocumentClient
{
public:

	//
	// create a table of interfaces that are supported. 
	// QueryInterface() uses this table.
	//

	BEGIN_DACOM_MAP_INBOUND(SampleViewer)
	DACOM_INTERFACE_ENTRY(IViewer)
	DACOM_INTERFACE_ENTRY(IDADispatch)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY2(IID_IViewer,IViewer)
	DACOM_INTERFACE_ENTRY2(IID_IDADispatch,IDADispatch)
	DACOM_INTERFACE_ENTRY2(IID_IDocumentClient,IDocumentClient)
	END_DACOM_MAP()

	char szDisplayName[MAX_NAME_LENGTH];
	char szClassName[MAX_NAME_LENGTH];
	char szInstanceName[MAX_NAME_LENGTH];
	HWND hParentWindow;
	HWND hMainWindow;
	RECT rect;

	BOOL32 bVisible, bReadOnly, bAutoClose, bWinDataChanged;
	HWND hEdit;
	WNDPROC lpfnOldEditProcedure;

	IDocument *doc;
	U32 connHandle;

	// enumeration which matches the items in the property list, below
	enum PROP
	{
		DISPLAY_STATE,
		DISPLAY_VALUE,
		CLASS_NAME,
		INSTANCE_NAME,
		PARENT_WINDOW,
		MAIN_WINDOW,
		WINDOW_RECT,
		READ_ONLY_DATA,
		STRING_LENGTH,
		AUTO_CLOSE
	};

	BEGIN_DACOM_DISPATCH_MAP(SampleViewer)
	DACOM_DISPATCH_METHOD(set_display_state,  DAVT_BOOL32)
	DACOM_DISPATCH_METHOD(get_display_state,  DAVT_BOOL32|DAVT_BYREF)
	DACOM_DISPATCH_METHOD(get_class_name,     DAVT_STRING)
	DACOM_DISPATCH_METHOD(set_instance_name,  DAVT_STRING)
	DACOM_DISPATCH_METHOD(get_instance_name,  DAVT_STRING)
	DACOM_DISPATCH_METHOD(get_main_window,	  DAVT_PVOID|DAVT_BYREF)
	DACOM_DISPATCH_METHOD(set_rect,			  DAVT_PRECT)
	DACOM_DISPATCH_METHOD(get_rect,			  DAVT_PRECT)
	DACOM_DISPATCH_METHOD(set_read_only,	  DAVT_BOOL32)
	DACOM_DISPATCH_METHOD(get_read_only,	  DAVT_PBOOL32)
	DACOM_DISPATCH_METHOD(set_display_value,  DAVT_STRING)
	DACOM_DISPATCH_METHOD(get_display_value,  DAVT_STRING)
	DACOM_DISPATCH_MEMBER_PROPERTY("auto_close", bAutoClose, DAVT_BOOL32)
	END_DACOM_DISPATCH_MAP()

	//----------------------------------------------------------
	//----------------------------------------------------------

	SampleViewer (void)
	{
		// NOTE: relies on "new" operator to clear instance memory.
		// Instances on the stack will not be initialized correctly!!!
		rect.left = 300;
		rect.top = 200;
		rect.right = 400;
		rect.bottom = 300;
	}

	void * operator new (size_t size);		// calls calloc()

	~SampleViewer (void);

	/* IViewer members */
	
	DEFMETHOD(set_display_state) (BOOL32 state);

	DEFMETHOD(get_display_state) (BOOL32 *state);

	DEFMETHOD(get_class_name) (C8 *name);

	DEFMETHOD(set_instance_name) (const C8 *name);

	DEFMETHOD(get_instance_name) (C8 *name);

	DEFMETHOD(get_main_window) (void **hwnd);

	DEFMETHOD(set_rect) (const struct tagRECT *rect);

	DEFMETHOD(get_rect) (struct tagRECT *rect);

	DEFMETHOD(set_read_only) (BOOL32 value);

	DEFMETHOD(get_read_only) (BOOL32 *value);

	/* IDocumentClient members */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message=0, void *parm=0);

	DEFMETHOD(OnClose) (struct IDocument *doc);

	/* SampleViewer members */

	DEFMETHOD(set_display_value) (const C8 *name);

	DEFMETHOD(get_display_value) (C8 *name, U32 bufferLength);

	DEFMETHOD(get_string_length) (U32 *value);

	DEFMETHOD(set_auto_close) (BOOL32 value);

	DEFMETHOD(get_auto_close) (BOOL32 *value);

	DEFMETHOD(set_hex_numbers) (BOOL32 value);

	DEFMETHOD(get_hex_numbers) (BOOL32 *value);

	BOOL32 init (void);

	BOOL32 WriteNewData (void);

	LRESULT MainWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
   
	static LRESULT CALLBACK StaticWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	static LONG CALLBACK EditControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam);

	IDAComponent * BaseComponent (void)
	{
		return (IDAComponent *) (daoffsetofclass(IViewer, SampleViewer) + ((U32) this));
	}

	GENRESULT init (VIEWDESC *lpDesc);
};


//--------------------------------------------------------------------------//
//----------------------------End SampleViewer.h--------------------------------//
//--------------------------------------------------------------------------//

#endif