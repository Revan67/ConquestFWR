#ifndef VIEWER_H
#define VIEWER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Viewer.H                                   //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
*/			    
//--------------------------------------------------------------------------//


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
/*
	It is recommended (not required) that viewers have the following properties:

	Property					Description
	--------					-----------
  
	"display_state"				TRUE if data is being displayed. (BOOL32)
	"class_name"				Data type identifier. [read-only] (ASCIIZ string)
	"instance_name"				Instance identifier. (ASCIIZ string)
	"main_window"				Main window of viewer. (HWND)
	"rect"						Recomended size/position of window. (RECT *)
	"read_only"					TRUE if data cannot be modified. (BOOL32)

	By convention, the default value for boolean (BOOL32) properties is FALSE.

	Viewers support (at least) the following interfaces:
		IViewer (Default interface)		 
		IDADispatch						 ( IDispatch.h  )		
		IDocumentClient	                 ( IDocClient.h )

*/

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#ifndef DACOM_H
#include "DACOM.H"
#endif

#ifndef IDISPATCH_H
#include "IDispatch.h"
#endif

#ifndef _WINDOWS_
typedef void *HANDLE;
#endif

struct VIEWDESC : public DACOMDESC
{
	const C8 *			className;
	struct IDocument *	doc;
	U32  dwFlags;			// no flags defined yet
	HANDLE hOwnerWindow;
	struct IStringSet * spellingSet; //used to check spelling

	VIEWDESC (const C8 *_interfaceName = "IViewer") : DACOMDESC(_interfaceName)
	{
		memset(((char *)this)+sizeof(DACOMDESC), 0, sizeof(*this)-sizeof(DACOMDESC));
		size = sizeof(*this);
	};
};

//

#define IID_IViewer MAKE_IID( "IViewer", 1 )

//

struct DACOM_NO_VTABLE IViewer : public IDADispatch
{
	DEFMETHOD(set_display_state) (BOOL32 state) = 0;

	DEFMETHOD(get_display_state) (BOOL32 *state) = 0;

	DEFMETHOD(get_class_name) (C8 *name) = 0;

	DEFMETHOD(set_instance_name) (const C8 *name) = 0;

	DEFMETHOD(get_instance_name) (C8 *name) = 0;

	DEFMETHOD(get_main_window) (void **hwnd) = 0;

	DEFMETHOD(set_rect) (const struct tagRECT *rect) = 0;

	DEFMETHOD(get_rect) (struct tagRECT *rect) = 0;

	DEFMETHOD(set_read_only) (BOOL32 value) = 0;

	DEFMETHOD(get_read_only) (BOOL32 *value) = 0;

	DEFMETHOD(set_viewer_path) (const char *path) = 0;
};

//--------------------------------------------------------------------------//
//----------------------------End Viewer.h----------------------------------//
//--------------------------------------------------------------------------//

#endif