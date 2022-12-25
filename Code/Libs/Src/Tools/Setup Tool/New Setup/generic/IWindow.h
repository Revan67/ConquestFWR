// IWindow
//
//
//

#ifndef IWindow_H
#define IWindow_H

#include "DACOM.h"
#include "DACOM_Utility.h"


#define IWINDOW_MANAGED		(1<<0)
#define IWINDOW_EMBEDDED	(1<<1)
#define IWINDOW_MODAL		(1<<2)

static const char *IID_IWindow = "IWindow";
dacom_interface( IWindow )
{
	DACOM_INTERFACE_METHOD( Create,		( HWND hParent, RECT *child_area, U32 flags ));
	DACOM_INTERFACE_METHOD( Destroy,	( void ));
	DACOM_INTERFACE_METHOD( Show,		( U32 show ));
	DACOM_INTERFACE_METHOD( Refresh,	( void ));
	
//	DACOM_INTERFACE_METHOD( Move,		( RECT *new_area ));
};


static const char *IID_IModalWindow = "IModalWindow";
dacom_interface( IModalWindow )
{
	DACOM_INTERFACE_METHOD( DoModal,	( HWND hParent, U32 *out_return ));
	DACOM_INTERFACE_METHOD( SetTitle,	( const char *szTitle ));
};

static const char *IID_IDockingWindow = "IDockingWindow";
dacom_interface( IDockingWindow )
{
	DACOM_INTERFACE_METHOD( Dock,	( HWND hParent ));
	DACOM_INTERFACE_METHOD( Undock,	( void ));
};

static const char *IID_IDockingSite = "IDockingSite";
dacom_interface( IDockingSite )
{
	DACOM_INTERFACE_METHOD( Dock,			( HWND hChild, RECT *inout_rect ));
	DACOM_INTERFACE_METHOD( Undock,			( HWND hChild ));
};


#endif
