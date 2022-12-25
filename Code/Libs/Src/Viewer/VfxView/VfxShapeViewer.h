#ifndef VFXSHAPEVIEWER_H
#define VFXSHAPEVIEWER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               VfxShapeViewer.H                           //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Pbleisch $

    $Header: /Libs/dev/Src/Viewer/VfxView/VfxShapeViewer.h 2     3/21/00 4:30p Pbleisch $
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
struct DACOM_NO_VTABLE Viewer : public Dispatch<Viewer, IViewer>, IDocumentClient
{
public:

	//
	// create a table of interfaces that are supported. 
	// QueryInterface() uses this table.
	//

	BEGIN_DACOM_MAP_INBOUND(Viewer)
	DACOM_INTERFACE_ENTRY(IViewer)
	DACOM_INTERFACE_ENTRY(IDADispatch)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY2(IID_IViewer,IViewer)
	DACOM_INTERFACE_ENTRY2(IID_IDADispatch,IDADispatch)
	DACOM_INTERFACE_ENTRY2(IID_IDocumentClient,IDocumentClient)
	END_DACOM_MAP()

	char szClassName[MAX_NAME_LENGTH];
	char szInstanceName[MAX_NAME_LENGTH];
	HWND hMainWindow;
	RECT rect;

	BOOL32 bVisible, bAutoClose;

	IDocument *doc;
	U32 connHandle;
	U32 currentImage;
	HBITMAP hDIB;
	HPALETTE hPalette;

	LOGPALETTE       *pLogPal;            // LOGPALETTE structure
	BITMAPINFO       *pbmi;               // BITMAPINFO structure
	BITMAPINFOHEADER *pbmih;              // Pointer to pbmi header
	void * lpDIBBuffer;					  // pointer to DIB memory	
	BOOL32 bPaletteChanged;
	U32 dwClientWidth, dwClientHeight;
	BOOL32 bNameOverriden;
	S32 nMinWinX;

	BEGIN_DACOM_DISPATCH_MAP(Viewer)
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
	DACOM_DISPATCH_MEMBER_PROPERTY("auto_close", bAutoClose, DAVT_BOOL32)
	END_DACOM_DISPATCH_MAP()

	//----------------------------------------------------------
	//----------------------------------------------------------

	Viewer (void)
	{
		// NOTE: relies on "new" operator to clear instance memory.
		// Instances on the stack will not be initialized correctly!!!
		rect.left = 300;
		rect.top = 200;
		rect.right = 400;
		rect.bottom = 300;
	}

	void * operator new (size_t size);		// calls calloc()

	~Viewer (void);

	GENRESULT init (VIEWDESC *info);

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

	/* Viewer members */

	LRESULT MainWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
   
	static LRESULT CALLBACK StaticWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	void refreshSurface (HDC hdc);

	IDAComponent * GetBase (void)
	{
		return (IViewer *) this;
	}
};


//--------------------------------------------------------------------------//
//----------------------------End VfxShapeViewer.h--------------------------//
//--------------------------------------------------------------------------//

#endif